#pragma once

#include "../primitives/table.h"

namespace sycon {

class ForwardingTbl : public Table {
public:
  enum ForwardingOp {
    FORWARD_NF_DEV = 0,
    FORWARD_TO_CPU = 1,
    RECIRCULATE    = 2,
    DROP           = 3,
  };

  ForwardingTbl() : Table("Ingress.forwarding_tbl") {}

  void add_fwd_nf_dev_entry(u32 nf_dev, u16 dev_port) {
    bf_status_t bf_status;

    const table_field_t fwd_op_field       = get_key_field("fwd_op");
    const table_field_t nf_dev_field       = get_key_field("nf_dev");
    const table_field_t ingress_port_field = get_key_field("meta.ingress_port");

    bf_status = table->keyReset(key.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValue(fwd_op_field.id, ForwardingOp::FORWARD_NF_DEV);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValueandMask(nf_dev_field.id, nf_dev, 0xffff);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValueandMask(ingress_port_field.id, 0, 0);
    ASSERT_BF_STATUS(bf_status);

    const table_action_t fwd_nf_dev = get_action("Ingress.fwd_nf_dev");

    bf_status = table->dataReset(fwd_nf_dev.action_id, data.get());
    ASSERT_BF_STATUS(bf_status);

    assert(fwd_nf_dev.data_fields.size() == 1);
    bf_status = data->setValue(fwd_nf_dev.data_fields[0].id, static_cast<uint64_t>(dev_port));
    ASSERT_BF_STATUS(bf_status);

    uint64_t flags;
    BF_RT_FLAG_INIT(flags);
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);

    bf_status = table->tableEntryAdd(*session, dev_tgt, flags, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

  void add_fwd_to_cpu_entry() {
    bf_status_t bf_status;

    const table_field_t fwd_op_field       = get_key_field("fwd_op");
    const table_field_t nf_dev_field       = get_key_field("nf_dev");
    const table_field_t ingress_port_field = get_key_field("meta.ingress_port");

    bf_status = table->keyReset(key.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValue(fwd_op_field.id, ForwardingOp::FORWARD_TO_CPU);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValueandMask(nf_dev_field.id, 0, 0);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValueandMask(ingress_port_field.id, 0, 0);
    ASSERT_BF_STATUS(bf_status);

    const table_action_t fwd_to_cpu = get_action("Ingress.fwd_to_cpu");

    bf_status = table->dataReset(fwd_to_cpu.action_id, data.get());
    ASSERT_BF_STATUS(bf_status);

    uint64_t flags;
    BF_RT_FLAG_INIT(flags);
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);

    bf_status = table->tableEntryAdd(*session, dev_tgt, flags, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

  void add_recirc_entry(u16 dev_port) {
    bf_status_t bf_status;

    const table_field_t fwd_op_field       = get_key_field("fwd_op");
    const table_field_t nf_dev_field       = get_key_field("nf_dev");
    const table_field_t ingress_port_field = get_key_field("meta.ingress_port");

    bf_status = table->keyReset(key.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValue(fwd_op_field.id, ForwardingOp::RECIRCULATE);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValueandMask(nf_dev_field.id, 0, 0);
    ASSERT_BF_STATUS(bf_status);

    const u16 pipe_mask    = 0b110000000;
    const u16 pipe_masking = dev_port & pipe_mask;

    bf_status = key->setValueandMask(ingress_port_field.id, pipe_masking, pipe_mask);
    ASSERT_BF_STATUS(bf_status);

    const table_action_t fwd = get_action("Ingress.fwd");

    bf_status = table->dataReset(fwd.action_id, data.get());
    ASSERT_BF_STATUS(bf_status);

    assert(fwd.data_fields.size() == 1);
    bf_status = data->setValue(fwd.data_fields[0].id, static_cast<uint64_t>(dev_port));
    ASSERT_BF_STATUS(bf_status);

    uint64_t flags;
    BF_RT_FLAG_INIT(flags);
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);

    bf_status = table->tableEntryAdd(*session, dev_tgt, flags, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

  void add_drop_entry() {
    bf_status_t bf_status;

    const table_field_t fwd_op_field       = get_key_field("fwd_op");
    const table_field_t nf_dev_field       = get_key_field("nf_dev");
    const table_field_t ingress_port_field = get_key_field("meta.ingress_port");

    bf_status = table->keyReset(key.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValue(fwd_op_field.id, ForwardingOp::RECIRCULATE);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValueandMask(nf_dev_field.id, 0, 0);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValueandMask(ingress_port_field.id, 0, 0);
    ASSERT_BF_STATUS(bf_status);

    const table_action_t drop = get_action("Ingress.drop");

    bf_status = table->dataReset(drop.action_id, data.get());
    ASSERT_BF_STATUS(bf_status);

    uint64_t flags;
    BF_RT_FLAG_INIT(flags);
    BF_RT_FLAG_SET(flags, BF_RT_FROM_HW);

    bf_status = table->tableEntryAdd(*session, dev_tgt, flags, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }
};

} // namespace sycon