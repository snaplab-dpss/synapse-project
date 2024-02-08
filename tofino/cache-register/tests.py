#!/usr/bin/env python3

import os
import sys
import signal
import argparse
import json
import random

from time import sleep

from ptf import config
import ptf.testutils as testutils
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as client

PADDING = 30
PROGRAM = 'timeouts'

class Table(object):
	def __init__(self, client, bfrt_info, name):
		self.gc = client
		self.bfrt_info = bfrt_info
		self.target = self.gc.Target(device_id=0, pipe_id=0xffff)

		# child clases must set table
		self.table = bfrt_info.table_get(name)

		# lowest possible  priority for ternary match rules
		self.lowest_priority = 1 << 24

	def clear(self):
		# get all keys in table
		resp = self.table.entry_get(self.target, [], {"from_hw": False})

		# delete all keys in table
		for _, key in resp:
			if key:
				self.table.entry_del(self.target, [key])

	def print_current_state(self):
		print()
		print("====================================================")

		print("Table info:")
		print("  ID   {}".format(self.table.info.id_get()))
		print("  Name {}".format(self.table.info.name_get()))

		actions = self.table.info.action_name_list_get()
		data_fields = self.table.info.data_field_name_list_get()
		key_fields = self.table.info.key_field_name_list_get()
		attributes = self.table.info.attributes_supported_get()

		print("  Actions:")
		for action in actions:
			print("  {}".format(action))

		print("  Data fields:")
		for data_field in data_fields:
			data_field_size = self.table.info.data_field_size_get(data_field)
			data_field_type = self.table.info.data_field_size_get(data_field)

			print("    Name {}".format(data_field))
			print("    Size {} bytes, {} bits".format(data_field_size[0], data_field_size[1]))
			print("    Type {}".format(data_field_type))
		
		print("  Keys:")
		for key_field in key_fields:
			key_field_size = self.table.info.key_field_size_get(key_field)
			key_field_type = self.table.info.key_field_type_get(key_field)

			print("    Name {}".format(key_field))
			print("    Size {} bytes, {} bits".format(key_field_size[0], key_field_size[1]))
			print("    Type {}".format(key_field_type))

		print("  Attributes:")
		for attribute in attributes:
			print("    Name {}".format(attribute))

		target = gc.Target(device_id=0, pipe_id=0xffff)
		keys = self.table.entry_get(target, [], {"from_hw": False})

		print("Table entries:")
		for data, key in keys:
			print("  -------------------------------------------------")
			for key_field in key.to_dict().keys():
				print("  {} {}".format(key_field.ljust(PADDING), key.to_dict()[key_field]['value']))

			for data_field in data.to_dict().keys():
				if data_field in [ 'is_default_entry', 'action_name' ]:
					continue

				print("  {} {}".format(data_field.ljust(PADDING), data.to_dict()[data_field]))

			print("  {} {}".format("Actions".ljust(PADDING), data.to_dict()['action_name']))

		print("====================================================")
		print()


class TableWithTimeout(Table):
	def __init__(self, client, bfrt_info):
		super(TableWithTimeout, self).__init__(client, bfrt_info, "Ingress.table_with_timeout")
	
	def __set_notify_mode(self, enable, ttl_ms):
		mode = bfruntime_pb2.IdleTable.IDLE_TABLE_NOTIFY_MODE
		self.table.attribute_idle_time_set(self.target, enable, mode, ttl_ms)
		
		resp = self.table.attribute_get(self.target, "IdleTimeout")
		for d in resp:
			assert d["ttl_query_interval"] == ttl_ms
			assert d["idle_table_mode"] == mode
			assert d["enable"] == enable

	def enable_notify_mode(self, ttl_ms):
		print(f'--- ENABLE NOTIFY MODE TTL {ttl_ms} ms')
		self.__set_notify_mode(True, ttl_ms)
	
	def disable_notify_mode(self, ttl_ms):
		print(f'--- DISABLE NOTIFY MODE TTL {ttl_ms} ms')
		self.__set_notify_mode(False, ttl_ms)
	
	def __set_poll_mode(self, enable):
		mode = bfruntime_pb2.IdleTable.IDLE_TABLE_POLL_MODE
		self.table.attribute_idle_time_set(self.target, enable, mode)
		
		resp = self.table.attribute_get(self.target, "IdleTimeout")
		for d in resp:
			assert d["ttl_query_interval"] == 0
			assert d["idle_table_mode"] == mode
			assert d["enable"] == enable
	
	def enable_poll_mode(self):
		print(f'--- ENABLE POLL MODE')
		self.__set_poll_mode(True)
	
	def disable_poll_mode(self):
		print(f'--- DISABLE POLL MODE')
		self.__set_poll_mode(False)

	def get_entry(self, key):		
		resp = self.table.entry_get(
			self.target,
			[
				self.table.make_key([
					self.gc.KeyTuple('hdr.data.data[111:96]', key),
				])
			],
			{"from_hw": False}
		)

		return next(resp)[0].to_dict()
	
	def get_entry_ttl(self, key):
		entry = self.get_entry(key)
		assert '$ENTRY_TTL' in entry.keys()

		ttl = entry['$ENTRY_TTL']
		print(f'--- GET TTL {hex(key)} => {ttl} ms')

		return ttl
	
	def poll_hit_state(self, key):
		self.table.operations_execute(self.target, 'UpdateHitState')

		entry = self.get_entry(key)
		assert '$ENTRY_HIT_STATE' in entry.keys()

		state = entry['$ENTRY_HIT_STATE']
		print(f'--- GET TTL STATE {hex(key)} => {state}')

		return state
	
	def add_entry(self, key, out_port, ttl_ms=None):
		print(f'--- ADD {hex(key)} => {out_port} [ttl={ttl_ms}]')

		keys = [
			self.gc.KeyTuple(f'hdr.data.data[111:96]', key),
		]

		data = [
			self.gc.DataTuple(f'port', out_port)
		]

		if ttl_ms:
			data.append(self.gc.DataTuple(f'$ENTRY_TTL', ttl_ms))

		self.table.entry_add(
			self.target,
			[ self.table.make_key(keys) ],
			[ self.table.make_data(data, 'Ingress.fwd') ]
		)
	
	def clear(self):
		print('--- CLEAR')
		self.table.entry_del(self.target, [])

