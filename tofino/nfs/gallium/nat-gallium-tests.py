#!/usr/bin/env python3

import sys
import os
import logging
import copy
import time

from scapy.all import *

try:
	# Import BFRT GRPC stuff
	import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
	import bfrt_grpc.client as gc
	import grpc

	from ptf import config, ptfutils

	from ptf import packet
	from bf_pktpy.library.specs.templates.payload import Payload

	from ptf.testutils import *
	from bfruntime_client_base_tests import BfRuntimeTest
except:
	python_v = '{}.{}'.format(sys.version_info.major, sys.version_info.minor)
	sde_install = os.environ['SDE_INSTALL']
	sde_python_libs = '{}/lib/python{}/site-packages'.format(sde_install, python_v)

	tofino_libs = '{}/tofino'.format(sde_python_libs)
	testing_libs = '{}/p4testutils/'.format(sde_python_libs)

	sys.path.append(tofino_libs)
	sys.path.append(testing_libs)

	import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
	import bfrt_grpc.client as gc
	import grpc

	from ptf import config, ptfutils

	from ptf import packet
	from bf_pktpy.library.specs.templates.payload import Payload

	from ptf.testutils import *
	from bfruntime_client_base_tests import BfRuntimeTest

logger = logging.getLogger('Test')
if not len(logger.handlers):
	logger.addHandler(logging.StreamHandler())

P4_PROGRAM = 'nat-gallium'
PUBLIC_IP = 0x08080808

LAN = 0
WAN = 1
CPU = 2

class CPU(Packet):
	name = "CPU"
	fields_desc = [
		ShortField("allocated_port", 0),
		ShortField("out_port", 0),
	]

class NAT:
	def __init__(self, bfrt_info, target):
		self.bfrt_info = bfrt_info
		self.target = target

		self.int_to_ext = self.bfrt_info.table_get('Ingress.nat_int_to_ext')
		self.ext_to_int = self.bfrt_info.table_get('Ingress.nat_ext_to_int')
	
	def __add_int_to_ext(src_addr, dst_addr, src_port, dst_port, allocated_port):
		self.int_to_ext.entry_add(
			self.target,
			[
				self.map.make_key([
					gc.KeyTuple(f'hdr.ipv4.src_addr', src_addr),
					gc.KeyTuple(f'hdr.ipv4.dst_addr', dst_addr),
					gc.KeyTuple(f'hdr.tcpudp.src_port', src_port),
					gc.KeyTuple(f'hdr.tcpudp.dst_port', dst_port),
				])
			],
			[
				self.map.make_data([
					gc.DataTuple('src_addr', PUBLIC_IP),
					gc.DataTuple('src_port', allocated_port),
				], 'Ingress.nat_int_to_ext_hit')
			])
	
	def __add_ext_to_int(src_addr, dst_addr, src_port, dst_port, allocated_port):
		self.ext_to_int.entry_add(
			self.target,
			[
				self.map.make_key([
					gc.KeyTuple(f'hdr.ipv4.src_addr', dst_addr),
					gc.KeyTuple(f'hdr.ipv4.dst_addr', PUBLIC_IP),
					gc.KeyTuple(f'hdr.tcpudp.src_port', dst_port),
					gc.KeyTuple(f'hdr.tcpudp.dst_port', allocated_port),
				])
			],
			[
				self.map.make_data([
					gc.DataTuple('dst_addr', src_addr),
					gc.DataTuple('dst_port', src_port),
				], 'Ingress.nat_int_to_ext_hit')
			])

	def add_flow(self, src_addr, dst_addr, src_port, dst_port, allocated_port):
		self.__add_int_to_ext(src_addr, dst_addr, src_port, dst_port, allocated_port)
		self.__add_ext_to_int(src_addr, dst_addr, src_port, dst_port, allocated_port)

	def handle_CPU_pkt(self, pkt):
		src_addr = pkt[IP].src
		dst_addr = pkt[IP].dst
		src_port = pkt[TCP].sport
		dst_port = pkt[TCP].dport

		print(pkt)
		print(src_addr)
		print(dst_addr)
		print(src_port)
		print(dst_port)

		print("  Added an entry to nat_int_to_ext: {}:{} --> {}:{}".format(
			src_addr, src_port, PUBLIC_IP, 8080))

class LanNewFlow(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, P4_PROGRAM)
		
		self.bfrt_info = self.interface.bfrt_info_get(P4_PROGRAM)
		self.target = gc.Target(device_id=0)

		self.nat = NAT(self.bfrt_info, self.target)

	def runTest(self):
		pkt = simple_tcp_packet(eth_dst="00:98:76:54:32:10",
								eth_src='00:55:55:55:55:55',
								ip_dst="172.168.1.1",
								ip_src="192.168.1.1",
								ip_id=101,
								ip_ttl=64,
								ip_ihl=5)
		send_packet(self, LAN, pkt)

		# epkt = copy.deepcopy(ipkt) / CPU(allocated_port=0, out_port=0)
		# epkt = copy.deepcopy(ipkt) / Payload(data='\x00\x00\x00\x00')
		# epkt = packet.Ether() / packet.IP() / packet.UDP() / Payload(data='\x00\x00\x00\x00')
		# epkt['UDP'].dport = expected_port

		# testutils.verify_packets(self, ipkt, [ CPU ])
		timeout = ptfutils.default_timeout
		epkt = dp_poll(self, device_number=0, port_number=CPU, timeout=timeout)
		print(epkt)
