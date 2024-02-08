#!/usr/bin/env python3

import sys
import os
import logging
import copy
import time

try:
	# Import BFRT GRPC stuff
	import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
	import bfrt_grpc.client as gc
	import grpc

	from ptf import config
	import ptf.testutils as testutils
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

	from ptf import config
	import ptf.testutils as testutils
	from bfruntime_client_base_tests import BfRuntimeTest

logger = logging.getLogger('Test')
if not len(logger.handlers):
	logger.addHandler(logging.StreamHandler())

class Map:
	def __init__(self, bfrt_info, target):
		self.bfrt_info = bfrt_info
		self.target = target
		self.map = self.bfrt_info.table_get('Ingress.map')

	def add_entry(self, values):
		logger.info(f"adding values = {values}")

		keys = [ gc.KeyTuple(f'key_byte_{i}', v) for i,v in enumerate(values) ]

		self.map.entry_add(
			self.target,
			[
				self.map.make_key(keys)
			],
			[
				self.map.make_data([], 'Ingress.hit')
			])

class LanMatch(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		p4_name = "indirect-keys"
		BfRuntimeTest.setUp(self, client_id, p4_name)

		self.lan = 0
		self.wan = 1
		self.eg_port = 2
		
		self.bfrt_info = self.interface.bfrt_info_get(p4_name)
		self.target = gc.Target(device_id=0)

		self.map = Map(self.bfrt_info, self.target)

		self.src_ip = 0x00010203
		self.dst_ip = 0x04050607
		self.src_port = 0x0809
		self.dst_port = 0x0a0b

		self.map.add_entry([
			0x03, 0x02, 0x01, 0x00,
			0x07, 0x06, 0x05, 0x04,
			0x09, 0x08,
			0x0b, 0x0a,
		])

	def runTest(self):
		pkt = testutils.simple_tcp_packet(
			ip_src=self.src_ip,
			ip_dst=self.dst_ip,
			tcp_sport=self.src_port,
			tcp_dport=self.dst_port
		)

		exp_pkt = pkt

		logger.info("Sending packet on port %d", self.lan)
		testutils.send_packet(self, self.lan, pkt)

		logger.info("Expecting packet on port %d", self.eg_port)
		testutils.verify_packets(self, exp_pkt, [ self.eg_port ])

class WanMatch(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		p4_name = "indirect-keys"
		BfRuntimeTest.setUp(self, client_id, p4_name)

		self.lan = 0
		self.wan = 1
		self.eg_port = 2
		
		self.bfrt_info = self.interface.bfrt_info_get(p4_name)
		self.target = gc.Target(device_id=0)

		self.map = Map(self.bfrt_info, self.target)

		self.dst_ip = 0x00010203
		self.src_ip = 0x04050607
		self.dst_port = 0x0809
		self.src_port = 0x0a0b

		self.map.add_entry([
			0x07, 0x06, 0x05, 0x04,
			0x03, 0x02, 0x01, 0x00,
			0x0b, 0x0a,
			0x09, 0x08,
		])

	def runTest(self):
		pkt = testutils.simple_tcp_packet(
			ip_src=self.src_ip,
			ip_dst=self.dst_ip,
			tcp_sport=self.src_port,
			tcp_dport=self.dst_port
		)

		exp_pkt = pkt

		logger.info("Sending packet on port %d", self.wan)
		testutils.send_packet(self, self.wan, pkt)

		logger.info("Expecting packet on port %d", self.eg_port)
		testutils.verify_packets(self, exp_pkt, [ self.eg_port ])
