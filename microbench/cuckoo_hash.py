#!/usr/bin/env python3

####################################################################################################
# Description: A simple implementation of a cuckoo hash table.
# 
# Source: Cuckoo hashing, Rasmus Pagh and Flemming Friche Rodler, Journal of Algorithms 51 (2004)
# https://www.sciencedirect.com/science/article/pii/S0196677403001925
#
# Trying to estimate the probability of:
#   1. A collision during insertion
#   2. A double collision during insertion, requiring going back to the first vector
#   3. A loop of collisions, making it impossible to insert the element without resizing and rehashing
#
# These probably depend on the load factor, the number of elements, and the number of vectors.
####################################################################################################

import zlib
import struct
import math
import random
import statistics

from crc import Calculator, Configuration
from prettytable import PrettyTable
from typing import Set, Tuple

CRC1_CALCULATOR = Calculator(
	Configuration(
		width=32,
		polynomial=0x104c11db7,
		reverse_input=True,
		init_value=0x00000000,
		final_xor_value=0xFFFFFFFF,
		reverse_output=True,
	)
)

CRC2_CALCULATOR = Calculator(
	Configuration(
		width=32,
		polynomial=0x8005,
		reverse_input=False,
		init_value=0xFFFF,
		final_xor_value=0x0000,
		reverse_output=False,
	)
)

def random_number(min_bits: int, max_bits: int, forbidden: list[int] = []):
	assert min_bits >= 1
	while True:
		bits = random.randint(min_bits, max_bits - 1)
		value = random.randint(0, (1<<bits)-1)
		if value not in forbidden:
			break
	return value

def hash1(key: int, hash_size_bits: int) -> int:
	input = struct.pack("!I", key)
	# output = CRC1_CALCULATOR.checksum(input)
	output = zlib.crc32(struct.pack("!I", 0x7f4a7c13) + input)
	return output & ((1 << hash_size_bits) - 1)

def hash2(key: int, hash_size_bits: int) -> int:
	input = struct.pack("!I", key)
	# output = CRC1_CALCULATOR.checksum(struct.pack("!I", 0x97c29b3a) + input)
	# output = CRC2_CALCULATOR.checksum(input)
	output = zlib.crc32(struct.pack("!I", 0x97c29b3a) + input)
	return output & ((1 << hash_size_bits) - 1)

class InsertionResult:
	success: bool
	already_inserted: bool
	collision: bool
	backtracks: int
	loop: bool

	def __init__(self):
		self.success = False
		self.already_inserted = False
		self.collision = False
		self.backtracks = 0
		self.loop = False
	
	def __str__(self) -> str:
		str = ""
		str += f"success={self.success} "
		str += f"already_inserted={self.already_inserted} "
		str += f"collision={self.collision} "
		str += f"backtracks={self.backtracks} "
		str += f"loop={self.loop}"
		return str

class CuckooHash:
	def __init__(self, capacity: int):
		assert capacity & (capacity - 1) == 0, "n must be a power of 2"

		self.capacity = capacity
		self.hash_size_bits = int(math.log(capacity, 2))
		self.t1: dict[int, int] = dict()
		self.t2: dict[int, int] = dict()
		self.total_elements = 0
		self.elements: Set[int] = set()
	
	def lookup(self, key: int) -> bool:
		h1 = hash1(key, self.hash_size_bits)
		h2 = hash2(key, self.hash_size_bits)
		if h1 in self.t1 and self.t1[h1] == key:
			return True
		if h2 in self.t2 and self.t2[h2] == key:
			return True
		return False
	
	def delete(self, key: int) -> bool:
		h1 = hash1(key, self.hash_size_bits)
		h2 = hash2(key, self.hash_size_bits)
		if h1 in self.t1 and self.t1[h1] == key:
			del self.t1[h1]
			self.elements.remove(key)
			self.total_elements -= 1
			return True
		if h2 in self.t2 and self.t2[h2] == key:
			del self.t2[h2]
			self.elements.remove(key)
			self.total_elements -= 1
			return True
		return False

	def insert(self, key: int) -> InsertionResult:
		result = InsertionResult()

		if self.lookup(key):
			result.already_inserted = True
			return result

		insertions = set()
		tables = [self.t1, self.t2]
		hashes = [hash1, hash2]
		target_key = key
		target_tid = 0

		while True:
			h = hashes[target_tid](target_key, self.hash_size_bits)

			if (target_key, target_tid) in insertions:
				result.loop = True
				break

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
				result.backtracks += 1
		
		if not result.loop:
			result.success = True
			self.elements.add(key)
			self.total_elements += 1

		# print(result)
		# print(self)

		return result
	
	def assert_invariants(self):
		assert len(self.t1) + len(self.t2) == self.total_elements
		assert len(self.elements) == self.total_elements

		for element in self.elements:
			h1 = hash1(element, self.hash_size_bits)
			h2 = hash2(element, self.hash_size_bits)
			in_t1 = h1 in self.t1 and self.t1[h1] == element
			in_t2 = h2 in self.t2 and self.t2[h2] == element
			assert in_t1 or in_t2, f"Element 0x{element:x} not found in tables"
			assert not (in_t1 and in_t2), f"Element 0x{element:x} found in both tables"
	
	def __str__(self) -> str:
		table = PrettyTable()
		table.field_names = [ "hash", "t1", "t2" ]

		for i in range(self.capacity):
			if i in self.t1 and i in self.t2:
				table.add_row([i, hex(self.t1[i]), hex(self.t2[i])])
			elif i in self.t1:
				table.add_row([i, hex(self.t1[i]), ''])
			elif i in self.t2:
				table.add_row([i, '', hex(self.t2[i])])
			else:
				table.add_row([i, '', ''])
		return str(table)

