#!/usr/bin/env python3

import copy
import struct
import zlib
import random

import bfrt_grpc.client as gc
from bfruntime_client_base_tests import BfRuntimeTest
from p4testutils.misc_utils import *
from ptf.testutils import *

from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP
from scapy.packet import Packet, bind_layers
from scapy import fields

from prettytable import PrettyTable
from dataclasses import dataclass

assert test_param_get("arch") in ["tofino", "tofino2"]

STORAGE_SERVER_NF_DEV = 0
STORAGE_SERVER_PORT = 0 if test_param_get("arch") == "tofino" else 8
CLIENT_NF_DEV = 1
CLIENT_PORT = 1 if test_param_get("arch") == "tofino" else 16

PROGRAM = "cuckoo_hash_table"

TIMEOUT_SEC = 2
SRC_MAC = "00:11:22:33:44:55"
DST_MAC = "00:11:22:33:44:56"

HASH_SALT_1 = 0xFBC31FC7
HASH_SALT_2 = 0x2681580B

KVS_UDP_PORT = 670
KVS_KEY_SIZE_BYTES = 4
KVS_VALUE_SIZE_BYTES = 4
KVS_OP_GET = 0
KVS_OP_PUT = 1
KVS_STATUS_OK = 1
KVS_STATUS_FAIL = 0

CUCKOO_CAPACITY = 8192
EXPIRATION_TIME = 256  # units of 65536 ns = 16ms
MAX_LOOPS = 4


class KVS(Packet):
    name = "kvs"
    fields_desc = [
        fields.BitField("ts", 0, 32),
        fields.BitField("op", 0, 8),
        fields.BitField("key", 0, KVS_KEY_SIZE_BYTES * 8),
        fields.BitField("val", 0, KVS_VALUE_SIZE_BYTES * 8),
        fields.BitField("status", 0, 8),
        fields.BitField("client_port", 0, 16),
    ]

    def __str__(self):
        return f"KVS{{ts={self.ts} op={self.op}, key={self.key:08x}, val={self.val:08x}, status={self.status}, client_port={self.client_port}}}"


bind_layers(UDP, KVS)


def generate_random_key():
    return random.randint(0, 2 ** (KVS_KEY_SIZE_BYTES * 8) - 1)


def generate_random_value():
    return random.randint(0, 2 ** (KVS_VALUE_SIZE_BYTES * 8) - 1)


def generate_random_ip():
    return f"{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}"


def generate_random_port():
    return random.randint(1024, 65535)


def send_and_expect(bfruntimetest, kvs: KVS, expected_port: int, send_to_port: int = CLIENT_PORT, verbose: bool = False) -> KVS:
    ipkt = Ether(src=SRC_MAC, dst=DST_MAC)
    ipkt /= IP(src=generate_random_ip(), dst=generate_random_ip())
    ipkt /= UDP(sport=generate_random_port(), dport=KVS_UDP_PORT)
    ipkt /= kvs

    if verbose:
        print(f"[sent] {kvs}")

    send_packet(bfruntimetest, send_to_port, ipkt)
    _, _, obytes, _ = dp_poll(bfruntimetest, 0, expected_port, timeout=TIMEOUT_SEC)

    assert obytes is not None, f"[recv] No response from port {expected_port}"
    opkt = Ether(obytes)

    assert IP in opkt and UDP in opkt
    oapp = KVS(bytes(opkt[3]))

    if verbose:
        print(f"[recv] {oapp}")

    return oapp


def hash(key: int):
    hash_size_bits = (CUCKOO_CAPACITY - 1).bit_length()
    input = key.to_bytes(KVS_KEY_SIZE_BYTES, byteorder="big")
    output = zlib.crc32(input)
    return output & ((1 << hash_size_bits) - 1)


def hash1(key: int):
    hash_size_bits = (CUCKOO_CAPACITY - 1).bit_length()
    input = key.to_bytes(KVS_KEY_SIZE_BYTES, byteorder="big")
    salt = struct.pack("!I", HASH_SALT_1)
    output = zlib.crc32(salt + input)
    return output & ((1 << hash_size_bits) - 1)


def hash2(key: int):
    hash_size_bits = (CUCKOO_CAPACITY - 1).bit_length()
    input = key.to_bytes(KVS_KEY_SIZE_BYTES, byteorder="big")
    salt = struct.pack("!I", HASH_SALT_2)
    output = zlib.crc32(salt + input)
    return output & ((1 << hash_size_bits) - 1)


