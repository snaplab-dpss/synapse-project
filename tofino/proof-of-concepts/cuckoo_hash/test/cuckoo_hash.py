#!/usr/bin/env python3

import zlib
import random
import struct

import bfrt_grpc.client as gc
from ptf.testutils import test_param_get, send_packet, dp_poll
from bfruntime_client_base_tests import BfRuntimeTest

import scapy.layers.l2
import scapy.packet
import scapy.fields

PROGRAM = "cuckoo_hash"
CUCKOO_HASH_CAPACITY = 1024
HASH_SIZE_BITS = 10 # 2^10 = 1024
HASH_SIZE_BITS_BYTE_ALIGN = 6 # 10 + 6 = 16 bits = 2 bytes
KEY_SIZE_BITS = 128
SALT0 = 0x7f4a7c13
SALT1 = 0x97c29b3a

OP_READ = 0
OP_WRITE = 1
OP_DELETE = 2

assert CUCKOO_HASH_CAPACITY & (CUCKOO_HASH_CAPACITY - 1) == 0, "n must be a power of 2"
assert test_param_get('arch') in [ 'tofino', 'tofino2' ]

IN_PORT = 0 if test_param_get('arch') == 'tofino' else 8
OUT_PORT = 1 if test_param_get('arch') == 'tofino' else 9

class Report:
	def __init__(self):
		self.success = False
		self.duplicate = False
		self.collision = False
		self.recirc = 0
		self.loop = False
		self.hash = 0
		self.tid = 0
	
	def __str__(self):
		str = ""
		str += f"success={self.success} "
		str += f"tid={self.tid} "
		str += f"hash=0x{self.hash:x} "
		str += f"duplicate={self.duplicate} "
		str += f"collision={self.collision} "
		str += f"recirc={self.recirc} "
		str += f"loop={self.loop}"
		return str

class CuckooHashEmulator:
	def __init__(self):
		self.t0 = dict()
		self.t1 = dict()
		self.total_elements = 0
		self.elements = set()
	
	def usage(self):
		return self.total_elements / CUCKOO_HASH_CAPACITY
	
	def _hash(self, key, salt):
		num_bytes = KEY_SIZE_BITS // 8
		input_bytes = key.to_bytes(num_bytes, byteorder='big')
		salt_bytes = struct.pack("!I", salt)
		output = zlib.crc32(salt_bytes + input_bytes)
		return output & ((1 << HASH_SIZE_BITS) - 1)

	def hash0(self, key):
		return self._hash(key, SALT0)

	def hash1(self, key):
		return self._hash(key, SALT1)

	def clear(self):
		self.t0.clear()
		self.t1.clear()
		self.total_elements = 0
		self.elements.clear()
	
	def lookup(self, key):
		h0 = self.hash0(key)
		h1 = self.hash1(key)
		if h0 in self.t0 and self.t0[h0] == key:
			return True
		if h1 in self.t1 and self.t1[h1] == key:
			return True
		return False
	
	def get_tid_and_hash(self, key):
		h0 = self.hash0(key)
		h1 = self.hash1(key)
		if h0 in self.t0 and self.t0[h0] == key:
			return 0, h0
		if h1 in self.t1 and self.t1[h1] == key:
			return 1, h1
		assert False, f"Element 0x{key:x} not found in tables"
	
	def delete(self, key):
		h0 = self.hash0(key)
		h1 = self.hash1(key)
		if h0 in self.t0 and self.t0[h0] == key:
			del self.t0[h0]
			self.elements.remove(key)
			self.total_elements -= 1
			return True
		if h1 in self.t1 and self.t1[h1] == key:
			del self.t1[h1]
			self.elements.remove(key)
			self.total_elements -= 1
			return True
		return False

	def insert(self, key):
		result = Report()

		if self.lookup(key):
			tid, hash = self.get_tid_and_hash(key)
			result.tid = tid
			result.hash = hash
			result.duplicate = True
			return result
		
		# Every insertion requires a recirculation first.
		result.recirc = 1

		insertions = set()
		tables = [self.t0, self.t1]
		hashes = [self.hash0, self.hash1]
		target_key = key
		target_tid = 0

		result.tid = 0
		result.hash = self.hash0(key)

		while True:
			h = hashes[target_tid](target_key)

			# print(f"Inserting 0x{target_key:x} in t{target_tid}[{h}]")
			insertions.add((target_key, target_tid))

			if h not in tables[target_tid]:
				tables[target_tid][h] = target_key
				break
			
			result.collision = True
			
			# print(f"Ejecting 0x{tables[target_tid][h]:x}")
			tables[target_tid][h], target_key = target_key, tables[target_tid][h]
			target_tid = 1 - target_tid

			if target_tid == 0:
				if target_key == key:
					result.loop = True
					break
				result.recirc += 1
			elif target_key == key:
				# Inserting the original key on the second table.
				result.tid = 1
				result.hash = h
		
		if not result.loop:
			result.success = True
			self.elements.add(key)
			self.total_elements += 1

		return result
	
	def assert_invariants(self):
		assert len(self.t0) + len(self.t1) == self.total_elements
		assert len(self.elements) == self.total_elements

		for element in self.elements:
			h0 = self.hash0(element)
			h1 = self.hash1(element)
			in_t1 = h0 in self.t0 and self.t0[h0] == element
			in_t2 = h1 in self.t1 and self.t1[h1] == element
			assert in_t1 or in_t2, f"Element 0x{element:x} not found in tables"
			assert not (in_t1 and in_t2), f"Element 0x{element:x} found in both tables"
	
