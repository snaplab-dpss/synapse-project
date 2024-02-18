#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <unordered_set>
#include <vector>

#include <bf_rt/bf_rt.hpp>
#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <port_mgr/bf_port_if.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#include <bf_switchd/bf_switchd.h>
#include <bfutils/bf_utils.h> // required for bfshell
#include <pkt_mgr/pkt_mgr_intf.h>
#ifdef __cplusplus
}
#endif

#define ALL_PIPES 0xffff

#define ENV_VAR_SDE_INSTALL "SDE_INSTALL"
#define PROGRAM_NAME "tofino"

#define SWITCH_PACKET_MAX_BUFFER_SIZE 10000
#define MTU 1500

bf_pkt *tx_pkt = nullptr;
bf_pkt_tx_ring_t tx_ring = BF_PKT_TX_RING_0;

bf_rt_target_t dev_tgt;
const bfrt::BfRtInfo *info;
std::shared_ptr<bfrt::BfRtSession> session;

struct eth_hdr_t {
  uint8_t dst_mac[6];
  uint8_t src_mac[6];
  uint16_t eth_type;
} __attribute__((packed));

struct ipv4_hdr_t {
  uint8_t ihl : 4;
  uint8_t version : 4;
  uint8_t ecn : 2;
  uint8_t dscp : 6;
  uint16_t tot_len;
  uint16_t id;
  uint16_t frag_off;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t check;
  uint32_t src_ip;
  uint32_t dst_ip;
} __attribute__((packed));

struct tcp_hdr_t {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq;
  uint32_t ack_seq;
  uint16_t res1;
  uint16_t window;
  uint16_t check;
  uint16_t urg_ptr;
} __attribute__((packed));

struct pkt_hdr_t {
  struct eth_hdr_t eth_hdr;
  struct ipv4_hdr_t ip_hdr;
  struct tcp_hdr_t tcp_hdr;
} __attribute__((packed));

void pretty_print_pkt(struct pkt_hdr_t *pkt_hdr) {
  printf("###[ Ethernet ]###\n");
  printf("  dst  %02x:%02x:%02x:%02x:%02x:%02x\n", pkt_hdr->eth_hdr.dst_mac[0],
         pkt_hdr->eth_hdr.dst_mac[1], pkt_hdr->eth_hdr.dst_mac[2],
         pkt_hdr->eth_hdr.dst_mac[3], pkt_hdr->eth_hdr.dst_mac[4],
         pkt_hdr->eth_hdr.dst_mac[5]);
  printf("  src  %02x:%02x:%02x:%02x:%02x:%02x\n", pkt_hdr->eth_hdr.src_mac[0],
         pkt_hdr->eth_hdr.src_mac[1], pkt_hdr->eth_hdr.src_mac[2],
         pkt_hdr->eth_hdr.src_mac[3], pkt_hdr->eth_hdr.src_mac[4],
         pkt_hdr->eth_hdr.src_mac[5]);
  printf("  type 0x%x\n", ntohs(pkt_hdr->eth_hdr.eth_type));

  printf("###[ IP ]###\n");
  printf("  ihl     %u\n", (pkt_hdr->ip_hdr.ihl & 0x0f));
  printf("  version %u\n", (pkt_hdr->ip_hdr.ihl & 0xf0) >> 4);
  printf("  tos     %u\n", pkt_hdr->ip_hdr.version);
  printf("  len     %u\n", ntohs(pkt_hdr->ip_hdr.tot_len));
  printf("  id      %u\n", ntohs(pkt_hdr->ip_hdr.id));
  printf("  off     %u\n", ntohs(pkt_hdr->ip_hdr.frag_off));
  printf("  ttl     %u\n", pkt_hdr->ip_hdr.ttl);
  printf("  proto   %u\n", pkt_hdr->ip_hdr.protocol);
  printf("  chksum  0x%x\n", ntohs(pkt_hdr->ip_hdr.check));
  printf("  src     %u.%u.%u.%u\n", (pkt_hdr->ip_hdr.src_ip >> 0) & 0xff,
         (pkt_hdr->ip_hdr.src_ip >> 8) & 0xff,
         (pkt_hdr->ip_hdr.src_ip >> 16) & 0xff,
         (pkt_hdr->ip_hdr.src_ip >> 24) & 0xff);
  printf("  dst     %u.%u.%u.%u\n", (pkt_hdr->ip_hdr.dst_ip >> 0) & 0xff,
         (pkt_hdr->ip_hdr.dst_ip >> 8) & 0xff,
         (pkt_hdr->ip_hdr.dst_ip >> 16) & 0xff,
         (pkt_hdr->ip_hdr.dst_ip >> 24) & 0xff);

  printf("###[ UDP ]###\n");
  printf("  sport   %u\n", ntohs(pkt_hdr->tcp_hdr.src_port));
  printf("  dport   %u\n", ntohs(pkt_hdr->tcp_hdr.dst_port));
}

