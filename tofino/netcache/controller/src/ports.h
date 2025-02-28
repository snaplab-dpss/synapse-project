#pragma once

#include <cstdint>
#include <map>

#include "netcache.h"
#include "table.h"

#include <port_mgr/bf_port_if.h>
#include <bf_pm/bf_pm_intf.h>
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_common.h>

namespace netcache {

class Port_HDL_Info : Table {
private:
  // Key fields IDs
  bf_rt_id_t CONN_ID;
  bf_rt_id_t CHNL_ID;

  // Data field ids
  bf_rt_id_t DEV_PORT;

public:
  Port_HDL_Info(const bfrt::BfRtInfo *bfrt_info, std::shared_ptr<bfrt::BfRtSession> bfrt_session, const bf_rt_target_t &dev_tgt)
      : Table(bfrt_info, bfrt_session, dev_tgt, "$PORT_HDL_INFO") {
    auto bf_status = table->keyFieldIdGet("$CONN_ID", &CONN_ID);
    ASSERT_BF_STATUS(bf_status);
    bf_status = table->keyFieldIdGet("$CHNL_ID", &CHNL_ID);
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->dataFieldIdGet("$DEV_PORT", &DEV_PORT);
    ASSERT_BF_STATUS(bf_status);
  }

  uint16_t get_dev_port(uint16_t front_panel_port, uint16_t lane) {
    uint64_t hwflag = 0;

    key_setup(front_panel_port, lane);

    auto bf_status = table->tableEntryGet(*session, dev_tgt, hwflag, *key, data.get());
    ASSERT_BF_STATUS(bf_status);

    uint64_t value;
    bf_status = data->getValue(DEV_PORT, &value);
    ASSERT_BF_STATUS(bf_status);

    return (uint16_t)value;
  }

private:
  void key_setup(uint16_t front_panel_port, uint16_t lane) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(CONN_ID, static_cast<uint64_t>(front_panel_port));
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->setValue(CHNL_ID, static_cast<uint64_t>(lane));
    ASSERT_BF_STATUS(bf_status);
  }
};

class Port_Stat : Table {
private:
  struct key_fields_t {
    // Key fields IDs
    bf_rt_id_t dev_port;
  };

