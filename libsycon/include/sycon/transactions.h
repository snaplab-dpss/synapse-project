#pragma once

namespace sycon {

void begin_transaction();
void end_transaction();

void begin_batch();
void flush_batch();
void end_batch();

}  // namespace sycon