char *get_env_var_value(const char *env_var) {
  auto env_var_value = getenv(env_var);

  if (!env_var_value) {
    std::cerr << env_var << " env var not found.\n";
    exit(1);
  }

  return env_var_value;
}

char *get_install_dir() { return get_env_var_value(ENV_VAR_SDE_INSTALL); }

typedef uint64_t time_ns_t;

time_ns_t get_time() {
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1000000000ul + tp.tv_nsec;
}

bool nf_process(time_ns_t now, uint8_t *pkt, uint32_t packet_size);

void pcie_tx(bf_dev_id_t device, uint8_t *pkt, uint32_t packet_size) {
  if (bf_pkt_data_copy(tx_pkt, pkt, packet_size) != 0) {
    fprintf(stderr, "bf_pkt_data_copy failed: pkt_size=%d\n", packet_size);
    bf_pkt_free(device, tx_pkt);
    return;
  }

  if (bf_pkt_tx(device, tx_pkt, tx_ring, (void *)tx_pkt) != BF_SUCCESS) {
    fprintf(stderr, "bf_pkt_tx failed\n");
    bf_pkt_free(device, tx_pkt);
  }
}

bf_status_t txComplete(bf_dev_id_t device, bf_pkt_tx_ring_t tx_ring,
                       uint64_t tx_cookie, uint32_t status) {
  return BF_SUCCESS;
}

bf_status_t pcie_rx(bf_dev_id_t device, bf_pkt *pkt, void *data,
                    bf_pkt_rx_ring_t rx_ring) {
  bf_pkt *orig_pkt = nullptr;
  char in_packet[SWITCH_PACKET_MAX_BUFFER_SIZE];
  char *pkt_buf = nullptr;
  char *bufp = nullptr;
  uint32_t packet_size = 0;
  uint16_t pkt_len = 0;

  // save a pointer to the packet
  orig_pkt = pkt;

  // assemble the received packet
  bufp = &in_packet[0];

  do {
    pkt_buf = (char *)bf_pkt_get_pkt_data(pkt);
    pkt_len = bf_pkt_get_pkt_size(pkt);

    if ((packet_size + pkt_len) > SWITCH_PACKET_MAX_BUFFER_SIZE) {
      fprintf(stderr, "Packet too large to Transmit - SKipping\n");
      break;
    }

    memcpy(bufp, pkt_buf, pkt_len);
    bufp += pkt_len;
    packet_size += pkt_len;
    pkt = bf_pkt_get_nextseg(pkt);
  } while (pkt);

  auto atomic = true;
  auto hw_sync = true;

  auto now = get_time();

  session->beginTransaction(atomic);
  auto fwd = nf_process(now, (uint8_t *)in_packet, packet_size);
  session->commitTransaction(hw_sync);

  if (fwd) {
    pcie_tx(device, (uint8_t *)in_packet, packet_size);
  }

  bf_pkt_free(device, orig_pkt);

  return BF_SUCCESS;
}

void register_pcie_pkt_ops() {
  if (bf_pkt_alloc(dev_tgt.dev_id, &tx_pkt, MTU, BF_DMA_CPU_PKT_TRANSMIT_0) !=
      0) {
    fprintf(stderr, "Failed to allocate packet buffer\n");
    exit(1);
  }

  // register callback for TX complete
  for (int tx_ring = BF_PKT_TX_RING_0; tx_ring < BF_PKT_TX_RING_MAX;
       tx_ring++) {
    bf_pkt_tx_done_notif_register(dev_tgt.dev_id, txComplete,
                                  (bf_pkt_tx_ring_t)tx_ring);
  }

  // register callback for RX
  for (int rx_ring = BF_PKT_RX_RING_0; rx_ring < BF_PKT_RX_RING_MAX;
       rx_ring++) {
    auto status = bf_pkt_rx_register(dev_tgt.dev_id, pcie_rx,
                                     (bf_pkt_rx_ring_t)rx_ring, 0);
    if (status != BF_SUCCESS) {
      fprintf(stderr, "Failed to register pcie callback\n");
      exit(1);
    }
  }
}