class Table(object):
	def __init__(self, bfrt_info, target, name):
		self.bfrt_info = bfrt_info
		self.target = target
		self.name = name
		self.table = bfrt_info.table_get(name)

	def clear(self):
		resp = self.table.entry_get(self.target, [], {"from_hw": True})
		for _, key in resp:
			if key:
				self.table.entry_del(self.target, [key])

class Register(Table):
	def __init__(self, bfrt_info, target, name):
		super().__init__(bfrt_info, target, name)
		self.clear()
	
	def read(self, index):
		key = self.table.make_key([gc.KeyTuple("$REGISTER_INDEX", index)])
		resp = self.table.entry_get(self.target, [key], {"from_hw": True})
		data = next(resp)[0].to_dict()

		# On real hardware, we have a register per pipe.
		# During testing, we assume a single pipe usage, and so discard all the other pipe values.
		value = data[f"{self.name}.f1"][0]

		return value

	def write(self, index, value):
		key = self.table.make_key([gc.KeyTuple("$REGISTER_INDEX", index)])
		data = self.table.make_data([gc.DataTuple(f"{self.name}.f1", value)])
		self.table.entry_add(self.target, [key], [data])
	
	def clear(self):
		resp = self.table.entry_get(self.target, [], {"from_hw": True})
		for _, key in resp:
			index = key.to_dict()["$REGISTER_INDEX"]["value"]
			if key:
				self.write(index, 0)

