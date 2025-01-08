#!/usr/bin/env python3

import statistics
import random
import multiprocessing
import zlib
from prettytable import PrettyTable

MIN_HASH_SIZE = 16
MAX_HASH_SIZE = 32
TOTAL_ELEMENTS = 65536
ELEMENT_SIZE_BITS = 12 * 8
ITERATIONS = 10

def hash(key: int, hash_size_bits: int) -> int:
	num_bytes = ELEMENT_SIZE_BITS // 8
	input_bytes = key.to_bytes(num_bytes, byteorder='big')
	output = zlib.crc32(input_bytes)
	return output & ((1 << hash_size_bits) - 1)

def random_element(min_bits: int, max_bits: int, forbidden: set[int] = set()):
	assert 1 <= min_bits <= max_bits
	while True:
		bits = random.randint(min_bits, max_bits - 1)
		value = random.randint(0, (1<<bits)-1)
		if value not in forbidden:
			break
	return value

class Data:
	def __init__(self):
		self.hash_sizes = []
		self.result = []
	
	def __str__(self) -> str:
		table = PrettyTable()
		table.field_names = [ "Hash (bits)", "Collisions (%)", "Error (%)" ]

		for hash_size, (collisions, error) in zip(self.hash_sizes, self.result):
			table.add_row([
				hash_size,
				f"{collisions*100:.6f}",
				f"{error*100:.6f}"
			])

		return str(table)

def get_collisions(hash_size: int) -> tuple[float, float]:
	relative_collisions = []

	for _ in range(ITERATIONS):
		elements = set()
		hashes = set()
		collisions = 0

		for _ in range(TOTAL_ELEMENTS):
			element = random_element(1, ELEMENT_SIZE_BITS, elements)
			elements.add(element)
			h = hash(element, hash_size)

			if h in hashes:
				collisions += 1
			else:
				hashes.add(h)
		
		relative_collisions.append(collisions / TOTAL_ELEMENTS)
	
	mean_rel_col = statistics.mean(relative_collisions)
	stdev_rel_col = statistics.stdev(relative_collisions)

	return mean_rel_col, stdev_rel_col

def get_collisions_per_hash_size() -> Data:
	cpus = multiprocessing.cpu_count()

	data = Data()
	hash_sizes = [ i for i in range(MIN_HASH_SIZE, MAX_HASH_SIZE+1) ]

	with multiprocessing.Pool(cpus) as pool:
		result = pool.map(get_collisions, hash_sizes)
		data.hash_sizes = hash_sizes
		data.result = result
	
	return data

def main():
	data = get_collisions_per_hash_size()
	print(data)

if __name__ == "__main__":
	main()