class EntriesWithoutTimeout(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)
		
		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.table = TableWithTimeout(client, self.bfrt_info)
		self.table.clear()

	def runTest(self):
		TABLE_TTL_MS = 3000 # 3 seconds
		self.table.enable_notify_mode(TABLE_TTL_MS)

		dmac_bytes = client.to_bytes(random.randint(0, 2 ** 48 - 1), 6)
		key = int.from_bytes(dmac_bytes[0:2], 'big')

		dmac = client.bytes_to_mac(dmac_bytes)
		pkt = testutils.simple_tcp_packet(eth_dst=dmac)
		exp_pkt = pkt

		in_port = 0
		out_port = 1

		self.table.add_entry(key, out_port)
		assert self.table.get_entry_ttl(key) == 0

		sleep(1)

		print(f'Sending packet on port {in_port}')
		testutils.send_packet(self, in_port, pkt)

		print(f'Expecting packet on {out_port}')
		testutils.verify_packets(self, exp_pkt, [out_port])

		self.table.disable_notify_mode(TABLE_TTL_MS)

class EntriesWithTimeout(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)
		
		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.table = TableWithTimeout(client, self.bfrt_info)
		self.table.clear()

	def runTest(self):
		TABLE_TTL_MS = 3000 # 3 seconds
		self.table.enable_notify_mode(TABLE_TTL_MS)

		dmac_bytes = client.to_bytes(random.randint(0, 2 ** 48 - 1), 6)
		key = int.from_bytes(dmac_bytes[0:2], 'big')

		dmac = client.bytes_to_mac(dmac_bytes)
		pkt = testutils.simple_tcp_packet(eth_dst=dmac)
		exp_pkt = pkt

		in_port = 0
		out_port = 1

		# Entry TTL must be >= table TTL
		ENTRY_TTL_MS = TABLE_TTL_MS + 1000

		self.table.add_entry(key, out_port, ENTRY_TTL_MS)

		assert self.table.get_entry_ttl(key) == ENTRY_TTL_MS

		print(f'Sending packet on port {in_port}')
		testutils.send_packet(self, in_port, pkt)

		print(f'Expecting packet on {out_port}')
		testutils.verify_packets(self, exp_pkt, [out_port])

		# Entry TTL is not supposed to change with time

		sleep(1)
		assert self.table.get_entry_ttl(key) == ENTRY_TTL_MS
		sleep(1)
		assert self.table.get_entry_ttl(key) == ENTRY_TTL_MS
		sleep(1)
		assert self.table.get_entry_ttl(key) == ENTRY_TTL_MS

		self.table.get_entry_ttl(key)

		self.table.disable_notify_mode(TABLE_TTL_MS)

class PollMode(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)
		
		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.table = TableWithTimeout(client, self.bfrt_info)
		self.table.clear()

	def runTest(self):
		self.table.enable_poll_mode()

		dmac_bytes = client.to_bytes(random.randint(0, 2 ** 48 - 1), 6)
		key = int.from_bytes(dmac_bytes[0:2], 'big')

		dmac = client.bytes_to_mac(dmac_bytes)
		pkt = testutils.simple_tcp_packet(eth_dst=dmac)
		exp_pkt = pkt

		in_port = 0
		out_port = 1

		self.table.add_entry(key, out_port)

		sleep(1)
		assert self.table.poll_hit_state(key) == 'ENTRY_IDLE'

		sleep(1)
		assert self.table.poll_hit_state(key) == 'ENTRY_IDLE'

		sleep(1)
		assert self.table.poll_hit_state(key) == 'ENTRY_IDLE'

		print(f'Sending packet on port {in_port}')
		testutils.send_packet(self, in_port, pkt)

		print(f'Expecting packet on {out_port}')
		testutils.verify_packets(self, exp_pkt, [out_port])

		assert self.table.poll_hit_state(key) == 'ENTRY_ACTIVE'

		self.table.disable_poll_mode()