void init_bf_switchd() {
  auto switchd_main_ctx =
      (bf_switchd_context_t *)calloc(1, sizeof(bf_switchd_context_t));

  /* Allocate memory to hold switchd configuration and state */
  if (switchd_main_ctx == NULL) {
    std::cerr << "ERROR: Failed to allocate memory for switchd context\n";
    exit(1);
  }

  char target_conf_file[100];
  sprintf(target_conf_file, "%s/share/p4/targets/tofino/%s.conf",
          get_install_dir(), PROGRAM_NAME);

  memset(switchd_main_ctx, 0, sizeof(bf_switchd_context_t));

  switchd_main_ctx->install_dir = get_install_dir();
  switchd_main_ctx->conf_file = const_cast<char *>(target_conf_file);
  switchd_main_ctx->skip_p4 = false;
  switchd_main_ctx->skip_port_add = false;
  switchd_main_ctx->running_in_background = false;
  switchd_main_ctx->dev_sts_thread = false;

  auto bf_status = bf_switchd_lib_init(switchd_main_ctx);

  if (bf_status != BF_SUCCESS) {
    exit(1);
  }
}

void setup_bf_session() {
  dev_tgt.dev_id = 0;
  dev_tgt.pipe_id = ALL_PIPES;

  // Get devMgr singleton instance
  auto &devMgr = bfrt::BfRtDevMgr::getInstance();

  // Get info object from dev_id and p4 program name
  auto bf_status = devMgr.bfRtInfoGet(dev_tgt.dev_id, PROGRAM_NAME, &info);
  assert(bf_status == BF_SUCCESS);

  // Create a session object
  session = bfrt::BfRtSession::sessionCreate();
}

template <int key_size> bool key_eq(void *k1, void *k2) {
  auto _k1 = (uint8_t *)k1;
  auto _k2 = (uint8_t *)k2;

  for (auto i = 0; i < key_size; i++) {
    if (_k1[i] != _k2[i]) {
      return false;
    }
  }

  return true;
}

template <int key_size> unsigned key_hash(void *k) {
  auto _k = (uint8_t *)k;
  unsigned hash = 0;

  for (auto i = 0; i < key_size; i++) {
    hash = __builtin_ia32_crc32si(hash, _k[i]);
  }

  return hash;
}

class Table {
protected:
  std::string table_name;
  const bfrt::BfRtTable *table;

  std::unique_ptr<bfrt::BfRtTableKey> key;
  std::unique_ptr<bfrt::BfRtTableData> data;

protected:
  Table(const std::string &_table_name)
      : table_name(_table_name), table(nullptr) {
    assert(info);
    assert(session);

    auto bf_status = info->bfrtTableFromNameGet(table_name, &table);
    assert(bf_status == BF_SUCCESS);

    // Allocate key and data once, and use reset across different uses
    bf_status = table->keyAllocate(&key);
    assert(bf_status == BF_SUCCESS);
    assert(key);

    bf_status = table->dataAllocate(&data);
    assert(bf_status == BF_SUCCESS);
    assert(data);
  }

  void init_key(const std::string &name, bf_rt_id_t *id) {
    auto bf_status = table->keyFieldIdGet(name, id);

    if (bf_status != BF_SUCCESS) {
      std::cerr << "Error configuring table key " << name << "\n";
      exit(1);
    }
  }

  void init_key(std::unordered_map<std::string, bf_rt_id_t *> fields) {
    for (const auto &field : fields) {
      init_key(field.first, field.second);
    }
  }

  void init_data(const std::string &name, bf_rt_id_t *id) {
    auto bf_status = table->dataFieldIdGet(name, id);

    if (bf_status != BF_SUCCESS) {
      std::cerr << "Error configuring table data " << name << "\n";
      exit(1);
    }
  }

