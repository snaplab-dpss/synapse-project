#pragma once

#include <bits/stdint-uintn.h>
#include <vector>
#include <algorithm>
#include <cmath>

namespace peregrine {

class MathLookup {

	public:
		MathLookup() {}

		std::vector<std::vector<uint32_t>> sqr(uint32_t N, uint32_t m) {
			std::vector<std::vector<uint32_t>> entries;
			std::vector<std::vector<uint32_t>> entries_final;

			for (int n = 0; n < N; n++) {
				entries = build_entries(N, m, n);
				for (int i = 0; i < entries.size(); i++) {
					uint64_t avg = (uint64_t)get_avg(entries[i], N);
					uint64_t res = avg * avg;

					// Find res's bit length.
					uint64_t bits, var = (res < 0) ? -res : res;
					for(bits = 0; var != 0; ++bits) var >>= 1;

					if (bits <= N) {
						uint32_t res_32 = static_cast<uint32_t>(res);
						entries_final.push_back({
								entries[i][0],
								correct_mask(entries[i][1], N),
								res_32});
					}
				}
			}

			// Sort all entries based on column 0 (value).
			std::reverse(entries_final.begin(), entries_final.end());
			std::sort(entries_final.begin(), entries_final.end(), sort_col);

			return entries_final;
		}

		std::vector<std::vector<uint32_t>> sqrt(uint32_t N, uint32_t m) {
			std::vector<std::vector<uint32_t>> entries;
			std::vector<std::vector<uint32_t>> entries_final;

			for (int n = 0; n < N; n++) {
				entries = build_entries(N, m, n);
				for (int i = 0; i < entries.size(); i++) {
					uint32_t avg = get_avg(entries[i], N);
					uint32_t res = std::sqrt(avg);

					// Find res's bit length.
					unsigned bits, res_len = (res < 0) ? -res : res;
					for(bits = 0; res_len != 0; ++bits) res_len >>= 1;

					if (res_len <= N) {
						entries_final.push_back({entries[i][0], entries[i][1], res});
					}
				}
			}

			return entries_final;
		}

		std::vector<std::vector<uint32_t>> build_entries(uint32_t N, uint32_t m, uint32_t n) {
			// 0 * n ++ 1 ++ (0|1) * min(m-1, N-n-1) ++ x * max(0, N-n-m)
			//                       <----- m0 ---->        <---  m1 --->

			std::vector<std::vector<uint32_t>> entries{ { 1, 0 } };

			uint32_t m0 = std::min(m - 1, N - n - 1);
			uint32_t m1 = std::max<int32_t>(0, N - n - m);

			uint32_t mask = (1 << m1) - 1;

			for (int i = 0; i < m0; i++) {
				std::vector<std::vector<uint32_t>> lead_0;
				std::vector<std::vector<uint32_t>> lead_1;
				auto entries_size = entries.size();

				for (int j = 0; j < entries.size(); j++) {
					lead_0.push_back({entries[j][0] << 1, mask});
					lead_1.push_back({entries[j][0] << 1 | 1, mask});
				}

				entries = lead_0;
				entries.insert( entries.end(), lead_1.begin(), lead_1.end() );
			}

			for (int i = 0; i < entries.size(); i++) {
				entries[i][0] = entries[i][0] << m1;
				entries[i][1] = mask;
			}

			return entries;
		}

		uint32_t get_avg(std::vector<uint32_t> entry, uint32_t N) {
			uint32_t lo = 0;
			uint32_t hi = 0;

			uint32_t m = reverse_bits(entry[1]);
			uint32_t v = reverse_bits(entry[0]);

			for (int i = 0; i < N; i++) {
				if ((m & 1) == 1) {
					lo = (lo << 1);
					hi = (hi << 1) | 1;
				} else {
					lo = (lo << 1) | (v & 1);
					hi = (hi << 1) | (v & 1);
				}

				m >>= 1;
				v >>= 1;
			}

			return (hi + lo) / 2;
		}

		uint32_t reverse_bits(uint32_t n) {
			for (int i = 0, j = 31; i < j; i++, j--) {
				bool i_set = (bool)(n & (1 << i));
				bool j_set = (bool)(n & (1 << j));
				n &= ~(1 << j);
				n &= ~(1 << i);
				if(i_set) n |= (1 << j);
				if(j_set) n |= (1 << i);
			}

			return n;
		}

		uint32_t correct_mask(uint32_t mask_tmp, uint32_t N) {
			uint32_t m = reverse_bits(mask_tmp);
			uint32_t mask = 0;

			for (auto i = 0u; i < N; i++) {
				mask = (mask << 1) | (1 - (m & 1));
				m >>= 1;
			}

			return mask;
		}

		static bool sort_col(const std::vector<uint32_t>& v1,const std::vector<uint32_t>& v2) {
			return v1[0] < v2[0];
		}
};

};