class Table(object):
    def __init__(self, bfrt_info, name):
        self.bfrt_info = bfrt_info
        self.name = name
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)
        self.table = bfrt_info.table_get(name)

    def clear(self):
        resp = self.table.entry_get(self.target, [], {"from_hw": True})
        for _, key in resp:
            if key:
                self.table.entry_del(self.target, [key])

    def dump(self):
        table = PrettyTable()

        print(f"====================== {self.name} ======================")
        table.field_names = ["Key", "Data"]

        target = gc.Target(device_id=0, pipe_id=0xFFFF)
        keys = self.table.entry_get(target, [], {"from_hw": False})

        for data, key in keys:
            keys = [f"{key_field}: {key.to_dict()[key_field]['value']}" for key_field in key.to_dict().keys()]
            datas = [f"{data_field}: {data.to_dict()[data_field]['value']}" for data_field in data.to_dict().keys()]
            table.add_row(["\n".join(keys), "\n".join(datas)])

        print(table)


class Register(Table):
    def __init__(self, bfrt_info, name):
        super().__init__(bfrt_info, name)

    def read(self, index: int) -> int:
        key = self.table.make_key([gc.KeyTuple("$REGISTER_INDEX", index)])
        resp = self.table.entry_get(self.target, [key], {"from_hw": True})
        data = next(resp)[0].to_dict()

        # On real hardware, we have a register per pipe.
        # During testing, we assume a single pipe usage, and so discard all the other pipe values.
        value = data[f"{self.name}.f1"][0]

        return value

    def get_all(self):
        target = gc.Target(device_id=0, pipe_id=0xFFFF)
        keys = self.table.entry_get(target, [], {"from_hw": True})

        all_values = {}
        for data, key in keys:
            index = key.to_dict()["$REGISTER_INDEX"]["value"]
            value = data.to_dict()[f"{self.name}.f1"][0]
            all_values[index] = value

        return all_values

    def write(self, index: int, value: int):
        key = self.table.make_key([gc.KeyTuple("$REGISTER_INDEX", index)])
        data = self.table.make_data([gc.DataTuple(f"{self.name}.f1", value)])
        self.table.entry_add(self.target, [key], [data])

    def clear(self):
        resp = self.table.entry_get(self.target, [], {"from_hw": True})
        for _, key in resp:
            index = key.to_dict()["$REGISTER_INDEX"]["value"]
            if key:
                self.write(index, 0)

    def dump(self):
        table = PrettyTable()

        print(f"====================== {self.name} ======================")

        table.field_names = ["Index", "Value"]

        target = gc.Target(device_id=0, pipe_id=0xFFFF)
        keys = self.table.entry_get(target, [], {"from_hw": True})

        last = None
        set_three_dots = False

        for data, key in keys:
            index = key.to_dict()["$REGISTER_INDEX"]["value"]
            value = data.to_dict()[f"{self.name}.f1"][0]

            if last != value:
                table.add_row([hex(index), value])
                set_three_dots = False
            elif last == value and not set_three_dots:
                table.add_row(["...", "..."])
                set_three_dots = True

            last = value

        print(table)


class IngressPortToNFDev(Table):
    def __init__(self, bfrt_info):
        super().__init__(bfrt_info, "Ingress.ingress_port_to_nf_dev")

    def setup(self):
        try:
            self.table.entry_add(
                self.target,
                [self.table.make_key([gc.KeyTuple("ig_intr_md.ingress_port", CLIENT_PORT)])],
                [self.table.make_data([gc.DataTuple("nf_dev", CLIENT_NF_DEV)], "Ingress.set_ingress_dev")],
            )

            self.table.entry_add(
                self.target,
                [self.table.make_key([gc.KeyTuple("ig_intr_md.ingress_port", STORAGE_SERVER_PORT)])],
                [self.table.make_data([gc.DataTuple("nf_dev", STORAGE_SERVER_NF_DEV)], "Ingress.set_ingress_dev")],
            )
        except:
            self.table.entry_mod(
                self.target,
                [self.table.make_key([gc.KeyTuple("ig_intr_md.ingress_port", CLIENT_PORT)])],
                [self.table.make_data([gc.DataTuple("nf_dev", CLIENT_NF_DEV)], "Ingress.set_ingress_dev")],
            )

            self.table.entry_mod(
                self.target,
                [self.table.make_key([gc.KeyTuple("ig_intr_md.ingress_port", STORAGE_SERVER_PORT)])],
                [self.table.make_data([gc.DataTuple("nf_dev", STORAGE_SERVER_NF_DEV)], "Ingress.set_ingress_dev")],
            )


