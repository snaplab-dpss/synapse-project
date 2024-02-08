#ifndef _EXPIRATOR_
#define _EXPIRATOR_

#include "../headers.p4"
#include "../constants.p4"

control Expirator(in time_t now, in key_t key, in op_t op, out bool_t valid) {
    Register<time_t, _>(EXPIRATOR_CAPACITY, 0) reg;

    RegisterAction<time_t, key_t, bool_t>(reg) read_action = {
        void apply(inout time_t alarm, out bool_t alive) {
            alive = 0;
            if (now < alarm) {
                alarm = now + EXPIRATION_TIME;
                alive = 1;
            }
        }
    };

    RegisterAction<time_t, key_t, bool_t>(reg) write_action = {
        void apply(inout time_t alarm, out bool_t alive) {
            alive = 0;
            if (now < alarm) {
                alive = 1;
            }
            alarm = now + EXPIRATION_TIME;
        }
    };

    action read() {
        valid = read_action.execute(key);
    }

    action write() {
        valid = write_action.execute(key);
    }

    apply {
        if (op == READ) {
            read();
        } else {
            write();
        }
    }
}

#endif /* _EXPIRATOR_ */