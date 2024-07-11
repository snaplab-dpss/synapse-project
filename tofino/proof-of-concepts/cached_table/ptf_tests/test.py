#!/usr/bin/env python3

import random
import time
import math
import struct
import zlib

import bfrt_grpc.client as gc

import ptf.testutils as testutils

from bfruntime_client_base_tests import BfRuntimeTest
from p4testutils.misc_utils import *
from ptf.testutils import *

import scapy.layers.l2
import scapy.packet
import scapy.fields

PROGRAM = 'cached_table'

OP_READ  = 1
OP_WRITE = 2
OP_COND_WRITE = 3
OP_DELETE = 4
KEY_FROM_VALUE = 5

KEY_SIZE_BITS = 32
VALUE_SIZE_BITS = 32
HASH_SIZE_BITS = 16
EXPIRATION_TIME_SEC = 5

ITERATIONS_PER_TEST = 1000
CACHE_CAPACITY = 1 << HASH_SIZE_BITS

assert test_param_get('arch') in [ 'tofino', 'tofino2' ]
if test_param_get('arch') == 'tofino':
    IN_PORT = 0
    OUT_PORT = 1
else:
    IN_PORT = 8
    OUT_PORT = 9

def hash_key(key):
    return zlib.crc32(struct.pack("!I", key)) & ((1 << HASH_SIZE_BITS) - 1)

def custom_random(min_bits: int, max_bits: int, forbidden: list[int] = []):
    assert min_bits >= 1
    while True:
        bits = random.randint(min_bits, max_bits - 1)
        value = random.randint(0, (1<<bits)-1)
        if value not in forbidden:
            break
    return value

def random_unique_key(prev_keys: list[int] = [], prev_hashes: list[int] = []):
    key = None

    while True:
        key = custom_random(1, KEY_SIZE_BITS, prev_keys)
        hash = hash_key(key)

        if hash not in prev_hashes:
            break

    return key

def random_value(forbidden: list[int] = []):
    return custom_random(1, VALUE_SIZE_BITS, forbidden)

def random_collision_key(prev_keys: list[int] = [], prev_hashes: list[int] = [], forbidden_hashes: list[int] = []):
    key = None

    while True:
        key = custom_random(1, KEY_SIZE_BITS, prev_keys)
        hash = hash_key(key)

        if hash in prev_hashes and hash not in forbidden_hashes:
            break

    return key

class App(scapy.packet.Packet):
    name = "App"
    fields_desc = [
        scapy.fields.BitField("op", 0, 8),
        scapy.fields.BitField("key", 0, KEY_SIZE_BITS),
        scapy.fields.BitField("hash", 0, HASH_SIZE_BITS),
        scapy.fields.BitField("value", 0, VALUE_SIZE_BITS),
        scapy.fields.BitField("hit", 0, 8),
    ]

    def mysummary(self):
        return self.sprintf("App (op=%App.op%,key=%App.key%,hash=%App.hash%,value=%App.value%,hit=%App.hit%)")

scapy.packet.bind_layers(scapy.layers.l2.Ether, App)

def send_and_get_results(bfruntimetest, op, key, value=None) -> tuple[int,int,bool]:
    ether = scapy.layers.l2.Ether(src='00:11:22:33:44:55', dst='00:11:22:33:44:55')
    app = App(op=op, key=key, hash=0, value=value, hit=0)
    ipkt = ether / app

    testutils.send_packet(bfruntimetest, IN_PORT, ipkt)
    _, _, obytes, _ = testutils.dp_poll(bfruntimetest, 0, OUT_PORT, timeout=2)
    oether = scapy.layers.l2.Ether(obytes)
    oapp = App(bytes(oether[1]))

    return oapp.hash, oapp.value, oapp.hit != 0

