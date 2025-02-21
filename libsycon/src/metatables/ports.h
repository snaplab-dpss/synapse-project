#pragma once

#include <port_mgr/bf_port_if.h>
#include <unistd.h>

#include <map>

#include "port_hdl_info.h"
#include "port_stat.h"

namespace sycon {

class Ports : PrimitiveTable {
private:
  // Key fields IDs
  bf_rt_id_t DEV_PORT;

  // Data field ids
  bf_rt_id_t SPEED;
  bf_rt_id_t FEC;
  bf_rt_id_t PORT_ENABLE;
  bf_rt_id_t LOOPBACK_MODE;
  bf_rt_id_t PORT_UP;

  Port_HDL_Info port_hdl_info;
  Port_Stat port_stat;

  std::unordered_map<u16, u16> dev_port_to_front_panel_port;

public:
  Ports() : PrimitiveTable("", "$PORT") {
    init_key({
        {"$DEV_PORT", &DEV_PORT},
    });

    init_data({
        {"$SPEED", &SPEED},
        {"$FEC", &FEC},
        {"$PORT_ENABLE", &PORT_ENABLE},
        {"$LOOPBACK_MODE", &LOOPBACK_MODE},
        {"$PORT_UP", &PORT_UP},
    });
  }

  void add_dev_port(u16 dev_port, bf_port_speed_t speed, bf_loopback_mode_e loopback_mode, bool wait_until_ready) {
    std::map<bf_port_speed_t, std::string> speed_opts{
        {BF_SPEED_NONE, "BF_SPEED_10G"}, {BF_SPEED_25G, "BF_SPEED_25G"},   {BF_SPEED_40G, "BF_SPEED_40G"},
        {BF_SPEED_50G, "BF_SPEED_50G"},  {BF_SPEED_100G, "BF_SPEED_100G"},
    };

    std::map<bf_fec_type_t, std::string> fec_opts{
        {BF_FEC_TYP_NONE, "BF_FEC_TYP_NONE"},
        {BF_FEC_TYP_FIRECODE, "BF_FEC_TYP_FIRECODE"},
        {BF_FEC_TYP_REED_SOLOMON, "BF_FEC_TYP_REED_SOLOMON"},
    };

    std::map<bf_port_speed_t, bf_fec_type_t> speed_to_fec{
        {BF_SPEED_NONE, BF_FEC_TYP_NONE}, {BF_SPEED_25G, BF_FEC_TYP_NONE}, {BF_SPEED_40G, BF_FEC_TYP_NONE},
        {BF_SPEED_50G, BF_FEC_TYP_NONE},  {BF_SPEED_50G, BF_FEC_TYP_NONE}, {BF_SPEED_100G, BF_FEC_TYP_REED_SOLOMON},
    };

    std::map<bf_loopback_mode_e, std::string> loopback_mode_opts{
        {BF_LPBK_NONE, "BF_LPBK_NONE"},         {BF_LPBK_MAC_NEAR, "BF_LPBK_MAC_NEAR"},       {BF_LPBK_MAC_FAR, "BF_LPBK_MAC_FAR"},
        {BF_LPBK_PCS_NEAR, "BF_LPBK_PCS_NEAR"}, {BF_LPBK_SERDES_NEAR, "BF_LPBK_SERDES_NEAR"}, {BF_LPBK_SERDES_FAR, "BF_LPBK_SERDES_FAR"},
        {BF_LPBK_PIPE, "BF_LPBK_PIPE"},
    };

    auto fec = speed_to_fec[speed];

    key_setup(dev_port);
    data_setup(speed_opts[speed], fec_opts[fec], true, loopback_mode_opts[loopback_mode]);

    bf_status_t bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);

    while (wait_until_ready && !is_port_up(dev_port)) {
      sleep(1);
    }
  }

  void add_port(u16 front_panel_port, u16 lane, bf_port_speed_t speed, bool wait_until_ready) {
    auto dev_port = port_hdl_info.get_dev_port(front_panel_port, lane, false);
    add_dev_port(dev_port, speed, BF_LPBK_NONE, wait_until_ready);
  }

  bool is_port_up(u16 dev_port, bool from_hw = false) {
    auto hwflag = from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(dev_port);

    bf_status_t bf_status = table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    ASSERT_BF_STATUS(bf_status);

    bool value;
    bf_status = data->getValue(PORT_UP, &value);
    ASSERT_BF_STATUS(bf_status);

    return value;
  }

  u16 get_dev_port(u16 front_panel_port, u16 lane) {
    u16 dev_port                           = port_hdl_info.get_dev_port(front_panel_port, lane, false);
    dev_port_to_front_panel_port[dev_port] = front_panel_port;
    return dev_port;
  }

  u16 get_front_panel_port(u16 dev_port) {
    if (dev_port_to_front_panel_port.find(dev_port) == dev_port_to_front_panel_port.end()) {
      ERROR("Port %u not found", dev_port);
    }
    return dev_port_to_front_panel_port[dev_port];
  }

  u64 get_port_rx(u16 port) { return port_stat.get_port_rx(port); }
  u64 get_port_tx(u16 port) { return port_stat.get_port_tx(port); }
  void reset_stats_on_all_ports(u16 port) { port_stat.reset_stats(); }

private:
  void key_setup(u16 dev_port) {
    table->keyReset(key.get());

    bf_status_t bf_status = key->setValue(DEV_PORT, static_cast<u64>(dev_port));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup(std::string speed, std::string fec, bool port_enable, std::string loopback_mode) {
    table->dataReset(data.get());

    bf_status_t bf_status = data->setValue(SPEED, speed);
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(FEC, fec);
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(PORT_ENABLE, port_enable);
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(LOOPBACK_MODE, loopback_mode);
    ASSERT_BF_STATUS(bf_status);
  }
};

}; // namespace sycon