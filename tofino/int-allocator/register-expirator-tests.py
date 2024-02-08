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

MAX_PORTS = 256

class RegisterExpirator:
	def __init__(self, bfrt_info, target):
		self.bfrt_info = bfrt_info
		self.target = target

		self.int_allocator_tail = self.bfrt_info.table_get('Ingress.int_allocator_tail')
		self.int_allocator_head = self.bfrt_info.table_get('Ingress.int_allocator_head')
		self.int_allocator_values = self.bfrt_info.table_get('Ingress.int_allocator_values')

	def setPortAllocatorTail(self, tail):
		logger.info(f"int_allocator_tail = {tail}")

		self.int_allocator_tail.entry_add(
			self.target,
			[
				self.int_allocator_tail.make_key([ gc.KeyTuple('$REGISTER_INDEX', 0) ])
			],
			[
				self.int_allocator_tail.make_data(
					[ gc.DataTuple('Ingress.int_allocator_tail.f1', tail) ]
				)
			])
	
	def getPortAllocatorTail(self):
		resp = self.int_allocator_tail.entry_get(
			self.target,
			[
				self.int_allocator_tail.make_key([ gc.KeyTuple('$REGISTER_INDEX', 0) ])
			],
			{ 'from_hw': True }
		)

		data = next(resp)[0].to_dict()
		return data['Ingress.int_allocator_tail.f1'][0]

	def setPortAllocatorHead(self, head):
		logger.info(f"int_allocator_head = {head}")

		self.int_allocator_head.entry_add(
			self.target,
			[
				self.int_allocator_head.make_key([ gc.KeyTuple('$REGISTER_INDEX', 0) ])
			],
			[
				self.int_allocator_head.make_data(
					[ gc.DataTuple('Ingress.int_allocator_head.f1', head) ]
				)
			])
	
	def getPortAllocatorHead(self):
		resp = self.int_allocator_head.entry_get(
			self.target,
			[
				self.int_allocator_head.make_key([ gc.KeyTuple('$REGISTER_INDEX', 0) ])
			],
			{ 'from_hw': True }
		)

		data = next(resp)[0].to_dict()
		return data['Ingress.int_allocator_head.f1'][0]
	
	def setPortAllocatorPorts(self, index, port):
		logger.info(f"int_allocator_values[{index}] = {port}")

		self.int_allocator_values.entry_add(
			self.target,
			[
				self.int_allocator_values.make_key([ gc.KeyTuple('$REGISTER_INDEX', index) ])
			],
			[
				self.int_allocator_values.make_data(
					[ gc.DataTuple('Ingress.int_allocator_values.f1', port) ]
				)
			])
	
	def getPortAllocatorPorts(self, index):
		resp = self.int_allocator_values.entry_get(
			self.target,
			[
				self.int_allocator_values.make_key([ gc.KeyTuple('$REGISTER_INDEX', index) ])
			],
			{ 'from_hw': True }
		)

		data = next(resp)[0].to_dict()
		return data['Ingress.int_allocator_values.f1'][0]

class FillUpAndExpire(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		p4_name = "register-expirator"
		BfRuntimeTest.setUp(self, client_id, p4_name)

		self.ig_port = 0
		self.eg_port = 1
		
		self.bfrt_info = self.interface.bfrt_info_get(p4_name)
		self.target = gc.Target(device_id=0)

		self.register_expirator = RegisterExpirator(self.bfrt_info, self.target)

		self.register_expirator.setPortAllocatorHead(0)
		self.register_expirator.setPortAllocatorTail(MAX_PORTS - 1)

		for i in range(MAX_PORTS):
			self.register_expirator.setPortAllocatorPorts(i, i + 1)
	
	def expirePort(self, expired_port):
		tail = self.register_expirator.getPortAllocatorTail()

		logger.info(f"expiring port {expired_port} stored in index {tail}")

		self.register_expirator.setPortAllocatorPorts(tail, expired_port)
		self.register_expirator.setPortAllocatorTail((tail + 1) % MAX_PORTS)
	
	def allocatePort(self, expected_port):
		logger.info(f"allocating port {expected_port}")

		ipkt = testutils.simple_udp_packet(with_udp_chksum=False)
		epkt = copy.deepcopy(ipkt)
		epkt['UDP'].dport = expected_port

		testutils.send_packet(self, self.ig_port, ipkt, count=1)
		testutils.verify_packets(self, epkt, [ self.eg_port ])
	
	def failToAllocatePort(self, expected_port):
		logger.info(f"failing to allocate any port")

		ipkt = testutils.simple_udp_packet(with_udp_chksum=False)
		epkt = copy.deepcopy(ipkt)
		epkt['UDP'].dport = expected_port

		testutils.send_packet(self, self.ig_port, ipkt, count=1)
		testutils.verify_no_packet(self, epkt, self.eg_port)

	def runTest(self):
		head_prediction = self.register_expirator.getPortAllocatorHead()
		tail_prediction = self.register_expirator.getPortAllocatorTail()

		for port in range(1, min((MAX_PORTS + 1), 6)):
			self.allocatePort(port)
			head_prediction += 1

			assert head_prediction == self.register_expirator.getPortAllocatorHead()
			assert tail_prediction == self.register_expirator.getPortAllocatorTail()
		
		# allocating 256 ports will take a looong time
		self.register_expirator.setPortAllocatorHead(MAX_PORTS - 1)
		head_prediction = MAX_PORTS - 1

		assert head_prediction == self.register_expirator.getPortAllocatorHead()

		self.failToAllocatePort(MAX_PORTS + 1)

		assert head_prediction == self.register_expirator.getPortAllocatorHead()
		assert tail_prediction == self.register_expirator.getPortAllocatorTail()

		self.expirePort(3)
		tail_prediction = (tail_prediction + 1) % MAX_PORTS

		assert head_prediction == self.register_expirator.getPortAllocatorHead()
		assert tail_prediction == self.register_expirator.getPortAllocatorTail()

		self.allocatePort(3)
		head_prediction = (head_prediction + 1) % MAX_PORTS

		assert head_prediction == self.register_expirator.getPortAllocatorHead()
		assert tail_prediction == self.register_expirator.getPortAllocatorTail()