class CuckooHash:
	def __init__(self, bfrt_info, target):
		self.expirator_t0 = Register(bfrt_info, target, "Ingress.expirator_t0")
		self.expirator_t1 = Register(bfrt_info, target, "Ingress.expirator_t1")

		self.t0_k0_31 = Register(bfrt_info, target, "Ingress.t0_k0_31")
		self.t0_k32_63 = Register(bfrt_info, target, "Ingress.t0_k32_63")
		self.t0_k64_95 = Register(bfrt_info, target, "Ingress.t0_k64_95")
		self.t0_k96_127 = Register(bfrt_info, target, "Ingress.t0_k96_127")

		self.t1_k0_31 = Register(bfrt_info, target, "Ingress.t1_k0_31")
		self.t1_k32_63 = Register(bfrt_info, target, "Ingress.t1_k32_63")
		self.t1_k64_95 = Register(bfrt_info, target, "Ingress.t1_k64_95")
		self.t1_k96_127 = Register(bfrt_info, target, "Ingress.t1_k96_127")

		self.ts = [
			[self.t0_k0_31, self.t0_k32_63, self.t0_k64_95, self.t0_k96_127],
			[self.t1_k0_31, self.t1_k32_63, self.t1_k64_95, self.t1_k96_127],
		]

		self.clear()
	
	def expirator_read_all(self):
		v0 = []
		v1 = []
		for i in range(CUCKOO_HASH_CAPACITY):
			v0.append(self.expirator_t0.read(i))
			v1.append(self.expirator_t1.read(i))
		return v0, v1

	def expirator_read(self, tid, index):
		assert tid in [0, 1]
		return self.expirator_t0.read(index) if tid == 0 else self.expirator_t1.read(index)
	
	def expirator_write(self, tid, index, value):
		assert tid in [0, 1]
		if tid == 0:
			self.expirator_t0.write(index, value)
		else:
			self.expirator_t1.write(index, value)
	
	def read(self, tid, index):
		assert tid in [0, 1]
		
		k0_31   = self.ts[tid][0].read(index)
		k32_63  = self.ts[tid][1].read(index)
		k64_95  = self.ts[tid][2].read(index)
		k96_127 = self.ts[tid][3].read(index)

		value = (k96_127 << 96) | (k64_95 << 64) | (k32_63 << 32) | k0_31
		return value

	def write(self, tid, index, value):
		assert tid in [0, 1]

		k0_31   = value & ((1 << 32) - 1)
		k32_63  = (value >> 32) & ((1 << 32) - 1)
		k64_95  = (value >> 64) & ((1 << 32) - 1)
		k96_127 = (value >> 96) & ((1 << 32) - 1)

		self.ts[tid][0].write(index, k0_31)
		self.ts[tid][1].write(index, k32_63)
		self.ts[tid][2].write(index, k64_95)
		self.ts[tid][3].write(index, k96_127)
	
	def delete(self, tid, index):
		assert tid in [0, 1]

		self.write(tid, index, 0)

		if tid == 0:
			self.expirator_t0.write(index, 0)
		else:
			self.expirator_t1.write(index, 0)

	def clear(self):
		self.expirator_t0.clear()
		self.expirator_t1.clear()

		self.t0_k0_31.clear()
		self.t0_k32_63.clear()
		self.t0_k64_95.clear()
		self.t0_k96_127.clear()

		self.t1_k0_31.clear()
		self.t1_k32_63.clear()
		self.t1_k64_95.clear()
		self.t1_k96_127.clear()

def custom_random(min_bits, max_bits, forbidden=[]):
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
		scapy.fields.BitField("op", 0, 8),
		scapy.fields.BitField("key", 0, 128),
		scapy.fields.BitField("success", 0, 8),
		scapy.fields.BitField("tid", 0, 8),
		scapy.fields.BitField("hash_pad", 0, HASH_SIZE_BITS_BYTE_ALIGN),
		scapy.fields.BitField("hash", 0, HASH_SIZE_BITS),
		scapy.fields.BitField("duplicate", 0, 8),
		scapy.fields.BitField("collision", 0, 8),
		scapy.fields.BitField("recirc", 0, 32),
		scapy.fields.BitField("loop", 0, 8),
	]

	def build_report(self):
		result = Report()
		result.success = False if self.success == 0 else True
		result.tid = self.tid
		result.hash = self.hash
		result.duplicate = False if self.duplicate == 0 else True
		result.collision = False if self.collision == 0 else True
		result.recirc = self.recirc
		result.loop = False if self.loop == 0 else True
		return result

scapy.packet.bind_layers(scapy.layers.l2.Ether, App)

def send_and_get_result_back(bfruntimetest, op, key):
	ether = scapy.layers.l2.Ether(src='00:11:22:33:44:55', dst='00:11:22:33:44:55')
	app = App(op=op, key=key)
	ipkt = ether / app

	send_packet(bfruntimetest, IN_PORT, ipkt)
	_, _, obytes, _ = dp_poll(bfruntimetest, 0, OUT_PORT, timeout=2)
	oether = scapy.layers.l2.Ether(obytes)
	oapp = App(bytes(oether[1]))

	return oapp.build_report()

