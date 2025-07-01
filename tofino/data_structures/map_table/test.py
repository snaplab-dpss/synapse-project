#!/usr/bin/env python3

import random

from bfruntime_client_base_tests import BfRuntimeTest

from p4testutils.misc_utils import *
from ptf.testutils import *

from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, UDP

from dataclasses import dataclass

ARCH = test_param_get("arch")
assert ARCH in ["tofino", "tofino2"]

CLIENT_PORT = 0 if test_param_get("arch") == "tofino" else 8

PROGRAM = "map_table"

TIMEOUT_SEC = 2
SRC_MAC = "00:11:22:33:44:55"
DST_MAC = "00:11:22:33:44:56"

CAPACITY = 65536
EXPIRATION_TIME_SEC = 1


@dataclass(frozen=True)
class Flow:
    src_ip: str
    dst_ip: str
    src_port: int
    dst_port: int

    def __str__(self):
        return f"{{{self.src_ip}:{self.src_port} -> {self.dst_ip}:{self.dst_port}}}"


def generate_random_ip():
    return f"{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(0, 255)}"


def generate_random_port():
    return random.randint(1024, 65535)


def send_and_expect(bfruntimetest, flow: Flow, expected_port: int, send_to_port: int = CLIENT_PORT, verbose: bool = False) -> Flow:
    ipkt = Ether(src=SRC_MAC, dst=DST_MAC)
    ipkt /= IP(src=flow.src_ip, dst=flow.dst_ip)
    ipkt /= UDP(sport=flow.src_port, dport=flow.dst_port)

    if verbose:
        print(f"[sent] {flow}")

    send_packet(bfruntimetest, send_to_port, ipkt)
    _, _, obytes, _ = dp_poll(bfruntimetest, 0, expected_port, timeout=TIMEOUT_SEC)

    assert obytes is not None, f"[recv] No response from port {expected_port}"
    opkt = Ether(obytes)

    print(f"[recv] {opkt.summary()}")

    assert IP in opkt and UDP in opkt
    out_flow = Flow(
        src_ip=opkt[IP].src,
        dst_ip=opkt[IP].dst,
        src_port=opkt[UDP].sport,
        dst_port=opkt[UDP].dport,
    )

    if verbose:
        print(f"[recv] {out_flow}")

    return out_flow


class Random(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

        self.flows = []
        for _ in range(CAPACITY // 2):
            src_ip = generate_random_ip()
            dst_ip = generate_random_ip()
            src_port = generate_random_port()
            dst_port = generate_random_port()
            flow = Flow(src_ip=src_ip, dst_ip=dst_ip, src_port=src_port, dst_port=dst_port)
            self.flows.append(flow)

    def runTest(self):
        n = 1000
        for i in range(n):
            flow = random.choice(self.flows)
            res_flow = send_and_expect(self, flow, CLIENT_PORT, CLIENT_PORT)
            assert flow == res_flow, f"Expected flow {flow}, but got {res_flow}"
            print(f"Test {i + 1}/{n}: {flow} -> {res_flow}")
        print()

    def tearDown(self):
        self.interface.clear_all_tables()
        super().tearDown()
