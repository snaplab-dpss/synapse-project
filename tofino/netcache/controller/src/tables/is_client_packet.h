#pragma once

#include <bits/stdint-uintn.h>
#include "../table.h"

namespace netcache {

class IsClientPacket : public Table {
private:
  struct key_fields_t {
    // Key fields IDs
    bf_rt_id_t ig_port;
  };

  struct actions_t {
    // Actions ids
    bf_rt_id_t set_client_packet;
    bf_rt_id_t set_not_client_packet;
  };

  key_fields_t key_fields;
  actions_t actions;

public:
  IsClientPacket(const bfrt::BfRtInfo *info, std::shared_ptr<bfrt::BfRtSession> session, const bf_rt_target_t &dev_tgt)
      : Table(info, session, dev_tgt, "SwitchIngress.is_client_packet") {
    init_key({
        {"ig_intr_md.ingress_port", &key_fields.ig_port},
    });

    init_actions({
        {"SwitchIngress.set_client_packet", &actions.set_client_packet},
        {"SwitchIngress.set_not_client_packet", &actions.set_not_client_packet},
    });
  }

  void add_client_port(uint32_t ig_port) {
    key_setup(ig_port);
    data_setup(actions.set_client_packet);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

  void add_not_client_port(uint32_t ig_port) {
    key_setup(ig_port);
    data_setup(actions.set_not_client_packet);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

private:
  void key_setup(uint32_t ig_port) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(key_fields.ig_port, static_cast<uint64_t>(ig_port));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup(bf_rt_id_t action) {
    auto bf_status = table->dataReset(action, data.get());
    ASSERT_BF_STATUS(bf_status);
  }
};

}; // namespace netcache