class ForwardNFDev(Table):
    def __init__(self, bfrt_info):
        super().__init__(bfrt_info, "Ingress.forward_nf_dev")

    def setup(self):
        try:
            self.table.entry_add(
                self.target,
                [self.table.make_key([gc.KeyTuple("nf_dev", CLIENT_NF_DEV)])],
                [self.table.make_data([gc.DataTuple("port", CLIENT_PORT)], "Ingress.fwd")],
            )

            self.table.entry_add(
                self.target,
                [self.table.make_key([gc.KeyTuple("nf_dev", STORAGE_SERVER_NF_DEV)])],
                [self.table.make_data([gc.DataTuple("port", STORAGE_SERVER_PORT)], "Ingress.fwd")],
            )
        except:
            self.table.entry_mod(
                self.target,
                [self.table.make_key([gc.KeyTuple("nf_dev", CLIENT_NF_DEV)])],
                [self.table.make_data([gc.DataTuple("port", CLIENT_PORT)], "Ingress.fwd")],
            )

            self.table.entry_mod(
                self.target,
                [self.table.make_key([gc.KeyTuple("nf_dev", STORAGE_SERVER_NF_DEV)])],
                [self.table.make_data([gc.DataTuple("port", STORAGE_SERVER_PORT)], "Ingress.fwd")],
            )


class CuckooHashTable:
    def __init__(self, bfrt_info):
        self.reg_k_1 = Register(bfrt_info, "Ingress.cuckoo_hash_table.reg_k_1")
        self.reg_v_1 = Register(bfrt_info, "Ingress.cuckoo_hash_table.reg_v_1")
        self.reg_ts_1 = Register(bfrt_info, "Ingress.cuckoo_hash_table.reg_ts_1")

        self.reg_k_2 = Register(bfrt_info, "Ingress.cuckoo_hash_table.reg_k_2")
        self.reg_v_2 = Register(bfrt_info, "Ingress.cuckoo_hash_table.reg_v_2")
        self.reg_ts_2 = Register(bfrt_info, "Ingress.cuckoo_hash_table.reg_ts_2")

    def clear(self):
        self.reg_k_1.clear()
        self.reg_v_1.clear()
        self.reg_ts_1.clear()

        self.reg_k_2.clear()
        self.reg_v_2.clear()
        self.reg_ts_2.clear()

    def read(self, index: int):
        key1 = self.reg_k_1.read(index)
        key2 = self.reg_k_2.read(index)

        value1 = self.reg_v_1.read(index)
        value2 = self.reg_v_2.read(index)

        ts1 = self.reg_ts_1.read(index)
        ts2 = self.reg_ts_2.read(index)

        return (key1, value1, ts1), (key2, value2, ts2)

    def dump(self):
        table = PrettyTable()

        print("====================== Cuckoo Hash Tables ======================")
        table.field_names = ["Index", "Table1", "Table2"]

        reg_k_1_keys = self.reg_k_1.get_all()
        reg_k_2_keys = self.reg_k_2.get_all()

        reg_v_1_keys = self.reg_v_1.get_all()
        reg_v_2_keys = self.reg_v_2.get_all()

        reg_ts_1_keys = self.reg_ts_1.get_all()
        reg_ts_2_keys = self.reg_ts_2.get_all()

        last = None
        set_three_dots = False

        for index in reg_k_1_keys.keys():
            k_1 = reg_k_1_keys[index]
            v_1 = reg_v_1_keys[index]
            ts_1 = reg_ts_1_keys[index]

            k_2 = reg_k_2_keys[index]
            v_2 = reg_v_2_keys[index]
            ts_2 = reg_ts_2_keys[index]

            line = (k_1, v_1, ts_1, k_2, v_2, ts_2)

            entry_1 = "\n".join([f"key={k_1:08x}", f"val={v_1:08x}", f"ts={ts_1}"])
            entry_2 = "\n".join([f"key={k_2:08x}", f"val={v_2:08x}", f"ts={ts_2}"])

            if last != line:
                table.add_row([hex(index), entry_1, entry_2])
                set_three_dots = False
            elif last == line and not set_three_dots:
                table.add_row(["...", "...", "..."])
                set_three_dots = True

            last = line

        print(table)