  struct data_fields_t {
    // Data field ids
    bf_rt_id_t FramesReceivedOK;
    bf_rt_id_t FramesReceivedAll;
    bf_rt_id_t FramesReceivedwithFCSError;
    bf_rt_id_t FrameswithanyError;
    bf_rt_id_t OctetsReceivedinGoodFrames;
    bf_rt_id_t OctetsReceived;
    bf_rt_id_t FramesReceivedwithUnicastAddresses;
    bf_rt_id_t FramesReceivedwithMulticastAddresses;
    bf_rt_id_t FramesReceivedwithBroadcastAddresses;
    bf_rt_id_t FramesReceivedoftypePAUSE;
    bf_rt_id_t FramesReceivedwithLengthError;
    bf_rt_id_t FramesReceivedUndersized;
    bf_rt_id_t FramesReceivedOversized;
    bf_rt_id_t FragmentsReceived;
    bf_rt_id_t JabberReceived;
    bf_rt_id_t PriorityPauseFrames;
    bf_rt_id_t CRCErrorStomped;
    bf_rt_id_t FrameTooLong;
    bf_rt_id_t RxVLANFramesGood;
    bf_rt_id_t FramesDroppedBufferFull;
    bf_rt_id_t FramesReceivedLength_lt_64;
    bf_rt_id_t FramesReceivedLength_eq_64;
    bf_rt_id_t FramesReceivedLength_65_127;
    bf_rt_id_t FramesReceivedLength_128_255;
    bf_rt_id_t FramesReceivedLength_256_511;
    bf_rt_id_t FramesReceivedLength_512_1023;
    bf_rt_id_t FramesReceivedLength_1024_1518;
    bf_rt_id_t FramesReceivedLength_1519_2047;
    bf_rt_id_t FramesReceivedLength_2048_4095;
    bf_rt_id_t FramesReceivedLength_4096_8191;
    bf_rt_id_t FramesReceivedLength_8192_9215;
    bf_rt_id_t FramesReceivedLength_9216;
    bf_rt_id_t FramesTransmittedOK;
    bf_rt_id_t FramesTransmittedAll;
    bf_rt_id_t FramesTransmittedwithError;
    bf_rt_id_t OctetsTransmittedwithouterror;
    bf_rt_id_t OctetsTransmittedTotal;
    bf_rt_id_t FramesTransmittedUnicast;
    bf_rt_id_t FramesTransmittedMulticast;
    bf_rt_id_t FramesTransmittedBroadcast;
    bf_rt_id_t FramesTransmittedPause;
    bf_rt_id_t FramesTransmittedPriPause;
    bf_rt_id_t FramesTransmittedVLAN;
    bf_rt_id_t FramesTransmittedLength_lt_64;
    bf_rt_id_t FramesTransmittedLength_eq_64;
    bf_rt_id_t FramesTransmittedLength_65_127;
    bf_rt_id_t FramesTransmittedLength_128_255;
    bf_rt_id_t FramesTransmittedLength_256_511;
    bf_rt_id_t FramesTransmittedLength_512_1023;
    bf_rt_id_t FramesTransmittedLength_1024_1518;
    bf_rt_id_t FramesTransmittedLength_1519_2047;
    bf_rt_id_t FramesTransmittedLength_2048_4095;
    bf_rt_id_t FramesTransmittedLength_4096_8191;
    bf_rt_id_t FramesTransmittedLength_8192_9215;
    bf_rt_id_t FramesTransmittedLength_9216;
    bf_rt_id_t Pri0FramesTransmitted;
    bf_rt_id_t Pri1FramesTransmitted;
    bf_rt_id_t Pri2FramesTransmitted;
    bf_rt_id_t Pri3FramesTransmitted;
    bf_rt_id_t Pri4FramesTransmitted;
    bf_rt_id_t Pri5FramesTransmitted;
    bf_rt_id_t Pri6FramesTransmitted;
    bf_rt_id_t Pri7FramesTransmitted;
    bf_rt_id_t Pri0FramesReceived;
    bf_rt_id_t Pri1FramesReceived;
    bf_rt_id_t Pri2FramesReceived;
    bf_rt_id_t Pri3FramesReceived;
    bf_rt_id_t Pri4FramesReceived;
    bf_rt_id_t Pri5FramesReceived;
    bf_rt_id_t Pri6FramesReceived;
    bf_rt_id_t Pri7FramesReceived;
    bf_rt_id_t TransmitPri0Pause1USCount;
    bf_rt_id_t TransmitPri1Pause1USCount;
    bf_rt_id_t TransmitPri2Pause1USCount;
    bf_rt_id_t TransmitPri3Pause1USCount;
    bf_rt_id_t TransmitPri4Pause1USCount;
    bf_rt_id_t TransmitPri5Pause1USCount;
    bf_rt_id_t TransmitPri6Pause1USCount;
    bf_rt_id_t TransmitPri7Pause1USCount;
    bf_rt_id_t ReceivePri0Pause1USCount;
    bf_rt_id_t ReceivePri1Pause1USCount;
    bf_rt_id_t ReceivePri2Pause1USCount;
    bf_rt_id_t ReceivePri3Pause1USCount;
    bf_rt_id_t ReceivePri4Pause1USCount;
    bf_rt_id_t ReceivePri5Pause1USCount;
    bf_rt_id_t ReceivePri6Pause1USCount;
    bf_rt_id_t ReceivePri7Pause1USCount;
    bf_rt_id_t ReceiveStandardPause1USCount;
    bf_rt_id_t FramesTruncated;
  };

