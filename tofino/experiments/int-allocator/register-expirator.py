#!/usr/bin/env python3

import sys
import os
import logging
import copy
import time
import argparse

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

PROGRAM = 'register-expirator'
DEFAULT_MAX_PORTS = 8

class RegisterExpirator:
	def __init__(self, bfrt_info, max_ports):
		self.bfrt_info = bfrt_info
		self.target = gc.Target(device_id=0, pipe_id=0xffff)
		self.max_ports = max_ports

		self.port_allocator_tail = self.bfrt_info.table_get('Ingress.port_allocator_tail')
		self.port_allocator_head = self.bfrt_info.table_get('Ingress.port_allocator_head')
		self.port_allocator_ports = self.bfrt_info.table_get('Ingress.port_allocator_ports')

	def __set_tail(self, tail):
		print(f"tail = {tail}")
		assert 0 <= tail < self.max_ports

		self.port_allocator_tail.entry_add(
			self.target,
			[
				self.port_allocator_tail.make_key([ gc.KeyTuple('$REGISTER_INDEX', 0) ])
			],
			[
				self.port_allocator_tail.make_data(
					[ gc.DataTuple('Ingress.port_allocator_tail.f1', tail) ]
				)
			])
		
		assert self.__get_tail() == tail
	
	def __get_tail(self):
		resp = self.port_allocator_tail.entry_get(
			self.target,
			[
				self.port_allocator_tail.make_key([ gc.KeyTuple('$REGISTER_INDEX', 0) ])
			],
			{ "from_hw": True }
		)

		data = next(resp)[0].to_dict()
		return data['Ingress.port_allocator_tail.f1'][0]

	def __set_head(self, head):
		print(f"head = {head}")
		assert 0 <= head < self.max_ports

		self.port_allocator_head.entry_add(
			self.target,
			[
				self.port_allocator_head.make_key([ gc.KeyTuple('$REGISTER_INDEX', 0) ])
			],
			[
				self.port_allocator_head.make_data(
					[ gc.DataTuple('Ingress.port_allocator_head.f1', head) ]
				)
			])
		
		assert self.__get_head() == head
	
	def __get_head(self):
		resp = self.port_allocator_head.entry_get(
			self.target,
			[
				self.port_allocator_head.make_key([ gc.KeyTuple('$REGISTER_INDEX', 0) ])
			],
			{ "from_hw": True }
		)

		data = next(resp)[0].to_dict()
		return data['Ingress.port_allocator_head.f1'][0]
	
	def __set_port(self, index, port):
		print(f"ports[{index}] = {port}")
		assert 0 <= index < self.max_ports

		self.port_allocator_ports.entry_add(
			self.target,
			[
				self.port_allocator_ports.make_key([ gc.KeyTuple('$REGISTER_INDEX', index) ])
			],
			[
				self.port_allocator_ports.make_data(
					[ gc.DataTuple('Ingress.port_allocator_ports.f1', port) ]
				)
			])
		
		assert self.__get_port(index) == port
	
	def __get_port(self, index):
		resp = self.port_allocator_ports.entry_get(
			self.target,
			[
				self.port_allocator_ports.make_key([ gc.KeyTuple('$REGISTER_INDEX', index) ])
			],
			{ "from_hw": True }
		)

		data = next(resp)[0].to_dict()
		return data['Ingress.port_allocator_ports.f1'][0]
	
	def reset(self):
		self.__set_head(0)
		self.__set_tail(self.max_ports - 1)

		for i in range(self.max_ports):
			self.__set_port(i, i + 1)

	def expire(self, port):
		tail = self.__get_tail()
		next_tail = (tail + 1) % self.max_ports
		self.__set_port(tail, port)
		self.__set_tail(next_tail)

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	
	parser.add_argument('--grpc-server', type=str, default='localhost', help='GRPC server name/address')
	parser.add_argument('--grpc-port', type=int, default=50052, help='GRPC server port')
	parser.add_argument('--max-ports', type=int, default=DEFAULT_MAX_PORTS, help='setup registers')
	parser.add_argument('--reset', action='store_true', help='setup registers')
	parser.add_argument('--expire', type=int, help='expire port')

	args = parser.parse_args()
		
	# connect to GRPC server
	print("Connecting to GRPC server {}:{} and binding to program {}..."
		.format(args.grpc_server, args.grpc_port, PROGRAM))

	grpc_client = gc.ClientInterface("{}:{}".format(args.grpc_server, args.grpc_port), 0, 0)
	grpc_client.bind_pipeline_config(PROGRAM)

	# get all tables for program
	bfrt_info = grpc_client.bfrt_info_get(PROGRAM)

	register_expirator = RegisterExpirator(bfrt_info, args.max_ports)

	if args.reset:
		register_expirator.reset()
	
	if args.expire is not None:
		register_expirator.expire(args.expire)