def build_loaded_cuckoo_hash(capacity: int, load: float) -> CuckooHash:
	cuckoo_hash = CuckooHash(capacity)
	total_elements = int(capacity * load)

	consecutive_fails = 0
	while cuckoo_hash.total_elements < total_elements:
		result = cuckoo_hash.insert(random_number(1, 32, list(cuckoo_hash.elements)))
		consecutive_fails = consecutive_fails + 1 if not result.success else 0
		assert consecutive_fails < 50, "Too many consecutive fails"
	
	cuckoo_hash.assert_invariants()
	return cuckoo_hash

def get_loop_probability(capacity: int, load: float, iterations: int, num_sets: int) -> Tuple[float, float]:
	probabilities = []

	for _ in range(num_sets):
		cuckoo_hash = build_loaded_cuckoo_hash(capacity, load)
		total_loops = 0
		for _ in range(iterations):
			result = cuckoo_hash.insert(random_number(1, 32))
			if result.loop:
				total_loops += 1
			if result.success:
				cuckoo_hash.delete(random.choice(list(cuckoo_hash.elements)))

		probabilities.append(total_loops / iterations)
	
	avg = statistics.mean(probabilities)
	std_dev = statistics.stdev(probabilities)

	return avg, std_dev

def get_estimated_backtracks(capacity: int, load: float, iterations: int) -> Tuple[float, float]:
	backtracks = []

	cuckoo_hash = build_loaded_cuckoo_hash(capacity, load)
	for _ in range(iterations):
		result = cuckoo_hash.insert(random_number(1, 32))
		backtracks.append(result.backtracks)
		if result.success:
			cuckoo_hash.delete(random.choice(list(cuckoo_hash.elements)))
	
	avg = statistics.mean(backtracks)
	std_dev = statistics.stdev(backtracks)

	return avg, std_dev

def get_backtrack_probabilities(capacity: int, load: float, iterations: int) -> dict[int, float]:
	backtrack_counters: dict[int, int] = {}

	cuckoo_hash = build_loaded_cuckoo_hash(capacity, load)
	for _ in range(iterations):
		result = cuckoo_hash.insert(random_number(1, 32))

		if result.backtracks not in backtrack_counters:
			backtrack_counters[result.backtracks] = 0
		else:
			backtrack_counters[result.backtracks] += 1

		if result.success:
			cuckoo_hash.delete(random.choice(list(cuckoo_hash.elements)))
	
	total = sum(backtrack_counters.values())
	probabilities = { k: v / total for k, v in backtrack_counters.items() }

	return probabilities

def basic_test():
	cuckoo_hash = build_loaded_cuckoo_hash(8, 1)

	print()
	print(cuckoo_hash)
	result = cuckoo_hash.insert(0x04)
	print(result)
	print(cuckoo_hash)

def main():
	# capacities = [32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536]
	capacities = [32, 64, 128, 256, 512, 1024, 2048]
	iterations = 10_000
	load = 1
	
	for c in capacities:
		avg, stdev = get_loop_probability(c, load, iterations, 10)
		print(f"{c}\t {avg:.4f}±{stdev:.4f}")

		# avg, stdev = get_estimated_backtracks(c, load, iterations)
		# print(f"{c}\t {avg:.4f}±{stdev:.4f}")

		# probabilities = get_backtrack_probabilities(c, load, iterations)
		# print(f"{c}\t {[(k,probabilities[k]) for k in sorted(probabilities.keys())]}")

	# basic_test()

if __name__ == '__main__':
	main()