  void init_data(std::unordered_map<std::string, bf_rt_id_t *> fields) {
    for (const auto &field : fields) {
      init_data(field.first, field.second);
    }
  }

  void init_data_with_action(const std::string &name, bf_rt_id_t action_id,
                             bf_rt_id_t *field_id) {
    auto bf_status = table->dataFieldIdGet(name, action_id, field_id);

    if (bf_status != BF_SUCCESS) {
      std::cerr << "Error configuring table data with actions " << name << "\n";
      exit(1);
    }
  }

  void init_data_with_actions(
      std::unordered_map<std::string, std::pair<bf_rt_id_t, bf_rt_id_t *>>
          fields) {
    for (const auto &field : fields) {
      init_data_with_action(field.first, field.second.first,
                            field.second.second);
    }
  }

  void init_action(const std::string &name, bf_rt_id_t *action_id) {
    auto bf_status = table->actionIdGet(name, action_id);

    if (bf_status != BF_SUCCESS) {
      std::cerr << "Error configuring table action " << name << "\n";
      exit(1);
    }
  }

  void init_actions(std::unordered_map<std::string, bf_rt_id_t *> actions) {
    for (const auto &action : actions) {
      init_action(action.first, action.second);
    }
  }

  size_t get_size() const {
    size_t size;
    auto bf_status = table->tableSizeGet(*session, dev_tgt, &size);
    assert(bf_status == BF_SUCCESS);
    return size;
  }

  size_t get_usage() const {
    uint32_t usage;
    auto bf_status = table->tableUsageGet(
        *session, dev_tgt, bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW,
        &usage);
    assert(bf_status == BF_SUCCESS);
    return usage;
  }

public:
  void dump_data_fields();
  void dump_data_fields(std::ostream &);

  void dump();
  void dump(std::ostream &);

  static void dump_table_names(const bfrt::BfRtInfo *bfrtInfo);
};

class Port_HDL_Info : Table {
private:
  // Key fields IDs
  bf_rt_id_t CONN_ID;
  bf_rt_id_t CHNL_ID;

  // Data field ids
  bf_rt_id_t DEV_PORT;

public:
  Port_HDL_Info() : Table("$PORT_HDL_INFO") {
    auto bf_status = table->keyFieldIdGet("$CONN_ID", &CONN_ID);
    assert(bf_status == BF_SUCCESS);
    bf_status = table->keyFieldIdGet("$CHNL_ID", &CHNL_ID);
    assert(bf_status == BF_SUCCESS);

    bf_status = table->dataFieldIdGet("$DEV_PORT", &DEV_PORT);
    assert(bf_status == BF_SUCCESS);
  }