  key_fields_t key_fields;
  data_fields_t data_fields;

public:
  Port_Stat(const bfrt::BfRtInfo *info, std::shared_ptr<bfrt::BfRtSession> session, const bf_rt_target_t &dev_tgt)
      : Table(info, session, dev_tgt, "$PORT_STAT") {
    init_key({
        {"$DEV_PORT", &key_fields.dev_port},
    });

    init_data({
        {"$FramesReceivedOK", &data_fields.FramesReceivedOK},
        {"$FramesReceivedAll", &data_fields.FramesReceivedAll},
        {"$FramesReceivedwithFCSError", &data_fields.FramesReceivedwithFCSError},
        {"$FrameswithanyError", &data_fields.FrameswithanyError},
        {"$OctetsReceivedinGoodFrames", &data_fields.OctetsReceivedinGoodFrames},
        {"$OctetsReceived", &data_fields.OctetsReceived},
        {"$FramesReceivedwithUnicastAddresses", &data_fields.FramesReceivedwithUnicastAddresses},
        {"$FramesReceivedwithMulticastAddresses", &data_fields.FramesReceivedwithMulticastAddresses},
        {"$FramesReceivedwithBroadcastAddresses", &data_fields.FramesReceivedwithBroadcastAddresses},
        {"$FramesReceivedoftypePAUSE", &data_fields.FramesReceivedoftypePAUSE},
        {"$FramesReceivedwithLengthError", &data_fields.FramesReceivedwithLengthError},
        {"$FramesReceivedUndersized", &data_fields.FramesReceivedUndersized},
        {"$FramesReceivedOversized", &data_fields.FramesReceivedOversized},
        {"$FragmentsReceived", &data_fields.FragmentsReceived},
        {"$JabberReceived", &data_fields.JabberReceived},
        {"$PriorityPauseFrames", &data_fields.PriorityPauseFrames},
        {"$CRCErrorStomped", &data_fields.CRCErrorStomped},
        {"$FrameTooLong", &data_fields.FrameTooLong},
        {"$RxVLANFramesGood", &data_fields.RxVLANFramesGood},
        {"$FramesDroppedBufferFull", &data_fields.FramesDroppedBufferFull},
        {"$FramesReceivedLength_lt_64", &data_fields.FramesReceivedLength_lt_64},
        {"$FramesReceivedLength_eq_64", &data_fields.FramesReceivedLength_eq_64},
        {"$FramesReceivedLength_65_127", &data_fields.FramesReceivedLength_65_127},
        {"$FramesReceivedLength_128_255", &data_fields.FramesReceivedLength_128_255},
        {"$FramesReceivedLength_256_511", &data_fields.FramesReceivedLength_256_511},
        {"$FramesReceivedLength_512_1023", &data_fields.FramesReceivedLength_512_1023},
        {"$FramesReceivedLength_1024_1518", &data_fields.FramesReceivedLength_1024_1518},
        {"$FramesReceivedLength_1519_2047", &data_fields.FramesReceivedLength_1519_2047},
        {"$FramesReceivedLength_2048_4095", &data_fields.FramesReceivedLength_2048_4095},
        {"$FramesReceivedLength_4096_8191", &data_fields.FramesReceivedLength_4096_8191},
        {"$FramesReceivedLength_8192_9215", &data_fields.FramesReceivedLength_8192_9215},
        {"$FramesReceivedLength_9216", &data_fields.FramesReceivedLength_9216},
        {"$FramesTransmittedOK", &data_fields.FramesTransmittedOK},
        {"$FramesTransmittedAll", &data_fields.FramesTransmittedAll},
        {"$FramesTransmittedwithError", &data_fields.FramesTransmittedwithError},
        {"$OctetsTransmittedwithouterror", &data_fields.OctetsTransmittedwithouterror},
        {"$OctetsTransmittedTotal", &data_fields.OctetsTransmittedTotal},
        {"$FramesTransmittedUnicast", &data_fields.FramesTransmittedUnicast},
        {"$FramesTransmittedMulticast", &data_fields.FramesTransmittedMulticast},
        {"$FramesTransmittedBroadcast", &data_fields.FramesTransmittedBroadcast},
        {"$FramesTransmittedPause", &data_fields.FramesTransmittedPause},
        {"$FramesTransmittedPriPause", &data_fields.FramesTransmittedPriPause},
        {"$FramesTransmittedVLAN", &data_fields.FramesTransmittedVLAN},
        {"$FramesTransmittedLength_lt_64", &data_fields.FramesTransmittedLength_lt_64},
        {"$FramesTransmittedLength_eq_64", &data_fields.FramesTransmittedLength_eq_64},
        {"$FramesTransmittedLength_65_127", &data_fields.FramesTransmittedLength_65_127},
        {"$FramesTransmittedLength_128_255", &data_fields.FramesTransmittedLength_128_255},
        {"$FramesTransmittedLength_256_511", &data_fields.FramesTransmittedLength_256_511},
        {"$FramesTransmittedLength_512_1023", &data_fields.FramesTransmittedLength_512_1023},
        {"$FramesTransmittedLength_1024_1518", &data_fields.FramesTransmittedLength_1024_1518},
        {"$FramesTransmittedLength_1519_2047", &data_fields.FramesTransmittedLength_1519_2047},
        {"$FramesTransmittedLength_2048_4095", &data_fields.FramesTransmittedLength_2048_4095},
        {"$FramesTransmittedLength_4096_8191", &data_fields.FramesTransmittedLength_4096_8191},
        {"$FramesTransmittedLength_8192_9215", &data_fields.FramesTransmittedLength_8192_9215},
        {"$FramesTransmittedLength_9216", &data_fields.FramesTransmittedLength_9216},
        {"$Pri0FramesTransmitted", &data_fields.Pri0FramesTransmitted},
        {"$Pri1FramesTransmitted", &data_fields.Pri1FramesTransmitted},
        {"$Pri2FramesTransmitted", &data_fields.Pri2FramesTransmitted},
        {"$Pri3FramesTransmitted", &data_fields.Pri3FramesTransmitted},
        {"$Pri4FramesTransmitted", &data_fields.Pri4FramesTransmitted},
        {"$Pri5FramesTransmitted", &data_fields.Pri5FramesTransmitted},
        {"$Pri6FramesTransmitted", &data_fields.Pri6FramesTransmitted},
        {"$Pri7FramesTransmitted", &data_fields.Pri7FramesTransmitted},
        {"$Pri0FramesReceived", &data_fields.Pri0FramesReceived},
        {"$Pri1FramesReceived", &data_fields.Pri1FramesReceived},
        {"$Pri2FramesReceived", &data_fields.Pri2FramesReceived},
        {"$Pri3FramesReceived", &data_fields.Pri3FramesReceived},
        {"$Pri4FramesReceived", &data_fields.Pri4FramesReceived},
        {"$Pri5FramesReceived", &data_fields.Pri5FramesReceived},
        {"$Pri6FramesReceived", &data_fields.Pri6FramesReceived},
        {"$Pri7FramesReceived", &data_fields.Pri7FramesReceived},
        {"$TransmitPri0Pause1USCount", &data_fields.TransmitPri0Pause1USCount},
        {"$TransmitPri1Pause1USCount", &data_fields.TransmitPri1Pause1USCount},
        {"$TransmitPri2Pause1USCount", &data_fields.TransmitPri2Pause1USCount},
        {"$TransmitPri3Pause1USCount", &data_fields.TransmitPri3Pause1USCount},
        {"$TransmitPri4Pause1USCount", &data_fields.TransmitPri4Pause1USCount},
        {"$TransmitPri5Pause1USCount", &data_fields.TransmitPri5Pause1USCount},
        {"$TransmitPri6Pause1USCount", &data_fields.TransmitPri6Pause1USCount},
        {"$TransmitPri7Pause1USCount", &data_fields.TransmitPri7Pause1USCount},
        {"$ReceivePri0Pause1USCount", &data_fields.ReceivePri0Pause1USCount},
        {"$ReceivePri1Pause1USCount", &data_fields.ReceivePri1Pause1USCount},
        {"$ReceivePri2Pause1USCount", &data_fields.ReceivePri2Pause1USCount},
        {"$ReceivePri3Pause1USCount", &data_fields.ReceivePri3Pause1USCount},
        {"$ReceivePri4Pause1USCount", &data_fields.ReceivePri4Pause1USCount},
        {"$ReceivePri5Pause1USCount", &data_fields.ReceivePri5Pause1USCount},
        {"$ReceivePri6Pause1USCount", &data_fields.ReceivePri6Pause1USCount},
        {"$ReceivePri7Pause1USCount", &data_fields.ReceivePri7Pause1USCount},
        {"$ReceiveStandardPause1USCount", &data_fields.ReceiveStandardPause1USCount},
        {"$FramesTruncated", &data_fields.FramesTruncated},
    });
  }

