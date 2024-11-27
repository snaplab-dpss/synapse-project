#!/usr/bin/env python3

import random
import time

import bfrt_grpc.client as gc

import ptf.testutils as testutils

from bfruntime_client_base_tests import BfRuntimeTest
from p4testutils.misc_utils import *
from ptf.testutils import *

import scapy.layers.l2
import scapy.packet
import scapy.fields

PROGRAM = 'expirator'

OP_READ  = 0
OP_WRITE = 1

KEY_SIZE_BITS = 8
EXPIRATION_TIME_SEC = 5

assert test_param_get('arch') in [ 'tofino', 'tofino2' ]
if test_param_get('arch') == 'tofino':
    IN_PORT = 0
    OUT_PORT = 1
else:
    IN_PORT = 8
    OUT_PORT = 9
    
def custom_random(min_bits: int, max_bits: int, forbidden: list[int] = []):
    assert min_bits >= 1
    while True:
        bits = random.randint(min_bits, max_bits - 1)
        value = random.randint(0, (1<<bits)-1)
        if value not in forbidden:
            break
    return value

class App(scapy.packet.Packet):
    name = "App"
    fields_desc = [
        scapy.fields.BitField("key", 0, KEY_SIZE_BITS),
        scapy.fields.BitField("op", 0, 8),
        scapy.fields.BitField("valid", 0, 8),
    ]

    def mysummary(self):
        return self.sprintf("App (key=%App.key%,op=%App.op%,valid=%App.valid%)")

scapy.packet.bind_layers(scapy.layers.l2.Ether, App)

def send_and_get_validity(bfruntimetest, key, op):
    ether = scapy.layers.l2.Ether(src='00:11:22:33:44:55', dst='00:11:22:33:44:55')
    app = App(key=key, op=op, valid=0)
    ipkt = ether / app

    testutils.send_packet(bfruntimetest, IN_PORT, ipkt)
    _, _, obytes, _ = testutils.dp_poll(bfruntimetest, 0, OUT_PORT, timeout=2)
    oether = scapy.layers.l2.Ether(obytes)
    oapp = App(bytes(oether[1]))

    return oapp.valid != 0

class Table(object):
    def __init__(self, bfrt_info, target, name):
        self.bfrt_info = bfrt_info
        self.target = target

        # child clases must set table
        self.table = bfrt_info.table_get(name)

    def clear(self):
        # get all keys in table
        resp = self.table.entry_get(self.target, [], {"from_hw": False})

        # delete all keys in table
        for _, key in resp:
            if key:
                self.table.entry_del(self.target, [key])

class Register(Table):
    def __init__(self, bfrt_info, target, name):
        self.name = name
        super().__init__(bfrt_info, target, self.name)
        self.reset()
    
    def reset(self):
        total_keys = 1 << KEY_SIZE_BITS
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
                    [ gc.DataTuple(f"{self.name}.f1", new_value) ]
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
        return data[f"{self.name}.f1"][0]

class Expirator(Register):
    def __init__(self, bfrt_info, target):
        super().__init__(bfrt_info, target, f"SwitchIngress.expirator.reg")

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

        self.expirator = Expirator(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        for _ in range(100):
            key = custom_random(1, KEY_SIZE_BITS, used_keys)
            valid = send_and_get_validity(self, key, OP_READ)
            
            used_keys.append(key)

            alarm = self.expirator.get_value(key)
            print(f"key {key:3} alarm {alarm} valid {valid}")
            assert not valid

class ReadNonEmpty(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.expirator = Expirator(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        for _ in range(100):
            key = custom_random(1, KEY_SIZE_BITS, used_keys)
            valid = send_and_get_validity(self, key, OP_WRITE)
            ts1 = self.expirator.get_value(key)
            valid2 = send_and_get_validity(self, key, OP_READ)
            ts2 = self.expirator.get_value(key)
            
            used_keys.append(key)

            print(f"key {key:3} alarm {ts2} valid {valid} -> {valid2}")

            assert not valid
            assert valid2
            assert ts2 > ts1

class DoubleWrite(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.expirator = Expirator(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        for _ in range(100):
            key = custom_random(1, KEY_SIZE_BITS, used_keys)
            valid = send_and_get_validity(self, key, OP_WRITE)
            ts1 = self.expirator.get_value(key)
            valid2 = send_and_get_validity(self, key, OP_WRITE)
            ts2 = self.expirator.get_value(key)
            
            used_keys.append(key)

            print(f"key {key:3} alarm {ts2} valid {valid} -> {valid2}")

            assert not valid
            assert valid2
            assert ts2 > ts1

class Expire(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)

        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
        self.target = gc.Target(device_id=0)

        self.expirator = Expirator(self.bfrt_info, self.target)

    def runTest(self):
        used_keys = []
        for _ in range(100):
            key = custom_random(1, KEY_SIZE_BITS, used_keys)
            valid = send_and_get_validity(self, key, OP_WRITE)
            ts = self.expirator.get_value(key)
            used_keys.append(key)

            print(f"key {key:3} alarm {ts} valid {valid}")
        
        print(f"Waiting {EXPIRATION_TIME_SEC} seconds to let the entries expire...")
        time.sleep(EXPIRATION_TIME_SEC)

        for key in used_keys:
            valid = send_and_get_validity(self, key, OP_READ)
            ts = self.expirator.get_value(key)

            print(f"key {key:3} alarm {ts} valid {valid}")

            assert not valid
