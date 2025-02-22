#pragma once

#include <port_mgr/bf_port_if.h>

#include <map>

#include "../../include/sycon/log.h"
#include "../../include/sycon/primitives/meta_table.h"

namespace sycon {

class Port_Stat : MetaTable {
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
  Port_Stat() : MetaTable("", "$PORT_STAT") {
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

  u64 get_port_rx(u16 dev_port, bool from_hw = false) {
    bfrt::BfRtTable::BfRtTableGetFlag hwflag =
        from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(dev_port);

    bf_status_t bf_status = table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    ASSERT_BF_STATUS(bf_status);

    u64 value;
    bf_status = data->getValue(data_fields.FramesReceivedOK, &value);
    ASSERT_BF_STATUS(bf_status);

    return value;
  }

  u64 get_port_tx(u16 dev_port, bool from_hw = false) {
    bfrt::BfRtTable::BfRtTableGetFlag hwflag =
        from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(dev_port);

    bf_status_t bf_status = table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    ASSERT_BF_STATUS(bf_status);

    u64 value;
    bf_status = data->getValue(data_fields.FramesTransmittedOK, &value);
    ASSERT_BF_STATUS(bf_status);

    return value;
  }

  void reset_stats() {
    bf_status_t bf_status = table->tableClear(*session, dev_tgt);
    ASSERT_BF_STATUS(bf_status);
  }

private:
  void key_setup(u16 dev_port) {
    table->keyReset(key.get());
    assert(key);

    bf_status_t bf_status = key->setValue(key_fields.dev_port, static_cast<u64>(dev_port));
    ASSERT_BF_STATUS(bf_status);
  }
};

}; // namespace sycon