  uint64_t get_port_rx(uint16_t dev_port) {
    uint64_t hwflag = 0;

    key_setup(dev_port);

    auto bf_status = table->tableEntryGet(*session, dev_tgt, hwflag, *key, data.get());
    ASSERT_BF_STATUS(bf_status);

    uint64_t value;
    bf_status = data->getValue(data_fields.FramesReceivedOK, &value);
    ASSERT_BF_STATUS(bf_status);

    return value;
  }

  uint64_t get_port_tx(uint16_t dev_port) {
    uint64_t hwflag = 0;

    key_setup(dev_port);

    auto bf_status = table->tableEntryGet(*session, dev_tgt, hwflag, *key, data.get());
    ASSERT_BF_STATUS(bf_status);

    uint64_t value;
    bf_status = data->getValue(data_fields.FramesTransmittedOK, &value);
    ASSERT_BF_STATUS(bf_status);

    return value;
  }

  void reset_port() {
    uint64_t hwflag = 0;

    auto bf_status = table->tableClear(*session, dev_tgt, hwflag);
    ASSERT_BF_STATUS(bf_status);
  }

private:
  void key_setup(uint16_t dev_port) {
    table->keyReset(key.get());
    assert(key);

    auto bf_status = key->setValue(key_fields.dev_port, static_cast<uint64_t>(dev_port));
    ASSERT_BF_STATUS(bf_status);
  }
};