class CuckooHashBloomFilter:
    def __init__(self, bfrt_info):
        self.swap_transient = Register(bfrt_info, "Ingress.cuckoo_bloom_filter.swap_transient")
        self.swapped_transient = Register(bfrt_info, "Ingress.cuckoo_bloom_filter.swapped_transient")

    def clear(self):
        self.swap_transient.clear()
        self.swapped_transient.clear()

    def read(self, key: int):
        index = hash(key)
        swap = self.swap_transient.read(index)
        swapped = self.swapped_transient.read(index)
        return swap, swapped

    def dump(self):
        table = PrettyTable()

        print("====================== Cuckoo Hash Bloom Filter ======================")
        table.field_names = ["Index", "Swap", "Swapped"]

        swap_keys = self.swap_transient.get_all()
        swapped_keys = self.swapped_transient.get_all()

        last = None
        set_three_dots = False

        for index in swap_keys.keys():
            swap = swap_keys[index]
            swapped = swapped_keys[index]

            if last != (swap, swapped):
                table.add_row([hex(index), swap, swapped])
                set_three_dots = False
            elif last == (swap, swapped) and not set_three_dots:
                table.add_row(["...", "...", "..."])
                set_three_dots = True

            last = (swap, swapped)

        print(table)


@dataclass(frozen=True)
class CuckooHashTableEntry:
    key: int
    val: int
    ts: int


class CuckooHashSimulator:
    def __init__(self):
        self.table_1: dict[int, CuckooHashTableEntry] = {}
        self.table_2: dict[int, CuckooHashTableEntry] = {}
        self.keys_1 = set()
        self.keys_2 = set()
        self.hashes_1 = set()
        self.hashes_2 = set()
        self.entries = set()

    def assert_invarians(self):
        assert self.table_1.keys() == self.hashes_1, f"Table 1 keys {self.table_1.keys()} != hashes {self.hashes_1}"
        assert self.table_2.keys() == self.hashes_2, f"Table 2 keys {self.table_2.keys()} != hashes {self.hashes_2}"

        for key in self.keys_1:
            assert key not in self.keys_2, f"Key {key:08x} found in both tables"
        for key in self.keys_2:
            assert key not in self.keys_1, f"Key {key:08x} found in both tables"

        for key in self.entries:
            h1 = hash1(key)
            h2 = hash2(key)

            in_t1 = h1 in self.table_1 and self.table_1[h1].key == key
            in_t2 = h2 in self.table_2 and self.table_2[h2].key == key

            assert in_t1 or in_t2, f"Element {key:x} not found in any table"
            assert not (in_t1 and in_t2), f"Element {key:x} found in both tables"

    def get_usage(self):
        table_1_usage = len(self.table_1) / CUCKOO_CAPACITY
        table_2_usage = len(self.table_2) / CUCKOO_CAPACITY
        return table_1_usage, table_2_usage

    def add(self, entry: CuckooHashTableEntry) -> list[CuckooHashTableEntry]:
        original_entry = copy.deepcopy(entry)
        current_entry = copy.deepcopy(entry)

        keys = [self.keys_1, self.keys_2]
        hashes = [self.hashes_1, self.hashes_2]
        tables = [self.table_1, self.table_2]
        hash_functions = [hash1, hash2]

        now = entry.ts

        new_entries = []

        i = 0
        evictions = 0
        while True:
            if i == 0 and current_entry.key == original_entry.key and evictions > 0:
                # print(f"Loop detected!")
                return []

            new_entries.append(copy.deepcopy(current_entry))

            h = hash_functions[i](current_entry.key)

            evicted_entry = copy.deepcopy(tables[i].get(h, None))
            tables[i][h] = copy.deepcopy(current_entry)

            keys[i].add(current_entry.key)
            hashes[i].add(h)

            if evicted_entry is None or now - evicted_entry.ts >= EXPIRATION_TIME:
                break

            keys[i].remove(evicted_entry.key)
            current_entry = evicted_entry

            i = (i + 1) % 2
            evictions += 1

        self.entries.add(entry.key)

        return new_entries

    def get_next_key(self, ts: int, target_evictions: int) -> int:
        table_1 = copy.deepcopy(self.table_1)
        table_2 = copy.deepcopy(self.table_2)
        keys_1 = copy.deepcopy(self.keys_1)
        keys_2 = copy.deepcopy(self.keys_2)
        hashes_1 = copy.deepcopy(self.hashes_1)
        hashes_2 = copy.deepcopy(self.hashes_2)
        entries = copy.deepcopy(self.entries)

        evictions = -1
        while evictions != target_evictions:
            key = generate_random_key()
            new_entries = self.add(CuckooHashTableEntry(key=key, val=0, ts=ts))
            evictions = len(new_entries) - 1

            # print(f"Trying key {key:08x} -> evictions {evictions:3} (target {target_evictions:3})", end="\r")

            self.table_1 = copy.deepcopy(table_1)
            self.table_2 = copy.deepcopy(table_2)
            self.keys_1 = copy.deepcopy(keys_1)
            self.keys_2 = copy.deepcopy(keys_2)
            self.hashes_1 = copy.deepcopy(hashes_1)
            self.hashes_2 = copy.deepcopy(hashes_2)
            self.entries = copy.deepcopy(entries)

        # print()
        return key

    def dump(self):
        usage = self.get_usage()
        table1_usage = int(100 * usage[0])
        table2_usage = int(100 * usage[1])

        table = PrettyTable()
        table.field_names = ["Index", f"Table 1 ({table1_usage}%)", f"Table 2 ({table2_usage}%)"]

        indexes = sorted(self.table_1.keys() | self.table_2.keys())

        for i, h in enumerate(indexes):
            if i > 0 and indexes[i] > indexes[i - 1] + 1:
                table.add_row(["...", "...", "..."])

            entry_1 = self.table_1.get(h, None)
            entry_2 = self.table_2.get(h, None)

            if entry_1 is not None:
                entry_1_str = f"{entry_1.key:08x} (ts={entry_1.ts})"
            else:
                entry_1_str = ""

            if entry_2 is not None:
                entry_2_str = f"{entry_2.key:08x} (ts={entry_2.ts})"
            else:
                entry_2_str = ""

            table.add_row([hex(h), entry_1_str, entry_2_str])

        print(table)


