#!/usr/bin/env python3

import struct

import ptf.testutils as testutils
from bfruntime_client_base_tests import BfRuntimeTest
from p4testutils.misc_utils import *
from ptf.testutils import *

PROGRAM = 'bloom_filter'
    
class NoEntries(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, PROGRAM)
        self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)

    def runTest(self):
        dst_ip = 0x00010203
        src_ip = 0x04050607

        ipkt = testutils.simple_tcp_packet(ip_src=src_ip, ip_dst=dst_ip)

        assert test_param_get('arch') in [ 'tofino', 'tofino2' ]
        if test_param_get('arch') == 'tofino':
            in_port = 0
            out_port = 1
        else:
            in_port = 8
            out_port = 9

        for i in range(2):
            print(f'[{i}] Sending packet on port {in_port}')
            testutils.send_packet(self, in_port, ipkt)

            _, _, epkt, _ = testutils.dp_poll(self, 0, out_port, timeout=2)

            cpu_header = epkt[:5]
            cpu_entry = struct.unpack("!I", cpu_header[0:4])[0]
            cpu_found = int(cpu_header[4])

            if i == 0:
                assert cpu_entry == 0
                assert cpu_found == 0
            else:
                assert cpu_entry == 3
                assert cpu_found == 1

            print(f"[{i}] {cpu_entry} {cpu_found}")