class Ports : Table {
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

public:
  Ports(const bfrt::BfRtInfo *bfrt_info, std::shared_ptr<bfrt::BfRtSession> session, const bf_rt_target_t &dev_tgt)
      : Table(bfrt_info, session, dev_tgt, "$PORT"), port_hdl_info(bfrt_info, session, dev_tgt), port_stat(bfrt_info, session, dev_tgt) {
    auto bf_status = table->keyFieldIdGet("$DEV_PORT", &DEV_PORT);
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->dataFieldIdGet("$SPEED", &SPEED);
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->dataFieldIdGet("$FEC", &FEC);
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->dataFieldIdGet("$PORT_ENABLE", &PORT_ENABLE);
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->dataFieldIdGet("$LOOPBACK_MODE", &LOOPBACK_MODE);
    ASSERT_BF_STATUS(bf_status);

    bf_status = table->dataFieldIdGet("$PORT_UP", &PORT_UP);
    ASSERT_BF_STATUS(bf_status);
  }

  void add_dev_port(uint16_t dev_port, bf_port_speed_t speed, bf_loopback_mode_e loopback_mode = BF_LPBK_NONE) {
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

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);
  }

  void add_port(uint16_t front_panel_port, uint16_t lane, bf_port_speed_t speed) {
    auto dev_port = port_hdl_info.get_dev_port(front_panel_port, lane);
    add_dev_port(dev_port, speed);
  }

  bool is_port_up(uint16_t dev_port, bool from_hw = false) {
    auto hwflag = from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(dev_port);

    auto bf_status = table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    ASSERT_BF_STATUS(bf_status);

    bool value;
    bf_status = data->getValue(PORT_UP, &value);
    ASSERT_BF_STATUS(bf_status);

    return value;
  }

  uint16_t get_dev_port(uint16_t front_panel_port, uint16_t lane) { return port_hdl_info.get_dev_port(front_panel_port, lane); }

  uint64_t get_port_rx(uint16_t port) { return port_stat.get_port_rx(port); }
  uint64_t get_port_tx(uint16_t port) { return port_stat.get_port_tx(port); }

  void reset_ports() { port_stat.reset_port(); }

private:
  void key_setup(uint16_t dev_port) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(DEV_PORT, static_cast<uint64_t>(dev_port));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup(std::string speed, std::string fec, bool port_enable, std::string loopback_mode) {
    table->dataReset(data.get());

    auto bf_status = data->setValue(SPEED, speed);
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(FEC, fec);
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(PORT_ENABLE, port_enable);
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(LOOPBACK_MODE, loopback_mode);
    ASSERT_BF_STATUS(bf_status);
  }

public:
  static bf_port_speed_t gbps_to_bf_port_speed(uint32_t gbps) {
    switch (gbps) {
    case 100:
      return BF_SPEED_100G;
    case 50:
      return BF_SPEED_50G;
    case 40:
      return BF_SPEED_40G;
    case 25:
      return BF_SPEED_25G;
    default:
      return BF_SPEED_NONE;
    }
  }
};

}; // namespace netcache