class Table(object):
    def __init__(self, bfrt_info, target, control_name, name):
        self.bfrt_info = bfrt_info
        self.target = target
        self.control_name = control_name
        self.name = name

        # child clases must set table
        self.table = bfrt_info.table_get(f"{self.control_name}.{self.name}")

    def clear(self):
        # get all keys in table
        resp = self.table.entry_get(self.target, [], {"from_hw": False})

        # delete all keys in table
        for _, key in resp:
            if key:
                self.table.entry_del(self.target, [key])

class Register(Table):
    def __init__(self, bfrt_info, target, control_name, name):
        super().__init__(bfrt_info, target, control_name, name)
    
    def reset(self):
        total_keys = 1 << HASH_SIZE_BITS
        for key in range(total_keys):
            self.set_value(key, 0)

    def set_value(self, index, new_value):
        # print(f"reg[{index}] <- {new_value}")

        self.table.entry_add(
            self.target,
            [
                self.table.make_key([ gc.KeyTuple("$REGISTER_INDEX", index) ])
            ],
            [
                self.table.make_data(
                    [ gc.DataTuple(f"{self.control_name}.{self.name}.f1", new_value) ]
                )
            ])
    
        assert(self.get_value(index) == new_value)
    
    def get_value(self, index):
        resp = self.table.entry_get(
            self.target,
            [
                self.table.make_key([ gc.KeyTuple("$REGISTER_INDEX", index) ])
            ],
            { "from_hw": True }
        )

        data = next(resp)[0].to_dict()
        return data[f"{self.control_name}.{self.name}.f1"][0]

class Map(Table):
    def __init__(self, bfrt_info, target):
        super().__init__(bfrt_info, target, "SwitchIngress.cached_table", "kv_map")

    def add_entry(self, key, value):
        action = f"{self.control_name}.kv_map_get_value"

        print(f"kv_map[{key}] <- {value}")

        self.table.entry_add(
            self.target,
            [
                self.table.make_key([ gc.KeyTuple("k", key) ])
            ],
            [
                self.table.make_data([ gc.DataTuple("value", value) ], action)
            ])

class Expirator(Register):
    def __init__(self, bfrt_info, target):
        super().__init__(bfrt_info, target, "SwitchIngress.cached_table", "expirator")

class Keys(Register):
    def __init__(self, bfrt_info, target):
        super().__init__(bfrt_info, target, "SwitchIngress.cached_table", "keys")

class Values(Register):
    def __init__(self, bfrt_info, target):
        super().__init__(bfrt_info, target, "SwitchIngress.cached_table", "values")

# ====================================================================================
#
#                                      TESTS
#
# ====================================================================================