  uint16_t get_dev_port(uint16_t front_panel_port, uint16_t lane,
                        bool from_hw = false) {
    auto hwflag = from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW
                          : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(front_panel_port, lane);
    auto bf_status =
        table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    assert(bf_status == BF_SUCCESS);

    uint64_t value;
    bf_status = data->getValue(DEV_PORT, &value);
    assert(bf_status == BF_SUCCESS);

    return (uint16_t)value;
  }

private:
  void key_setup(uint16_t front_panel_port, uint16_t lane) {
    table->keyReset(key.get());

    auto bf_status =
        key->setValue(CONN_ID, static_cast<uint64_t>(front_panel_port));
    assert(bf_status == BF_SUCCESS);

    bf_status = key->setValue(CHNL_ID, static_cast<uint64_t>(lane));
    assert(bf_status == BF_SUCCESS);
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
  Port_Stat() : Table("$PORT_STAT") {
    init_key({
        {"$DEV_PORT", &key_fields.dev_port},
    });

    init_data({
        {"$FramesReceivedOK", &data_fields.FramesReceivedOK},
        {"$FramesReceivedAll", &data_fields.FramesReceivedAll},
        {"$FramesReceivedwithFCSError",
         &data_fields.FramesReceivedwithFCSError},
        {"$FrameswithanyError", &data_fields.FrameswithanyError},
        {"$OctetsReceivedinGoodFrames",
         &data_fields.OctetsReceivedinGoodFrames},
        {"$OctetsReceived", &data_fields.OctetsReceived},
        {"$FramesReceivedwithUnicastAddresses",
         &data_fields.FramesReceivedwithUnicastAddresses},
        {"$FramesReceivedwithMulticastAddresses",
         &data_fields.FramesReceivedwithMulticastAddresses},
        {"$FramesReceivedwithBroadcastAddresses",
         &data_fields.FramesReceivedwithBroadcastAddresses},
        {"$FramesReceivedoftypePAUSE", &data_fields.FramesReceivedoftypePAUSE},
        {"$FramesReceivedwithLengthError",
         &data_fields.FramesReceivedwithLengthError},
        {"$FramesReceivedUndersized", &data_fields.FramesReceivedUndersized},
        {"$FramesReceivedOversized", &data_fields.FramesReceivedOversized},
        {"$FragmentsReceived", &data_fields.FragmentsReceived},
        {"$JabberReceived", &data_fields.JabberReceived},
        {"$PriorityPauseFrames", &data_fields.PriorityPauseFrames},
        {"$CRCErrorStomped", &data_fields.CRCErrorStomped},
        {"$FrameTooLong", &data_fields.FrameTooLong},
        {"$RxVLANFramesGood", &data_fields.RxVLANFramesGood},
        {"$FramesDroppedBufferFull", &data_fields.FramesDroppedBufferFull},
        {"$FramesReceivedLength_lt_64",
         &data_fields.FramesReceivedLength_lt_64},
        {"$FramesReceivedLength_eq_64",
         &data_fields.FramesReceivedLength_eq_64},
        {"$FramesReceivedLength_65_127",
         &data_fields.FramesReceivedLength_65_127},
        {"$FramesReceivedLength_128_255",
         &data_fields.FramesReceivedLength_128_255},
        {"$FramesReceivedLength_256_511",
         &data_fields.FramesReceivedLength_256_511},
        {"$FramesReceivedLength_512_1023",
         &data_fields.FramesReceivedLength_512_1023},
        {"$FramesReceivedLength_1024_1518",
         &data_fields.FramesReceivedLength_1024_1518},
        {"$FramesReceivedLength_1519_2047",
         &data_fields.FramesReceivedLength_1519_2047},
        {"$FramesReceivedLength_2048_4095",
         &data_fields.FramesReceivedLength_2048_4095},
        {"$FramesReceivedLength_4096_8191",
         &data_fields.FramesReceivedLength_4096_8191},
        {"$FramesReceivedLength_8192_9215",
         &data_fields.FramesReceivedLength_8192_9215},
        {"$FramesReceivedLength_9216", &data_fields.FramesReceivedLength_9216},
        {"$FramesTransmittedOK", &data_fields.FramesTransmittedOK},
        {"$FramesTransmittedAll", &data_fields.FramesTransmittedAll},
        {"$FramesTransmittedwithError",
         &data_fields.FramesTransmittedwithError},
        {"$OctetsTransmittedwithouterror",
         &data_fields.OctetsTransmittedwithouterror},
        {"$OctetsTransmittedTotal", &data_fields.OctetsTransmittedTotal},
        {"$FramesTransmittedUnicast", &data_fields.FramesTransmittedUnicast},
        {"$FramesTransmittedMulticast",
         &data_fields.FramesTransmittedMulticast},
        {"$FramesTransmittedBroadcast",
         &data_fields.FramesTransmittedBroadcast},
        {"$FramesTransmittedPause", &data_fields.FramesTransmittedPause},
        {"$FramesTransmittedPriPause", &data_fields.FramesTransmittedPriPause},
        {"$FramesTransmittedVLAN", &data_fields.FramesTransmittedVLAN},
        {"$FramesTransmittedLength_lt_64",
         &data_fields.FramesTransmittedLength_lt_64},
        {"$FramesTransmittedLength_eq_64",
         &data_fields.FramesTransmittedLength_eq_64},
        {"$FramesTransmittedLength_65_127",
         &data_fields.FramesTransmittedLength_65_127},
        {"$FramesTransmittedLength_128_255",
         &data_fields.FramesTransmittedLength_128_255},
        {"$FramesTransmittedLength_256_511",
         &data_fields.FramesTransmittedLength_256_511},
        {"$FramesTransmittedLength_512_1023",
         &data_fields.FramesTransmittedLength_512_1023},
        {"$FramesTransmittedLength_1024_1518",
         &data_fields.FramesTransmittedLength_1024_1518},
        {"$FramesTransmittedLength_1519_2047",
         &data_fields.FramesTransmittedLength_1519_2047},
        {"$FramesTransmittedLength_2048_4095",
         &data_fields.FramesTransmittedLength_2048_4095},
        {"$FramesTransmittedLength_4096_8191",
         &data_fields.FramesTransmittedLength_4096_8191},
        {"$FramesTransmittedLength_8192_9215",
         &data_fields.FramesTransmittedLength_8192_9215},
        {"$FramesTransmittedLength_9216",
         &data_fields.FramesTransmittedLength_9216},
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
        {"$ReceiveStandardPause1USCount",
         &data_fields.ReceiveStandardPause1USCount},
        {"$FramesTruncated", &data_fields.FramesTruncated},
    });
  }

  uint64_t get_port_rx(uint16_t dev_port, bool from_hw = false) {
    auto hwflag = from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW
                          : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(dev_port);

    auto bf_status =
        table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    assert(bf_status == BF_SUCCESS);

    uint64_t value;
    bf_status = data->getValue(data_fields.FramesReceivedOK, &value);
    assert(bf_status == BF_SUCCESS);

    return value;
  }

  uint64_t get_port_tx(uint16_t dev_port, bool from_hw = false) {
    auto hwflag = from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW
                          : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(dev_port);

    auto bf_status =
        table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    assert(bf_status == BF_SUCCESS);

    uint64_t value;
    bf_status = data->getValue(data_fields.FramesTransmittedOK, &value);
    assert(bf_status == BF_SUCCESS);

    return value;
  }

private:
  void key_setup(uint16_t dev_port) {
    table->keyReset(key.get());
    assert(key);

    auto bf_status =
        key->setValue(key_fields.dev_port, static_cast<uint64_t>(dev_port));
    assert(bf_status == BF_SUCCESS);
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
  Ports() : Table("$PORT"), port_hdl_info(), port_stat() {
    auto bf_status = table->keyFieldIdGet("$DEV_PORT", &DEV_PORT);
    assert(bf_status == BF_SUCCESS);

    bf_status = table->dataFieldIdGet("$SPEED", &SPEED);
    assert(bf_status == BF_SUCCESS);

    bf_status = table->dataFieldIdGet("$FEC", &FEC);
    assert(bf_status == BF_SUCCESS);

    bf_status = table->dataFieldIdGet("$PORT_ENABLE", &PORT_ENABLE);
    assert(bf_status == BF_SUCCESS);

    bf_status = table->dataFieldIdGet("$LOOPBACK_MODE", &LOOPBACK_MODE);
    assert(bf_status == BF_SUCCESS);

    bf_status = table->dataFieldIdGet("$PORT_UP", &PORT_UP);
    assert(bf_status == BF_SUCCESS);
  }

  void add_dev_port(uint16_t dev_port, bf_port_speed_t speed,
                    bf_loopback_mode_e loopback_mode = BF_LPBK_NONE) {
    std::map<bf_port_speed_t, std::string> speed_opts{
        {BF_SPEED_NONE, "BF_SPEED_10G"},  {BF_SPEED_25G, "BF_SPEED_25G"},
        {BF_SPEED_40G, "BF_SPEED_40G"},   {BF_SPEED_50G, "BF_SPEED_50G"},
        {BF_SPEED_100G, "BF_SPEED_100G"},
    };

    std::map<bf_fec_type_t, std::string> fec_opts{
        {BF_FEC_TYP_NONE, "BF_FEC_TYP_NONE"},
        {BF_FEC_TYP_FIRECODE, "BF_FEC_TYP_FIRECODE"},
        {BF_FEC_TYP_REED_SOLOMON, "BF_FEC_TYP_REED_SOLOMON"},
    };

    std::map<bf_port_speed_t, bf_fec_type_t> speed_to_fec{
        {BF_SPEED_NONE, BF_FEC_TYP_NONE},
        {BF_SPEED_25G, BF_FEC_TYP_NONE},
        {BF_SPEED_40G, BF_FEC_TYP_NONE},
        {BF_SPEED_50G, BF_FEC_TYP_NONE},
        {BF_SPEED_50G, BF_FEC_TYP_NONE},
        {BF_SPEED_100G, BF_FEC_TYP_REED_SOLOMON},
    };

    std::map<bf_loopback_mode_e, std::string> loopback_mode_opts{
        {BF_LPBK_NONE, "BF_LPBK_NONE"},
        {BF_LPBK_MAC_NEAR, "BF_LPBK_MAC_NEAR"},
        {BF_LPBK_MAC_FAR, "BF_LPBK_MAC_FAR"},
        {BF_LPBK_PCS_NEAR, "BF_LPBK_PCS_NEAR"},
        {BF_LPBK_SERDES_NEAR, "BF_LPBK_SERDES_NEAR"},
        {BF_LPBK_SERDES_FAR, "BF_LPBK_SERDES_FAR"},
        {BF_LPBK_PIPE, "BF_LPBK_PIPE"},
    };

    auto fec = speed_to_fec[speed];

    key_setup(dev_port);
    data_setup(speed_opts[speed], fec_opts[fec], true,
               loopback_mode_opts[loopback_mode]);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    assert(bf_status == BF_SUCCESS);
  }

  void add_port(uint16_t front_panel_port, uint16_t lane,
                bf_port_speed_t speed) {
    auto dev_port = port_hdl_info.get_dev_port(front_panel_port, lane, false);
    add_dev_port(dev_port, speed);
  }

  bool is_port_up(uint16_t dev_port, bool from_hw = false) {
    auto hwflag = from_hw ? bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW
                          : bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    key_setup(dev_port);

    auto bf_status =
        table->tableEntryGet(*session, dev_tgt, *key, hwflag, data.get());
    assert(bf_status == BF_SUCCESS);

    bool value;
    bf_status = data->getValue(PORT_UP, &value);
    assert(bf_status == BF_SUCCESS);

    return value;
  }

  uint16_t get_dev_port(uint16_t front_panel_port, uint16_t lane) {
    return port_hdl_info.get_dev_port(front_panel_port, lane, false);
  }

  uint64_t get_port_rx(uint16_t port) { return port_stat.get_port_rx(port); }
  uint64_t get_port_tx(uint16_t port) { return port_stat.get_port_tx(port); }

private:
  void key_setup(uint16_t dev_port) {
    table->keyReset(key.get());

    auto bf_status = key->setValue(DEV_PORT, static_cast<uint64_t>(dev_port));
    assert(bf_status == BF_SUCCESS);
  }

  void data_setup(std::string speed, std::string fec, bool port_enable,
                  std::string loopback_mode) {
    table->dataReset(data.get());

    auto bf_status = data->setValue(SPEED, speed);
    assert(bf_status == BF_SUCCESS);

    bf_status = data->setValue(FEC, fec);
    assert(bf_status == BF_SUCCESS);

    bf_status = data->setValue(PORT_ENABLE, port_enable);
    assert(bf_status == BF_SUCCESS);

    bf_status = data->setValue(LOOPBACK_MODE, loopback_mode);
    assert(bf_status == BF_SUCCESS);
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

typedef uint64_t field_value_t;

struct fields_values_t {
  uint32_t size;
  field_value_t *values;

  fields_values_t(uint32_t _size)
      : size(_size), values(new field_value_t[_size]) {}

  fields_values_t(const fields_values_t &key)
      : size(key.size), values(new field_value_t[key.size]) {
    for (auto i = 0u; i < size; i++) {
      values[i] = key.values[i];
    }
  }

  ~fields_values_t() { delete values; }

  struct hash {
    std::size_t operator()(const fields_values_t &k) const {
      int hash = 0;

      for (auto i = 0u; i < k.size; i++) {
        hash ^= std::hash<int>()(k.values[i]);
      }

      return hash;
    }
  };
};

inline bool operator==(const fields_values_t &lhs, const fields_values_t &rhs) {
  if (lhs.size != rhs.size) {
    return false;
  }

  for (auto i = 0u; i < lhs.size; i++) {
    if (lhs.values[i] != rhs.values[i]) {
      return false;
    }
  }

  return true;
}

class SynapseTable : Table {
private:
  struct field_t {
    const std::string name;
    bf_rt_id_t id;

    field_t(const std::string &_name, bf_rt_id_t _id) : name(_name), id(_id) {}
  };

  std::vector<field_t> key_fields;
  bf_rt_id_t populate_action_id;
  std::vector<field_t> param_fields;
  std::unordered_set<fields_values_t, fields_values_t::hash> set_keys;

public:
  SynapseTable(const std::string _table_name,
               const std::vector<std::string> _key_fields_names,
               const std::vector<std::string> _param_fields_names)
      : Table("Ingress." + _table_name) {
    for (auto key_field_name : _key_fields_names) {
      bf_rt_id_t field_id;
      init_key(key_field_name, &field_id);
      key_fields.emplace_back(key_field_name, field_id);
    }

    init_action("Ingress.map_populate", &populate_action_id);

    for (auto param_field_name : _param_fields_names) {
      bf_rt_id_t field_id;
      init_data_with_action(param_field_name, populate_action_id, &field_id);
      param_fields.emplace_back(param_field_name, field_id);
    }
  }

  bool contains(const fields_values_t &key_fields_values) const {
    return set_keys.find(key_fields_values) != set_keys.end();
  }

  bool add(const fields_values_t &key_fields_values,
           const fields_values_t &param_fields_values) {
    if (contains(key_fields_values)) {
      return false;
    }

    key_setup(key_fields_values);
    data_setup(param_fields_values);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    assert(bf_status == BF_SUCCESS);

    set_keys.insert(key_fields_values);

    return true;
  }

  static std::unique_ptr<SynapseTable>
  build(const std::string _table_name,
        const std::vector<std::string> _key_fields_names,
        const std::vector<std::string> _param_fields_names) {
    return std::unique_ptr<SynapseTable>(
        new SynapseTable(_table_name, _key_fields_names, _param_fields_names));
  }

private:
  void key_setup(const fields_values_t &key_field_values) {
    auto bf_status = table->keyReset(key.get());
    assert(bf_status == BF_SUCCESS);

    for (auto i = 0u; i < key_fields.size(); i++) {
      auto key_field_value = key_field_values.values[i];
      auto key_field = key_fields[i];
      auto bf_status = key->setValue(key_field.id, key_field_value);
      assert(bf_status == BF_SUCCESS);
    }
  }

  void data_setup(const fields_values_t &param_field_values) {
    auto bf_status = table->dataReset(populate_action_id, data.get());
    assert(bf_status == BF_SUCCESS);

    for (auto i = 0u; i < param_fields.size(); i++) {
      auto param_field_value = param_field_values.values[i];
      auto param_field = param_fields[i];
      auto bf_status = data->setValue(param_field.id, param_field_value);
      assert(bf_status == BF_SUCCESS);
    }
  }
};

struct state_t {
  std::unique_ptr<SynapseTable> dp_map;

  state_t() {
    dp_map = SynapseTable::build("map",
                                 {
                                     "hdr.ipv4.src_addr",
                                     "hdr.ipv4.dst_addr",
                                     "hdr.tcpudp.src_port",
                                     "hdr.tcpudp.dst_port",
                                 },
                                 {"value"});
  }
};

std::unique_ptr<state_t> state;

void state_init() { state = std::unique_ptr<state_t>(new state_t()); }

bool nf_process(time_ns_t now, uint8_t *pkt, uint32_t size) {
  struct pkt_hdr_t *pkt_hdr = (struct pkt_hdr_t *)pkt;
  pretty_print_pkt(pkt_hdr);

  fields_values_t key(4);
  key.values[0] = ntohl(pkt_hdr->ip_hdr.src_ip);
  key.values[1] = ntohl(pkt_hdr->ip_hdr.dst_ip);
  key.values[2] = ntohs(pkt_hdr->tcp_hdr.src_port);
  key.values[3] = ntohs(pkt_hdr->tcp_hdr.dst_port);

  fields_values_t param(1);
  param.values[0] = 0x42;

  state->dp_map->add(key, param);

  return false;
}

int main(int argc, char **argv) {
  init_bf_switchd();
  setup_bf_session();
  state_init();
  register_pcie_pkt_ops();

  std::cerr << "\nController is ready.\n";

  // main thread sleeps
  while (1) {
    sleep(60);
  }

  return 0;
}