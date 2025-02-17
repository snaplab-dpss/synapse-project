#pragma once

#include "../primitives/table.h"

namespace sycon {

class IngressPortToNFDev : public PrimitiveTable {
private:
  struct key_fields_t {
    // Key fields IDs
    bf_rt_id_t ingress_port;
  } key_fields;

  struct {
    // Data field ids
    bf_rt_id_t nf_dev;
  } data_fields;

  struct actions_t {
    // Actions ids
    bf_rt_id_t set_ingress_dev;
  } actions;

public:
  IngressPortToNFDev() : PrimitiveTable("Ingress", "ingress_port_to_nf_dev") {
    init_key({
        {"ig_intr_md.ingress_port", &key_fields.ingress_port},
    });

    init_actions({
        {"set_ingress_dev", &actions.set_ingress_dev},
    });

    init_data_with_actions({
        {"nf_dev", {actions.set_ingress_dev, &data_fields.nf_dev}},
    });
  }

public:
  void add_entry(u16 ingress_port, u16 nf_dev) {
    key_setup(ingress_port);
    data_setup(nf_dev);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

private:
  void key_setup(u16 ingress_port) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(key_fields.ingress_port, static_cast<u64>(ingress_port));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup(u16 nf_dev) {
    auto bf_status = table->dataReset(actions.set_ingress_dev, data.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.nf_dev, static_cast<u64>(nf_dev));
    ASSERT_BF_STATUS(bf_status);
  }
};

} // namespace sycon