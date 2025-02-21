#pragma once

#include <port_mgr/bf_port_if.h>

#include <map>

#include "../../include/sycon/log.h"
#include "../../include/sycon/primitives/table.h"

namespace sycon {

class Port_HDL_Info : PrimitiveTable {
private:
  // Key fields IDs
  bf_rt_id_t CONN_ID;
  bf_rt_id_t CHNL_ID;

  // Data field ids
  bf_rt_id_t DEV_PORT;

public:
  Port_HDL_Info() : PrimitiveTable("", "$PORT_HDL_INFO") {
    init_key({
        {"$CONN_ID", &CONN_ID},
        {"$CHNL_ID", &CHNL_ID},
    });

    init_data({
        {"$DEV_PORT", &DEV_PORT},
    });
  }

  u16 get_dev_port(u16 front_panel_port, u16 lane, bool from_hw = false) {
    auto hwflag = from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(front_panel_port, lane);
    bf_status_t bf_status = table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    ASSERT_BF_STATUS(bf_status);

    u64 value;
    bf_status = data->getValue(DEV_PORT, &value);
    ASSERT_BF_STATUS(bf_status);

    return (u16)value;
  }

private:
  void key_setup(u16 front_panel_port, u16 lane) {
    table->keyReset(key.get());

    bf_status_t bf_status = key->setValue(CONN_ID, static_cast<u64>(front_panel_port));
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValue(CHNL_ID, static_cast<u64>(lane));
    ASSERT_BF_STATUS(bf_status);
  }
};

}; // namespace sycon