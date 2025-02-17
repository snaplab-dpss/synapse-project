#pragma once

#include "../primitives/table.h"

namespace sycon {

class ForwardNFDev : public PrimitiveTable {
private:
  struct key_fields_t {
    // Key fields IDs
    bf_rt_id_t nf_dev;
  } key_fields;

  struct {
    // Data field ids
    bf_rt_id_t port;
  } data_fields;

  struct actions_t {
    // Actions ids
    bf_rt_id_t fwd;
  } actions;

public:
  ForwardNFDev() : PrimitiveTable("Ingress", "forward_nf_dev") {
    init_key({
        {"nf_dev", &key_fields.nf_dev},
    });

    init_actions({
        {"fwd", &actions.fwd},
    });

    init_data_with_actions({
        {"port", {actions.fwd, &data_fields.port}},
    });
  }

public:
  void add_entry(u16 nf_dev, u16 dev_port) {
    key_setup(nf_dev);
    data_setup(dev_port);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

private:
  void key_setup(u16 nf_dev) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(key_fields.nf_dev, static_cast<u64>(nf_dev));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup(u16 dev_port) {
    auto bf_status = table->dataReset(actions.fwd, data.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.port, static_cast<u64>(dev_port));
    ASSERT_BF_STATUS(bf_status);
  }
};

} // namespace sycon