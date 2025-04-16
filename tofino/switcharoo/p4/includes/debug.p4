#ifndef __DEBUG__
#define __DEBUG__

#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#define DECLARE_DEBUG_VAR(name) 																				\
	Register<bit<32>, bit<1>>(1, 0) debug_##name##_reg; 														\
	RegisterAction<bit<32>, bit<1>, void>(debug_##name##_reg) debug_##name##_inc_reg_action = { 				\
		void apply(inout bit<32> val) {																			\
			val = val + 1;																						\
		}																										\
	};																											\
	action debug_##name##_inc() { 																				\
		debug_##name##_inc_reg_action.execute(0); 																\
	}

#endif