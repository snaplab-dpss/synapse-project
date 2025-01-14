#pragma once

#include "../include/sycon/util.hpp"

namespace sycon {

const constexpr u16 IP_PROTO_TCP = 6;
const constexpr u16 IP_PROTO_UDP = 17;

void packet_init(u16 size);

} // namespace sycon