class SingleInsertion(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)

		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.target = gc.Target(device_id=0)

		self.cuckoo_hash = CuckooHash(self.bfrt_info, self.target)
		self.emulator = CuckooHashEmulator()
	
	def runTest(self):
		keys = []
		for _ in range(100):
			key = custom_random(1, KEY_SIZE_BITS, keys)
			keys.append(key)

			expected = self.emulator.insert(key)
			result = send_and_get_result_back(self, OP_WRITE, key)
			value = self.cuckoo_hash.read(expected.tid, expected.hash)
			
			print("[SingleInsertion]")
			print(f"Key:      0x{key:x}")
			print(f"Expected: tid={expected.tid} hash={hex(expected.hash)}")
			print(f"Read:     0x{value:x}")
			print(f"Result:   {result}")
			print()

			assert expected.success
			assert not expected.duplicate
			assert not expected.collision
			assert not expected.loop

			assert result.success
			assert result.tid == expected.tid
			assert result.hash == expected.hash
			assert not result.duplicate
			assert not result.collision
			assert result.recirc == expected.recirc
			assert not result.loop

			self.cuckoo_hash.delete(expected.tid, expected.hash)
			self.emulator.clear()

class SpecificSingleInsertions(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)

		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.target = gc.Target(device_id=0)

		self.cuckoo_hash = CuckooHash(self.bfrt_info, self.target)
		self.emulator = CuckooHashEmulator()
	
	def runTest(self):
		keys = [0, (1 << KEY_SIZE_BITS) - 1]

		for key in keys:
			expected = self.emulator.insert(key)
			result = send_and_get_result_back(self, OP_WRITE, key)
			value = self.cuckoo_hash.read(expected.tid, expected.hash)
			
			print("[SpecificSingleInsertions]")
			print(f"Key:      0x{key:x}")
			print(f"Expected: tid={expected.tid} hash={hex(expected.hash)}")
			print(f"Read:     0x{value:x}")
			print(f"Result:   {result}")
			print()

			assert expected.success
			assert not expected.duplicate
			assert not expected.collision
			assert not expected.loop

			assert result.success
			assert result.tid == expected.tid
			assert result.hash == expected.hash
			assert not result.duplicate
			assert not result.collision
			assert result.recirc == expected.recirc
			assert not result.loop

			self.cuckoo_hash.delete(expected.tid, expected.hash)
			self.emulator.clear()

class CollisionInsert(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)

		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.target = gc.Target(device_id=0)

		self.cuckoo_hash = CuckooHash(self.bfrt_info, self.target)
		self.emulator = CuckooHashEmulator()
	
	def runTest(self):
		keys = []
		for _ in range(20):
			key0 = custom_random(1, KEY_SIZE_BITS, keys)
			keys.append(key0)

			h0 = self.emulator.hash0(key0)
			while True:
				key1 = custom_random(1, KEY_SIZE_BITS, keys)
				keys.append(key1)

				if h0 == self.emulator.hash0(key1):
					break
			
			print("[CollisionInsert]")
			print(f"Key0:     0x{key0:x}")
			print(f"Key1:     0x{key1:x}")
			print(f"Hash:     0x{h0:x}")

			expected_0 = self.emulator.insert(key0)
			expected_1 = self.emulator.insert(key1)

			assert expected_0.success
			assert not expected_0.duplicate
			assert not expected_0.collision
			assert not expected_0.loop
			assert expected_0.tid == 0
			assert expected_0.hash == h0

			assert expected_1.success
			assert not expected_1.duplicate
			assert expected_1.collision
			assert not expected_1.loop
			assert expected_1.tid == 0
			assert expected_1.hash == h0

			assert self.emulator.get_tid_and_hash(key0)[0] == 1
			assert self.emulator.get_tid_and_hash(key1)[0] == 0

			result0 = send_and_get_result_back(self, OP_WRITE, key0)
			result1 = send_and_get_result_back(self, OP_WRITE, key1)

			print(f"Result 0: {result0}")
			print(f"Result 1: {result1}")
			print()

			assert result0.success
			assert result0.tid == expected_0.tid
			assert result0.hash == expected_0.hash
			assert not result0.duplicate
			assert not result0.collision
			assert result0.recirc == expected_0.recirc

			assert result1.success
			assert result1.tid == expected_1.tid
			assert result1.hash == expected_1.hash
			assert not result1.duplicate
			assert result1.collision
			assert result1.recirc == expected_1.recirc

			assert key1 == self.cuckoo_hash.read(0, self.emulator.hash0(key1))
			assert key0 == self.cuckoo_hash.read(1, self.emulator.hash1(key0))

			self.cuckoo_hash.delete(0, self.emulator.hash0(key1))
			self.cuckoo_hash.delete(1, self.emulator.hash1(key0))
			self.emulator.clear()

