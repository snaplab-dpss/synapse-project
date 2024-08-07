#ifndef _CACHED_TABLE_
#define _CACHED_TABLE_

#include "constants.p4"

typedef bit<8> match_counter_t;

enum bit<8> ct_op_t {
    READ = 1,
    WRITE = 2,
    COND_WRITE = 3,
    DELETE = 4,
    KEY_FROM_VALUE = 5
}

control CachedTable(
    in time_t now,
    in ct_op_t op,
    in hash_t h,
    in key_t k,
    inout value_t v,
    out bool success
) {
    match_counter_t key_match = 0;
    bool expirator_valid;

    // ============================== Expirator ==============================

    Register<time_t, _>(CACHE_CAPACITY, 0) expirator;

    RegisterAction<time_t, hash_t, bool>(expirator) expirator_read_action = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            if (now < alarm) {
                alive = true;
                alarm = now + EXPIRATION_TIME;
            }
        }
    };

    RegisterAction<time_t, hash_t, bool>(expirator) expirator_write_action = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            if (now < alarm) {
                alive = true;
            }
            alarm = now + EXPIRATION_TIME;
        }
    };

    RegisterAction<time_t, hash_t, bool>(expirator) expirator_delete_action = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            alarm = 0;
        }
    };

    action expirator_read() {
        expirator_valid = expirator_read_action.execute(h);
    }

    action expirator_write() {
        expirator_valid = expirator_write_action.execute(h);
    }

    action expirator_delete() {
        expirator_valid = expirator_delete_action.execute(h);
    }

    // ============================== Cache ==============================

    Register<bit<32>, _>(CACHE_CAPACITY, 0) keys_src_addr;
	Register<bit<32>, _>(CACHE_CAPACITY, 0) keys_dst_addr;
	Register<bit<16>, _>(CACHE_CAPACITY, 0) keys_src_port;
	Register<bit<16>, _>(CACHE_CAPACITY, 0) keys_dst_port;

    Register<value_t, _>(CACHE_CAPACITY, 0) values;

    RegisterAction<bit<32>, hash_t, match_counter_t>(keys_src_addr) read_key_src_addr_action = {
		void apply(inout bit<32> curr_key, out match_counter_t match) {
			if (curr_key == k.src_addr) {
				match = 1;
			} else {
				match = 0;
			}
		}
	};

	RegisterAction<bit<32>, hash_t, match_counter_t>(keys_dst_addr) read_key_dst_addr_action = {
		void apply(inout bit<32> curr_key, out match_counter_t match) {
			if (curr_key == k.dst_addr) {
				match = 1;
			} else {
				match = 0;
			}
		}
	};

	RegisterAction<bit<16>, hash_t, match_counter_t>(keys_src_port) read_key_src_port_action = {
		void apply(inout bit<16> curr_key, out match_counter_t match) {
			if (curr_key == k.src_port) {
				match = 1;
			} else {
				match = 0;
			}
		}
	};

	RegisterAction<bit<16>, hash_t, match_counter_t>(keys_dst_port) read_key_dst_port_action = {
		void apply(inout bit<16> curr_key, out match_counter_t match) {
			if (curr_key == k.dst_port) {
				match = 1;
			} else {
				match = 0;
			}
		}
	};

    RegisterAction<value_t, hash_t, value_t>(values) read_value_action = {
        void apply(inout value_t curr_value, out value_t out_value) {
            out_value = curr_value;
        }
    };

    action read_key_src_addr() {
		key_match = key_match + read_key_src_addr_action.execute(h);
	}

	action read_key_dst_addr() {
		key_match = key_match + read_key_dst_addr_action.execute(h);
	}

	action read_key_src_port() {
		key_match = key_match + read_key_src_port_action.execute(h);
	}

	action read_key_dst_port() {
		key_match = key_match + read_key_dst_port_action.execute(h);
	}

    action read_value() {
        v = read_value_action.execute(h);
    }

    RegisterAction<bit<32>, hash_t, void>(keys_src_addr) write_key_src_addr_action = {
		void apply(inout bit<32> curr_key) {
			curr_key = k.src_addr;
		}
	};

	RegisterAction<bit<32>, hash_t, void>(keys_dst_addr) write_key_dst_addr_action = {
		void apply(inout bit<32> curr_key) {
			curr_key = k.dst_addr;
		}
	};

	RegisterAction<bit<16>, hash_t, void>(keys_src_port) write_key_src_port_action = {
		void apply(inout bit<16> curr_key) {
			curr_key = k.src_port;
		}
	};

	RegisterAction<bit<16>, hash_t, void>(keys_dst_port) write_key_dst_port_action = {
		void apply(inout bit<16> curr_key) {
			curr_key = k.dst_port;
		}
	};

    RegisterAction<value_t, hash_t, void>(values) write_value_action = {
        void apply(inout value_t curr_value) {
            curr_value = v;
        }
    };

	action write_key_src_addr() {
		write_key_src_addr_action.execute(h);
	}

	action write_key_dst_addr() {
		write_key_dst_addr_action.execute(h);
	}

	action write_key_src_port() {
		write_key_src_port_action.execute(h);
	}

	action write_key_dst_port() {
		write_key_dst_port_action.execute(h);
	}

    action write_value() {
        write_value_action.execute(h);
    }

    // ============================== Table ==============================

    action kv_map_get_value(value_t value) {
        v = value;
    }

    table kv_map_read {
		key = {
			k.src_addr: exact;
            k.dst_addr: exact;
            k.src_port: exact;
            k.dst_port: exact;
		}

		actions = {
			kv_map_get_value;
		}

		size = 65536;
	}

    table kv_map_write {
		key = {
			k.src_addr: exact;
            k.dst_addr: exact;
            k.src_port: exact;
            k.dst_port: exact;
		}

		actions = {
			kv_map_get_value;
		}

		size = 65536;
	}

    apply {
        success = false;

        if (op == ct_op_t.READ) {
            bool kv_map_hit = kv_map_read.apply().hit;

            if (kv_map_hit) {
                success = true;
            } else {
                expirator_read();

                if (expirator_valid) {
                    read_key_src_addr();
                    read_key_dst_addr();
                    read_key_src_port();
                    read_key_dst_port();
                    read_value();

                    if (key_match == 4) {
                        success = true;
                    }
                }
            }
        } else if (op == ct_op_t.WRITE) {
            bool kv_map_hit = kv_map_write.apply().hit;

            if (!kv_map_hit) {
                expirator_write();

                if (!expirator_valid) {
                    write_key_src_addr();
                    write_key_dst_addr();
                    write_key_src_port();
                    write_key_dst_port();
                    write_value();
                    success = true;
                }
            }
        } else if (op == ct_op_t.COND_WRITE) {
            bool kv_map_hit = kv_map_write.apply().hit;

            if (kv_map_hit) {
                 success = true;
            } else {
                expirator_write();

                if (!expirator_valid) {
                    write_key_src_addr();
                    write_key_dst_addr();
                    write_key_src_port();
                    write_key_dst_port();
                    write_value();
                    success = true;
                } else {
                    read_key_src_addr();
                    read_key_dst_addr();
                    read_key_src_port();
                    read_key_dst_port();
                    read_value();

                    if (key_match == 4) {
                        success = true;
                    }
                }
            }
        } else if (op == ct_op_t.DELETE) {
            bool kv_map_hit = kv_map_read.apply().hit;

            if (!kv_map_hit) {
                expirator_delete();
                success = true;
            }
        }
    }
}

#endif /* _CACHED_TABLE_ */