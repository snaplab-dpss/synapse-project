#ifndef _CACHED_TABLE_
#define _CACHED_TABLE_

#include "../headers.p4"
#include "../constants.p4"

#define HASH_SIZE_BITS 16
#define CACHE_CAPACITY (1 << HASH_SIZE_BITS)
#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds

enum bit<8> ct_op_t {
    READ = 1,
    WRITE = 2,
    COND_WRITE = 3,
    DELETE = 4,
    KEY_FROM_VALUE = 5
}

typedef bit<32> key_t;
typedef bit<32> value_t;
typedef bit<HASH_SIZE_BITS> hash_t;
typedef bit<32> time_t;
typedef bit<8> match_counter_t;

const time_t EXPIRATION_TIME = 5 * ONE_SECOND;

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

    Register<key_t, _>(CACHE_CAPACITY, 0) keys;
    Register<value_t, _>(CACHE_CAPACITY, 0) values;

    RegisterAction<key_t, hash_t, match_counter_t>(keys) read_key_action = {
        void apply(inout key_t curr_key, out match_counter_t match) {
            if (curr_key == k) {
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

    action read_key() {
        key_match = key_match + read_key_action.execute(h);
    }

    action read_value() {
        v = read_value_action.execute(h);
    }

    RegisterAction<key_t, hash_t, void>(keys) write_key_action = {
        void apply(inout key_t curr_key) {
            curr_key = k;
        }
    };

    RegisterAction<value_t, hash_t, void>(values) write_value_action = {
        void apply(inout value_t curr_value) {
            curr_value = v;
        }
    };

    action write_key() {
        write_key_action.execute(h);
    }

    action write_value() {
        write_value_action.execute(h);
    }

    // ============================== Table ==============================

    action kv_map_get_value(value_t value) {
        v = value;
    }

    table kv_map {
		key = {
			k: exact;
		}

		actions = {
			kv_map_get_value;
		}

		size = 65536;
	}

    apply {
        success = false;

        bool kv_map_hit = kv_map.apply().hit;

        if (op == ct_op_t.READ) {
            if (kv_map_hit) {
                success = true;
            } else {
                expirator_read();

                if (expirator_valid) {
                    read_key();
                    read_value();

                    if (key_match == 1) {
                        success = true;
                    }
                }
            }
        } else if (op == ct_op_t.WRITE) {
            if (!kv_map_hit) {
                expirator_write();

                if (!expirator_valid) {
                    write_key();
                    write_value();
                    success = true;
                }
            }
        } else if (op == ct_op_t.COND_WRITE) {
            if (kv_map_hit) {
                 success = true;
            } else {
                expirator_write();

                if (!expirator_valid) {
                    write_key();
                    write_value();
                    success = true;
                } else {
                    read_key();
                    read_value();

                    if (key_match == 1) {
                        success = true;
                    }
                }
            }
        } else if (op == ct_op_t.DELETE) {
            if (!kv_map_hit) {
                expirator_delete();
                success = true;
            }
        }
    }
}

#endif /* _CACHED_TABLE_ */