class RandomPopulationWithoutLoops(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)

		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.target = gc.Target(device_id=0)

		self.cuckoo_hash = CuckooHash(self.bfrt_info, self.target)
		self.emulator = CuckooHashEmulator()
	
	def runTest(self):
		target_usages = [0.25, 0.5, 0.75, 1.0]
		for usage in target_usages:
			keys = []
			used_keys = []
			while self.emulator.usage() < usage:
				key = custom_random(1, KEY_SIZE_BITS, keys)
				keys.append(key)

				print("[RandomPopulationWithoutLoops]")
				print(f"Usage:    {usage} (current={self.emulator.usage()})")
				print(f"Key:      0x{key:x}")

				expected = self.emulator.insert(key)
				if expected.loop:
					print("*** Loop detected, retrying... ***")
					self.emulator.clear()
					for k in used_keys:
						self.emulator.insert(k)
					continue
				used_keys.append(key)
				
				result = send_and_get_result_back(self, OP_WRITE, key)
				value = self.cuckoo_hash.read(expected.tid, expected.hash)
				
				print(f"Expected: tid={expected.tid} hash={hex(expected.hash)}")
				print(f"Read:     0x{value:x}")
				print(f"Result:   {result}")
				print()

				assert expected.success
				assert not expected.duplicate
				assert not expected.loop

				assert result.success
				assert result.tid == expected.tid
				assert result.hash == expected.hash
				assert not result.duplicate
				assert result.collision == expected.collision
				assert result.recirc == expected.recirc
				assert not result.loop

				# Make the entry never expire
				self.cuckoo_hash.expirator_write(result.tid, result.hash, 0xffffffff)

			print("Clearing the cuckoo hash...")
			self.cuckoo_hash.clear()
			self.emulator.clear()

class ReadAfterWrite(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)

		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.target = gc.Target(device_id=0)

		self.cuckoo_hash = CuckooHash(self.bfrt_info, self.target)
		self.emulator = CuckooHashEmulator()

		usage = 1
		self.keys = []
		while self.emulator.usage() < usage:
			key = custom_random(1, KEY_SIZE_BITS, self.keys)

			expected = self.emulator.insert(key)
			if not expected.success:
				self.emulator.clear()
				for k in self.keys:
					self.emulator.insert(k)
				continue
			
			result = send_and_get_result_back(self, OP_WRITE, key)

			assert result.success
			assert not result.duplicate
			assert not result.loop

			assert expected.tid == result.tid
			assert expected.hash == result.hash

			self.keys.append(key)

			assert key == self.cuckoo_hash.read(result.tid, result.hash)
			assert self.emulator.lookup(key)

			print(f"Inserted t{result.tid}[{hex(result.hash)}] = 0x{key:x}")
			
			# Make the entry never expire
			self.cuckoo_hash.expirator_write(result.tid, result.hash, 0xffffffff)

	def runTest(self):
		for key in self.keys:
			tid, hash = self.emulator.get_tid_and_hash(key)
			result = send_and_get_result_back(self, OP_READ, key)

			print("[ReadAfterWrite]")
			print(f"Key:    0x{key:x}")
			print(f"Tid:    0x{tid}")
			print(f"Hash:   0x{hash:x}")
			print(f"Result: {result}")
			print()

			assert result.success
			assert not result.duplicate
			assert not result.collision
			assert not result.recirc
			assert not result.loop

			value = self.cuckoo_hash.read(tid, hash)
			assert key == value

