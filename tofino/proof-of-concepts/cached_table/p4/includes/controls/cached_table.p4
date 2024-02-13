#ifndef _CACHED_TABLE_
#define _CACHED_TABLE_

#include "../headers.p4"
#include "../constants.p4"

control Expirator(
    in time_t now,
    in op_t op,
    in hash_t h,
    out bool valid
) {
    Register<time_t, _>(CACHE_CAPACITY, 0) reg;

    RegisterAction<time_t, hash_t, bool>(reg) read_action = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            if (now < alarm) {
                alarm = now + EXPIRATION_TIME;
                alive = true;
            }
        }
    };

    RegisterAction<time_t, hash_t, bool>(reg) write_action = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            if (now < alarm) {
                alive = true;
            }
            alarm = now + EXPIRATION_TIME;
        }
    };

    action read() {
        valid = read_action.execute(h);
    }

    action write() {
        valid = write_action.execute(h);
    }

    apply {
        if (op == READ) {
            read();
        } else {
            write();
        }
    }
}

control Cache(
    in time_t now,
    in op_t op,
    in hash_t h,
    in key_t k,
    inout value_t v,
    out bool hit
) {
    bool match;
    bool valid;

    Expirator() expirator;

    Register<key_t, _>(CACHE_CAPACITY, 0) keys;
    Register<value_t, _>(CACHE_CAPACITY, 0) values;

    RegisterAction<key_t, hash_t, bool>(keys) read_key_action = {
        void apply(inout key_t curr_key, out bool key_match) {
            if (curr_key == k) {
                key_match = true;
            } else {
                key_match = false;
            }
        }
    };

    RegisterAction<value_t, hash_t, value_t>(values) read_value_action = {
        void apply(inout value_t curr_value, out value_t out_value) {
            out_value = curr_value;
        }
    };

    action read_key() {
        match = read_key_action.execute(h);
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

    apply {
        hit = false;
        expirator.apply(now, op, h, valid);

        if (valid) {
            read_key();

            if (match) {
                read_value();
                hit = true;
            }
        } else if (op != READ) {
            write_key();
            write_value();
            hit = true;
        }
    }
}

control CachedTable(
    in time_t now,
    in op_t op,
    in hash_t h,
    in key_t k,
    inout value_t v,
    out bool hit
) {
    Cache() cache;

    action get_value(value_t value) {
        v = value;
    }

    table map {
		key = {
			k: exact;
		}

		actions = {
			get_value;
		}

		size = 65536;
	}

    apply {
        if (map.apply().hit) {
            hit = true;
        } else {
            cache.apply(now, op, h, k, v, hit);
        }
    }
}

#endif /* _CACHED_TABLE_ */