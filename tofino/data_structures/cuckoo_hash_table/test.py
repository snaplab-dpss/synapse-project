#!/usr/bin/env python3

import struct
import zlib

import bfrt_grpc.client as gc
import ptf.testutils as testutils

from bfruntime_client_base_tests import BfRuntimeTest
from p4testutils.misc_utils import *
from ptf.testutils import *

from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP
from scapy.packet import Packet, bind_layers
from scapy import fields

from prettytable import PrettyTable

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
        return f"KVS{{ts={self.ts} op={self.op}, key={self.key:0{KVS_KEY_SIZE_BYTES}x}, val={self.val:0{KVS_VALUE_SIZE_BYTES}x}, status={self.status}, client_port={self.client_port}}}"


bind_layers(UDP, KVS)


def generate_random_key():
    return random.randint(0, 2 ** (KVS_KEY_SIZE_BYTES * 8) - 1)


def generate_random_value():
    return random.randint(0, 2 ** (KVS_VALUE_SIZE_BYTES * 8) - 1)


def generate_random_ip():
    return f"{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}"


def generate_random_port():
    return random.randint(1024, 65535)


def send_and_expect(bfruntimetest, kvs: KVS, expected_port: int) -> KVS:
    ipkt = Ether(src=SRC_MAC, dst=DST_MAC)
    ipkt /= IP(src=generate_random_ip(), dst=generate_random_ip())
    ipkt /= UDP(sport=generate_random_port(), dport=KVS_UDP_PORT)
    ipkt /= kvs

    print(f"[sent] {kvs}")

    send_packet(bfruntimetest, CLIENT_PORT, ipkt)
    _, _, obytes, _ = dp_poll(bfruntimetest, 0, expected_port, timeout=TIMEOUT_SEC)

    assert obytes is not None, f"[recv] No response from port {expected_port}"
    opkt = Ether(obytes)

    assert IP in opkt and UDP in opkt
    oapp = KVS(bytes(opkt[3]))

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


class ForwardNFDev(Table):
    def __init__(self, bfrt_info):
        super().__init__(bfrt_info, "Ingress.forward_nf_dev")

    def setup(self):
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

    def read(self, key: int):
        index1 = hash1(key)
        index2 = hash2(key)

        value1 = self.reg_v_1.read(index1)
        value2 = self.reg_v_2.read(index2)

        ts1 = self.reg_ts_1.read(index1)
        ts2 = self.reg_ts_2.read(index2)

        return (value1, ts1), (value2, ts2)


class CuckooHashBloomFilter:
    def __init__(self, bfrt_info):
        self.swap_transient = Register(bfrt_info, "Ingress.cuckoo_bloom_filter.swap_transient")
        self.swapped_transient = Register(bfrt_info, "Ingress.cuckoo_bloom_filter.swapped_transient")

    def clear(self):
        self.swap_transient.clear()
        self.swapped_transient.clear()

    def read(self, key: int):
        index = hash(key)

        print(f"Hash index: {hex(index)}")
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


# class EmptyCuckooFailedLookup(BfRuntimeTest):
#     def setUp(self):
#         client_id = 0
#         BfRuntimeTest.setUp(self, client_id, PROGRAM)
#         self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

#         ingress_port_to_nf_dev = IngressPortToNFDev(self.bfrt_info)
#         forward_nf_dev = ForwardNFDev(self.bfrt_info)

#         ingress_port_to_nf_dev.setup()
#         forward_nf_dev.setup()

#     def runTest(self):
#         key = generate_random_key()
#         kvs = KVS(ts=0, op=KVS_OP_GET, key=key)

#         send_and_expect(self, kvs, STORAGE_SERVER_PORT)


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

        self.cuckoo_hash_bloom_filter.dump()

        self.time = EXPIRATION_TIME

    def runTest(self):
        self.time += 1

        key = generate_random_key()
        val = generate_random_value()
        kvs_put = KVS(ts=self.time, op=KVS_OP_PUT, key=key, val=val)
        print(f"Hash: {hex(hash(key))}")

        send_and_expect(self, kvs_put, CLIENT_PORT)

        (val1, ts1), (val2, ts2) = self.cuckoo_hash_table.read(key)

        assert val1 == val, f"Expected value {val}, but got {val1}"
        assert val2 == 0, f"Expected value 0, but got {val2}"

        assert ts1 == self.time, f"Expected timestamp {self.time}, but got {ts1}"
        assert ts2 == 0, f"Expected timestamp 0, but got {ts2}"

        self.cuckoo_hash_bloom_filter.dump()

        swap, swapped = self.cuckoo_hash_bloom_filter.read(key)

        assert swap == 1, f"Expected swap 1, but got {swap}"
        assert swapped == 1, f"Expected swapped 1, but got {swapped}"

        self.time += 1
        kvs_get = KVS(ts=self.time, op=KVS_OP_GET, key=key)

        kvs_res = send_and_expect(self, kvs_get, CLIENT_PORT)

        assert kvs_res.op == KVS_OP_GET, f"Expected GET operation, but got {kvs_res.op}"
        assert kvs_res.key == key, f"Expected key {key}, but got {kvs_res.key}"
        assert kvs_res.val == val, f"Expected value {val}, but got {kvs_res.val}"
        assert kvs_res.status == KVS_STATUS_OK, f"Expected status OK, but got {kvs_res.status}"