class AlreadyInserted(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)

		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.target = gc.Target(device_id=0)

		self.cuckoo_hash = CuckooHash(self.bfrt_info, self.target)
		self.emulator = CuckooHashEmulator()

	def runTest(self):
		keys = []
		for _ in range(100):
			key = custom_random(1, KEY_SIZE_BITS, keys)
			keys.append(key)

			expected = self.emulator.insert(key)
			if not expected.success:
				self.emulator.clear()
				for k in keys:
					self.emulator.insert(k)
				continue

			result0 = send_and_get_result_back(self, OP_WRITE, key)
			result1 = send_and_get_result_back(self, OP_WRITE, key)

			print("[AlreadyInserted]")
			print(f"Key:      0x{key:x}")
			print(f"Result 0: {result0}")
			print(f"Result 1: {result1}")
			print()

			assert result0.success
			assert not result0.duplicate
			assert not result0.loop

			assert not result1.success
			assert result1.duplicate
			assert not result1.loop

class CatchingLoops(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)

		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.target = gc.Target(device_id=0)

		self.cuckoo_hash = CuckooHash(self.bfrt_info, self.target)
		self.emulator = CuckooHashEmulator()

		usage = 1
		self.keys = []
		self.used_keys = []
		while self.emulator.usage() < usage:
			key = custom_random(1, KEY_SIZE_BITS, self.keys)
			self.keys.append(key)

			expected = self.emulator.insert(key)
			if not expected.success:
				self.emulator.clear()
				for k in self.used_keys:
					self.emulator.insert(k)
				continue
			self.used_keys.append(key)
			
			result = send_and_get_result_back(self, OP_WRITE, key)

			assert result.success
			assert not result.duplicate
			assert not result.loop

			assert expected.tid == result.tid
			assert expected.hash == result.hash

			assert key == self.cuckoo_hash.read(result.tid, result.hash)
			assert self.emulator.lookup(key)

			print(f"Usage:    {usage} (current={self.emulator.usage()})")
			print(f"Inserted: t{result.tid}[{hex(result.hash)}] = 0x{key:x}")
			print()
			
			# Make the entry never expire
			self.cuckoo_hash.expirator_write(result.tid, result.hash, 0xffffffff)
	
	def runTest(self):
		target_loops = 100
		loops = 0
		while loops < target_loops:
			key = custom_random(1, KEY_SIZE_BITS, self.keys)
			self.keys.append(key)

			expected = self.emulator.insert(key)
			if not expected.loop:
				self.emulator.clear()
				for k in self.used_keys:
					self.emulator.insert(k)
				continue
			self.used_keys.append(key)

			result = send_and_get_result_back(self, OP_WRITE, key)
			loops += 1

			print("[CatchingLoops]")
			print(f"Key:      0x{key:x}")
			print(f"Loop:     {loops}/{target_loops}")
			print(f"Expected: {expected}")
			print(f"Result:   {result}")
			print()

			assert not result.success
			assert result.loop
			assert result.recirc == expected.recirc

class Random(BfRuntimeTest):
	def setUp(self):
		client_id = 0
		BfRuntimeTest.setUp(self, client_id, PROGRAM)

		self.bfrt_info = self.interface.bfrt_info_get(PROGRAM)
		self.target = gc.Target(device_id=0)

		self.cuckoo_hash = CuckooHash(self.bfrt_info, self.target)
		self.emulator = CuckooHashEmulator()
	
	def runTest(self):
		for _ in range(1000):
			key = custom_random(1, KEY_SIZE_BITS)

			expected = self.emulator.insert(key)
			result = send_and_get_result_back(self, OP_WRITE, key)

			# Make the entry never expire
			if result.success:
				self.cuckoo_hash.expirator_write(result.tid, result.hash, 0xffffffff)

			print("[Random]")
			print(f"Key:      0x{key:x}")
			print(f"Expected: {expected}")
			print(f"Result:   {result}")
			print()

			assert result.success == expected.success
			assert result.duplicate == expected.duplicate
			assert result.collision == expected.collision
			assert result.recirc == expected.recirc
			assert result.loop == expected.loop