class ReadEmpty(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

    def runTest(self):
        used_keys = []
        used_hashes = []

        for _ in range(ITERATIONS_PER_TEST):
            key = random_unique_key(used_keys, used_hashes)
            hash, value, hit = send_and_get_results(self, OP_READ, key)

            print(f"key {key:10d} hash 0x{hash:04x} value {value:10d} hit {hit}")
            
            assert hash_key(key) == hash
            assert not hit
            
            used_keys.append(key)
            used_hashes.append(hash)
    
    def tearDown(self):
        self.interface.clear_all_tables()
        BfRuntimeTest.tearDown(self)

class TableHit(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.map = Map(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        used_hashes = []

        for _ in range(ITERATIONS_PER_TEST):
            key = random_unique_key(used_keys, used_hashes)
            exp_value = random_value()

            self.map.add_entry(key, exp_value)
            hash, value, hit = send_and_get_results(self, OP_READ, key)

            print(f"key {key:10d} hash 0x{hash:04x} value {value:10d} hit {hit}")
            
            assert hash_key(key) == hash
            assert hit
            assert value == exp_value
            
            used_keys.append(key)
            used_hashes.append(hash)
    
    def tearDown(self):
        self.interface.clear_all_tables()
        BfRuntimeTest.tearDown(self)

class CacheWrite(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.expirator = Expirator(self.bfrt_info, self.target)
        self.keys = Keys(self.bfrt_info, self.target)
        self.values = Values(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        used_hashes = []

        for _ in range(ITERATIONS_PER_TEST):
            key = random_unique_key(used_keys, used_hashes)
            exp_value = random_value()
            expected_hash = hash_key(key)

            ts1 = self.expirator.get_value(expected_hash)
            hash, value, hit = send_and_get_results(self, OP_WRITE, key, exp_value)
            ts2 = self.expirator.get_value(expected_hash)

            print(f"key {key:10d} hash 0x{hash:04x} value {value:10d} hit {hit}")
            print(f"ts1 {ts1:10d} ts2 {ts2:10d}")

            k = self.keys.get_value(expected_hash)
            v = self.values.get_value(expected_hash)
            
            assert expected_hash == hash
            assert hit
            assert ts1 < ts2
            assert k == key
            assert v == value
            assert value == exp_value
            
            used_keys.append(key)
            used_hashes.append(hash)
    
    def tearDown(self):
        self.interface.clear_all_tables()
        BfRuntimeTest.tearDown(self)

class CacheReadAfterWrite(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.expirator = Expirator(self.bfrt_info, self.target)
        self.keys = Keys(self.bfrt_info, self.target)
        self.values = Values(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        used_hashes = []

        for _ in range(ITERATIONS_PER_TEST):
            key = random_unique_key(used_keys, used_hashes)
            exp_value = random_value()
            expected_hash = hash_key(key)

            ts1 = self.expirator.get_value(expected_hash)
            send_and_get_results(self, OP_WRITE, key, exp_value)
            ts2 = self.expirator.get_value(expected_hash)
            hash, value, hit = send_and_get_results(self, OP_READ, key)
            ts3 = self.expirator.get_value(expected_hash)

            print(f"key {key:10d} hash 0x{hash:04x} value {value:10d} hit {hit}")
            print(f"ts1 {ts1:10d} ts2 {ts2:10d}")

            k = self.keys.get_value(expected_hash)
            v = self.values.get_value(expected_hash)
            
            assert expected_hash == hash
            assert hit
            assert ts1 < ts2
            assert ts2 < ts3
            assert k == key
            assert v == value
            assert value == exp_value
            
            used_keys.append(key)
            used_hashes.append(hash)
    
    def tearDown(self):
        self.interface.clear_all_tables()
        BfRuntimeTest.tearDown(self)

class CacheCollisionRejuvenation(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.expirator = Expirator(self.bfrt_info, self.target)
        self.keys = Keys(self.bfrt_info, self.target)

    # WARNING
    # It's possible to fail this test!
    # The allocated flows on the cache might expire during our collision phase.
    # In that case, we wouldn't rejuvenate them.
    # That's why I used a low number of test iterations here
    # I didn't want to increase the expiration time, as it's hardcoded on the p4 code.
    def runTest(self):
        used_keys = []
        used_hashes = []

        cache_occupancy = 100

        for _ in range(cache_occupancy):
            key = random_unique_key(used_keys, used_hashes)
            exp_value = random_value()
            hash, _, _ = send_and_get_results(self, OP_WRITE, key, exp_value)
            used_keys.append(key)
            used_hashes.append(hash)
        
        for _ in range(cache_occupancy):
            key = random_collision_key(used_keys, used_hashes)
            expected_hash = hash_key(key)

            ts1 = self.expirator.get_value(expected_hash)
            hash, value, hit = send_and_get_results(self, OP_READ, key)
            ts2 = self.expirator.get_value(expected_hash)

            print(f"key {key:10d} hash 0x{hash:04x} value {value:10d} hit {hit}")
            print(f"ts1 {ts1:10d} ts2 {ts2:10d}")

            k = self.keys.get_value(expected_hash)
            
            assert expected_hash == hash
            assert not hit
            assert k != key
            
            # By design, this cache rejuvenates on a cache miss due to a collision.
            assert ts1 < ts2
            
            used_keys.append(key)
            used_hashes.append(hash)
    
    def tearDown(self):
        self.interface.clear_all_tables()
        BfRuntimeTest.tearDown(self)

class CacheEviction(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.expirator = Expirator(self.bfrt_info, self.target)
        self.keys = Keys(self.bfrt_info, self.target)
        self.values = Values(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        used_hashes = []

        cache_occupancy = 10

        for _ in range(cache_occupancy):
            key = random_unique_key(used_keys, used_hashes)
            exp_value = random_value()
            hash, _, _ = send_and_get_results(self, OP_WRITE, key, exp_value)
            used_keys.append(key)
            used_hashes.append(hash)

        print(f"Waiting {EXPIRATION_TIME_SEC} seconds to let the entries expire...")
        time.sleep(EXPIRATION_TIME_SEC)
        
        collided_hashes = []

        for _ in range(cache_occupancy):
            key = random_collision_key(used_keys, used_hashes, collided_hashes)
            expected_hash = hash_key(key)

            prev_key = self.keys.get_value(expected_hash)
            prev_value = self.values.get_value(expected_hash)

            exp_value = random_value([prev_value])

            ts1 = self.expirator.get_value(expected_hash)
            hash, value, hit = send_and_get_results(self, OP_WRITE, key, exp_value)
            ts2 = self.expirator.get_value(expected_hash)

            new_key = self.keys.get_value(expected_hash)
            new_value = self.values.get_value(expected_hash)

            print(f"key {key:10d} hash 0x{hash:04x} value {value:10d} hit {hit}")
            print(f"ts1 {ts1:10d} ts2 {ts2:10d}")
            
            assert expected_hash == hash
            assert hit
            
            assert prev_key != key
            assert prev_value != exp_value

            assert key == new_key
            assert exp_value == new_value

            assert exp_value == value
            assert ts1 < ts2
            
            used_keys.append(key)
            collided_hashes.append(hash)
    
    def tearDown(self):
        self.interface.clear_all_tables()
        BfRuntimeTest.tearDown(self)

class WriteFail(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.expirator = Expirator(self.bfrt_info, self.target)
        self.keys = Keys(self.bfrt_info, self.target)
        self.values = Values(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        used_hashes = []

        cache_occupancy = 10

        for _ in range(cache_occupancy):
            key = random_unique_key(used_keys, used_hashes)
            exp_value = random_value()
            hash, _, _ = send_and_get_results(self, OP_WRITE, key, exp_value)
            used_keys.append(key)
            used_hashes.append(hash)

        collided_hashes = []

        for _ in range(cache_occupancy):
            key = random_collision_key(used_keys, used_hashes, collided_hashes)
            expected_hash = hash_key(key)

            prev_key = self.keys.get_value(expected_hash)
            prev_value = self.values.get_value(expected_hash)

            exp_value = random_value([prev_value])

            ts1 = self.expirator.get_value(expected_hash)
            hash, value, hit = send_and_get_results(self, OP_WRITE, key, exp_value)
            ts2 = self.expirator.get_value(expected_hash)

            new_key = self.keys.get_value(expected_hash)
            new_value = self.values.get_value(expected_hash)

            print(f"key {key:10d} hash 0x{hash:04x} value {value:10d} hit {hit}")
            print(f"ts1 {ts1:10d} ts2 {ts2:10d}")
            
            assert expected_hash == hash
            assert not hit
            
            assert prev_key != key
            assert prev_value != exp_value

            assert key != new_key
            assert exp_value != new_value

            assert prev_key == new_key
            assert prev_value == new_value

            assert ts1 < ts2
            
            used_keys.append(key)
            collided_hashes.append(hash)
    
    def tearDown(self):
        self.interface.clear_all_tables()
        BfRuntimeTest.tearDown(self)