class EmptyCuckooFailedLookup(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

        ingress_port_to_nf_dev = IngressPortToNFDev(self.bfrt_info)
        forward_nf_dev = ForwardNFDev(self.bfrt_info)

        ingress_port_to_nf_dev.setup()
        forward_nf_dev.setup()

    def runTest(self):
        key = generate_random_key()
        kvs = KVS(ts=0, op=KVS_OP_GET, key=key)

        send_and_expect(self, kvs, STORAGE_SERVER_PORT)


class SingleInsertion(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

        self.ingress_port_to_nf_dev = IngressPortToNFDev(self.bfrt_info)
        self.forward_nf_dev = ForwardNFDev(self.bfrt_info)
        self.cuckoo_hash_table = CuckooHashTable(self.bfrt_info)
        self.cuckoo_hash_bloom_filter = CuckooHashBloomFilter(self.bfrt_info)

        self.ingress_port_to_nf_dev.setup()
        self.forward_nf_dev.setup()

        self.time = EXPIRATION_TIME

    def runTest(self):
        self.time += 1

        key = generate_random_key()
        val = generate_random_value()
        kvs_put = KVS(ts=self.time, op=KVS_OP_PUT, key=key, val=val)

        send_and_expect(self, kvs_put, CLIENT_PORT)

        h1 = hash1(key)
        (key1, val1, ts1), (key2, val2, ts2) = self.cuckoo_hash_table.read(h1)

        assert key1 == key, f"Expected {key1:08x} == {key:08x}"
        assert val1 == val, f"Expected {val1:08x} == {val:08x}"
        assert ts1 == self.time, f"Expected {ts1} == {self.time}"

        assert key2 == 0, f"Expected {key2:08x} == 0"
        assert val2 == 0, f"Expected {val2:08x} == 0"
        assert ts2 == 0, f"Expected {ts2} == 0"

        swap, swapped = self.cuckoo_hash_bloom_filter.read(key)

        assert swap == 1, f"Expected swap 1, but got {swap}"
        assert swapped == 1, f"Expected swapped 1, but got {swapped}"

        self.time += 1
        kvs_get = KVS(ts=self.time, op=KVS_OP_GET, key=key)

        kvs_res = send_and_expect(self, kvs_get, CLIENT_PORT)

        assert kvs_res.op == KVS_OP_GET, f"Expected GET operation, but got {kvs_res.op}"
        assert kvs_res.key == key, f"Expected key {key:08x}, but got {kvs_res.key:08x}"
        assert kvs_res.val == val, f"Expected value {val:08x}, but got {kvs_res.val:08x}"
        assert kvs_res.status == KVS_STATUS_OK, f"Expected status OK, but got {kvs_res.status}"

    def tearDown(self):
        self.interface.clear_all_tables()
        super().tearDown()


class MultipleInsertionsOnTheFirstTable(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

        self.ingress_port_to_nf_dev = IngressPortToNFDev(self.bfrt_info)
        self.forward_nf_dev = ForwardNFDev(self.bfrt_info)
        self.cuckoo_hash_table = CuckooHashTable(self.bfrt_info)
        self.cuckoo_hash_bloom_filter = CuckooHashBloomFilter(self.bfrt_info)

        self.ingress_port_to_nf_dev.setup()
        self.forward_nf_dev.setup()

        self.time = EXPIRATION_TIME

    def runTest(self):
        hashes = set()

        i = 0
        n = 120
        while i < n:
            self.time += 1

            key = generate_random_key()
            val = generate_random_value()
            kvs_put = KVS(ts=self.time, op=KVS_OP_PUT, key=key, val=val)

            h = hash1(key)
            if h in hashes:
                continue
            hashes.add(h)
            i += 1

            print(f"[MultipleInsertionsOnTheFirstTable][{i}/{n}] h={h:08x} key={key:08x} val={val:08x}", end="\r")

            send_and_expect(self, kvs_put, CLIENT_PORT)

            h1 = hash1(key)
            (key1, val1, ts1), (key2, val2, ts2) = self.cuckoo_hash_table.read(h1)

            assert key1 == key, f"Expected {key1:08x} == {key:08x}"
            assert val1 == val, f"Expected {val1:08x} == {val:08x}"
            assert ts1 == self.time, f"Expected {ts1} == {self.time}"

            assert key2 == 0, f"Expected {key2:08x} == 0"
            assert val2 == 0, f"Expected {val2:08x} == 0"
            assert ts2 == 0, f"Expected {ts2} == 0"

            swap, swapped = self.cuckoo_hash_bloom_filter.read(key)

            assert swap == 1, f"Expected swap 1, but got {swap}"
            assert swapped == 1, f"Expected swapped 1, but got {swapped}"

            self.time += 1
            kvs_get = KVS(ts=self.time, op=KVS_OP_GET, key=key)

            kvs_res = send_and_expect(self, kvs_get, CLIENT_PORT)

            assert kvs_res.op == KVS_OP_GET, f"Expected GET operation, but got {kvs_res.op}"
            assert kvs_res.key == key, f"Expected key {key:08x}, but got {kvs_res.key:08x}"
            assert kvs_res.val == val, f"Expected value {val:08x}, but got {kvs_res.val:08x}"
            assert kvs_res.status == KVS_STATUS_OK, f"Expected status OK, but got {kvs_res.status}"
        print()

    def tearDown(self):
        self.interface.clear_all_tables()
        super().tearDown()


class InsertionsOnTheSecondTable(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

        self.ingress_port_to_nf_dev = IngressPortToNFDev(self.bfrt_info)
        self.forward_nf_dev = ForwardNFDev(self.bfrt_info)
        self.cuckoo_hash_table = CuckooHashTable(self.bfrt_info)
        self.cuckoo_hash_bloom_filter = CuckooHashBloomFilter(self.bfrt_info)
        self.cuckoo_hash_table_simulator = CuckooHashSimulator()

        self.ingress_port_to_nf_dev.setup()
        self.forward_nf_dev.setup()

        self.time = EXPIRATION_TIME + 1

        n = 120
        for i in range(n):
            key = self.cuckoo_hash_table_simulator.get_next_key(self.time, target_evictions=0)
            val = generate_random_value()

            print(f"[InsertionsOnTheSecondTable setup][{i+1}/{n}] key={key:08x} val={val:08x}", end="\r")

            self.cuckoo_hash_table_simulator.add(CuckooHashTableEntry(key=key, val=val, ts=self.time))
            kvs_put = KVS(ts=self.time, op=KVS_OP_PUT, key=key, val=val)
            send_and_expect(self, kvs_put, CLIENT_PORT)
        print()

    def runTest(self):
        self.time += 10

        n = 50
        for i in range(n):
            key = self.cuckoo_hash_table_simulator.get_next_key(self.time, target_evictions=1)
            val = generate_random_value()

            kvs_put = KVS(ts=self.time, op=KVS_OP_PUT, key=key, val=val)

            h1 = hash1(key)

            (evicted_key, evicted_val, evicted_ts), _ = self.cuckoo_hash_table.read(h1)

            self.cuckoo_hash_table_simulator.add(CuckooHashTableEntry(key=key, val=val, ts=self.time))
            send_and_expect(self, kvs_put, CLIENT_PORT)

            (key1, val1, ts1), _ = self.cuckoo_hash_table.read(h1)
            _, (key2, val2, ts2) = self.cuckoo_hash_table.read(hash2(evicted_key))

            print(f"[InsertionsOnTheSecondTable][{i+1}/{n}] h1={h1:08x} key={key:08x} val={val:08x}", end="\r")

            assert key1 == key, f"Expected {key1:08x} == {key:08x}"
            assert val1 == val, f"Expected {val1:08x} == {val:08x}"
            assert ts1 == self.time, f"Expected {ts1} == {self.time}"

            assert key2 == evicted_key, f"Expected {key2:08x} == {evicted_key:08x}"
            assert val2 == evicted_val, f"Expected {val2:08x} == {evicted_val:08x}"
            assert ts2 == evicted_ts, f"Expected {ts2} == {evicted_ts}"

            swap, swapped = self.cuckoo_hash_bloom_filter.read(key)

            assert swap == 2, f"Expected swap 2, but got {swap}"
            assert swapped == 2, f"Expected swapped 2, but got {swapped}"

            kvs_get = KVS(ts=self.time, op=KVS_OP_GET, key=key)
            kvs_res = send_and_expect(self, kvs_get, CLIENT_PORT)

            assert kvs_res.op == KVS_OP_GET, f"Expected GET operation, but got {kvs_res.op}"
            assert kvs_res.key == key, f"Expected key {key}, but got {kvs_res.key}"
            assert kvs_res.val == val, f"Expected value {val}, but got {kvs_res.val}"
            assert kvs_res.status == KVS_STATUS_OK, f"Expected status OK, but got {kvs_res.status}"
        print()

    def tearDown(self):
        self.interface.clear_all_tables()
        super().tearDown()


class Random(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

        self.ingress_port_to_nf_dev = IngressPortToNFDev(self.bfrt_info)
        self.forward_nf_dev = ForwardNFDev(self.bfrt_info)
        self.cuckoo_hash_table = CuckooHashTable(self.bfrt_info)
        self.cuckoo_hash_bloom_filter = CuckooHashBloomFilter(self.bfrt_info)
        self.cuckoo_hash_table_simulator = CuckooHashSimulator()

        self.ingress_port_to_nf_dev.setup()
        self.forward_nf_dev.setup()

        self.time = EXPIRATION_TIME + 1

    def runTest(self):
        self.time += 10

        n = CUCKOO_CAPACITY
        for i in range(n):
            key = generate_random_key()
            val = generate_random_value()

            new_entries = self.cuckoo_hash_table_simulator.add(CuckooHashTableEntry(key=key, val=val, ts=self.time))
            evictions = len(new_entries) - 1

            print(f"[Random][{i+1}/{n}] key={key:08x} -> evictions {evictions:3}", end="\r")

            expected_port = CLIENT_PORT if evictions >= 0 and evictions < MAX_LOOPS else STORAGE_SERVER_PORT

            kvs_put = KVS(ts=self.time, op=KVS_OP_PUT, key=key, val=val)
            kvs_put_res = send_and_expect(self, kvs_put, expected_port)

            if expected_port != CLIENT_PORT:
                continue

            assert kvs_put_res.op == KVS_OP_PUT, f"Expected PUT operation, but got {kvs_put_res.op}"
            assert kvs_put_res.key == key, f"Expected key {key}, but got {kvs_put_res.key}"
            assert kvs_put_res.val == val, f"Expected value {val}, but got {kvs_put_res.val}"

            assert kvs_put_res.status == KVS_STATUS_OK, f"Expected status OK, but got {kvs_put_res.status}"

            kvs_get = KVS(ts=self.time, op=KVS_OP_GET, key=key)
            kvs_get_res = send_and_expect(self, kvs_get, CLIENT_PORT)

            assert kvs_get_res.op == KVS_OP_GET, f"Expected GET operation, but got {kvs_get_res.op}"
            assert kvs_get_res.key == key, f"Expected key {key}, but got {kvs_get_res.key}"
            assert kvs_get_res.val == val, f"Expected value {val}, but got {kvs_get_res.val}"
            assert kvs_get_res.status == KVS_STATUS_OK, f"Expected status OK, but got {kvs_get_res.status}"
        print()

    def tearDown(self):
        self.interface.clear_all_tables()
        super().tearDown()


class ExpiringEntries(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

        self.ingress_port_to_nf_dev = IngressPortToNFDev(self.bfrt_info)
        self.forward_nf_dev = ForwardNFDev(self.bfrt_info)
        self.cuckoo_hash_table = CuckooHashTable(self.bfrt_info)
        self.cuckoo_hash_bloom_filter = CuckooHashBloomFilter(self.bfrt_info)
        self.cuckoo_hash_table_simulator = CuckooHashSimulator()

        self.ingress_port_to_nf_dev.setup()
        self.forward_nf_dev.setup()

        self.time = EXPIRATION_TIME + 1

        n = CUCKOO_CAPACITY
        for i in range(n):
            key = generate_random_key()
            val = generate_random_value()

            new_entries = self.cuckoo_hash_table_simulator.add(CuckooHashTableEntry(key=key, val=val, ts=self.time))
            evictions = len(new_entries) - 1

            print(f"[ExpiringEntries setup][{i+1}/{n}] key={key:08x} -> evictions {evictions:3}", end="\r")

            if evictions < 0 or evictions >= MAX_LOOPS:
                continue

            kvs_put = KVS(ts=self.time, op=KVS_OP_PUT, key=key, val=val)
            send_and_expect(self, kvs_put, CLIENT_PORT)

        print()

    def runTest(self):
        self.time += EXPIRATION_TIME * 2

        n = 120
        i = 0
        while i < n:
            key = generate_random_key()
            val = generate_random_value()
            h = hash1(key)

            if h not in self.cuckoo_hash_table_simulator.table_1:
                continue

            new_entries = self.cuckoo_hash_table_simulator.add(CuckooHashTableEntry(key=key, val=val, ts=self.time))
            evictions = len(new_entries) - 1

            print(f"[ExpiringEntries][{i+1}/{n}] key={key:08x} -> evictions {evictions:3}", end="\r")

            if evictions < 0 or evictions >= MAX_LOOPS:
                continue

            kvs_put = KVS(ts=self.time, op=KVS_OP_PUT, key=key, val=val)
            kvs_put_res = send_and_expect(self, kvs_put, CLIENT_PORT)

            assert kvs_put_res.op == KVS_OP_PUT, f"Expected PUT operation, but got {kvs_put_res.op}"
            assert kvs_put_res.key == key, f"Expected key {key}, but got {kvs_put_res.key}"
            assert kvs_put_res.val == val, f"Expected value {val}, but got {kvs_put_res.val}"

            assert kvs_put_res.status == KVS_STATUS_OK, f"Expected status OK, but got {kvs_put_res.status}"

            kvs_get = KVS(ts=self.time, op=KVS_OP_GET, key=key)
            kvs_get_res = send_and_expect(self, kvs_get, CLIENT_PORT)

            assert kvs_get_res.op == KVS_OP_GET, f"Expected GET operation, but got {kvs_get_res.op}"
            assert kvs_get_res.key == key, f"Expected key {key}, but got {kvs_get_res.key}"
            assert kvs_get_res.val == val, f"Expected value {val}, but got {kvs_get_res.val}"
            assert kvs_get_res.status == KVS_STATUS_OK, f"Expected status OK, but got {kvs_get_res.status}"

            (key1, val1, ts1), _ = self.cuckoo_hash_table.read(h)

            assert key1 == key, f"Expected {key1:08x} == {key:08x}"
            assert val1 == val, f"Expected {val1:08x} == {val:08x}"
            assert ts1 == self.time, f"Expected {ts1} == {self.time}"

            i += 1
        print()

    def tearDown(self):
        self.interface.clear_all_tables()
        super().tearDown()


class StorageServerAnswersShouldBeSentToClient(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

        ingress_port_to_nf_dev = IngressPortToNFDev(self.bfrt_info)
        forward_nf_dev = ForwardNFDev(self.bfrt_info)

        ingress_port_to_nf_dev.setup()
        forward_nf_dev.setup()

    def runTest(self):
        key = generate_random_key()
        val = generate_random_value()
        kvs = KVS(ts=0, op=KVS_OP_GET, key=key, val=val, status=KVS_STATUS_OK, client_port=CLIENT_NF_DEV)

        send_and_expect(self, kvs, CLIENT_PORT, send_to_port=STORAGE_SERVER_PORT)
