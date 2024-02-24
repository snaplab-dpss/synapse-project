#include <assert.h>
#include <sycon/sycon.h>
#include <unistd.h>

#include <functional>
#include <unordered_set>

using namespace sycon;

bf_status_t table_with_timeout_expiration_callback(
    const bf_rt_target_t &target, const bfrt::BfRtTableKey *key, void *cookie);

struct flow_t {
  u32 src_addr;
  u32 dst_addr;
  u16 src_port;
  u16 dst_port;

  bool operator==(const flow_t &other) const {
    return (src_addr == other.src_addr) && (dst_addr == other.dst_addr) &&
           (src_port == other.src_port) && (dst_port == other.dst_port);
  }
};

struct flow_hash_t {
public:
  size_t operator()(const flow_t &flow) const {
    std::size_t h0 = std::hash<u32>{}(flow.src_addr);
    std::size_t h1 = std::hash<u32>{}(flow.dst_addr);
    std::size_t h2 = std::hash<u16>{}(flow.src_port);
    std::size_t h3 = std::hash<u16>{}(flow.dst_port);
    return h0 ^ h1 ^ h2 ^ h3;
  }
};

class TableWithTimeout : public Table {
private:
  struct key_fields_t {
    // Key fields IDs
    bf_rt_id_t src_addr;
    bf_rt_id_t dst_addr;
    bf_rt_id_t src_port;
    bf_rt_id_t dst_port;
  } key_fields;

  struct {
    // Data field ids
    bf_rt_id_t port;
    bf_rt_id_t entry_ttl;
  } data_fields;

  struct actions_t {
    // Actions ids
    bf_rt_id_t fwd;
    bf_rt_id_t send_to_controller;
  } actions;

  std::unordered_set<flow_t, flow_hash_t> flows;

public:
  TableWithTimeout() : Table("Ingress", "table_with_timeout") {
    set_notify_mode(args.expiration_time, (void *)this,
                    table_with_timeout_expiration_callback, true);

    init_key({
        {"hdr.ipv4.src_addr", &key_fields.src_addr},
        {"hdr.ipv4.dst_addr", &key_fields.dst_addr},
        {"hdr.tcpudp.src_port", &key_fields.src_port},
        {"hdr.tcpudp.dst_port", &key_fields.dst_port},
    });

    init_actions({
        {"fwd", &actions.fwd},
    });

    init_actions({
        {"send_to_controller", &actions.send_to_controller},
    });

    init_data_with_actions({
        {"port", {actions.fwd, &data_fields.port}},
        {"$ENTRY_TTL", {actions.fwd, &data_fields.entry_ttl}},
    });
  }

public:
  void add_fwd_entry(const flow_t &flow, u16 out_port) {
    if (flows.find(flow) != flows.end()) {
      DEBUG("Flow already added. Skipping.")
      return;
    }

    DEBUG("Added new entry!");

    key_setup(flow);
    data_setup_fwd(out_port);

    auto bf_status = table->tableEntryAdd(*session, dev_tgt, *key, *data);
    ASSERT_BF_STATUS(bf_status);

    flows.emplace(flow);
  }

  void del(const bfrt::BfRtTableKey *target_key) {
    flow_t flow = key_value_to_flow(target_key);

    if (flows.find(flow) == flows.end()) {
      DEBUG("Flow already removed. Skipping.")
      return;
    }

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    DEBUG("Removed entry");

    auto bf_status =
        table->tableEntryDel(*session, dev_tgt, flags, *target_key);
    ASSERT_BF_STATUS(bf_status)

    flows.erase(flow);
  }

private:
  void key_setup(const flow_t &flow) {
    table->keyReset(key.get());

    auto bf_status =
        key->setValue(key_fields.src_addr, static_cast<u64>(flow.src_addr));
    ASSERT_BF_STATUS(bf_status);

    bf_status =
        key->setValue(key_fields.dst_addr, static_cast<u64>(flow.dst_addr));
    ASSERT_BF_STATUS(bf_status);

    bf_status =
        key->setValue(key_fields.src_port, static_cast<u64>(flow.src_port));
    ASSERT_BF_STATUS(bf_status);

    bf_status =
        key->setValue(key_fields.dst_port, static_cast<u64>(flow.dst_port));
    ASSERT_BF_STATUS(bf_status);
  }

  void data_setup_fwd(u16 port) {
    auto bf_status = table->dataReset(actions.fwd, data.get());
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.port, static_cast<u64>(port));
    ASSERT_BF_STATUS(bf_status);

    bf_status = data->setValue(data_fields.entry_ttl,
                               static_cast<u64>(args.expiration_time));
    ASSERT_BF_STATUS(bf_status);
  }

  flow_t key_value_to_flow(const bfrt::BfRtTableKey *target_key) {
    flow_t flow;

    auto bf_status =
        target_key->getValue(key_fields.src_addr, (u64 *)&flow.src_addr);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->getValue(key_fields.dst_addr, (u64 *)&flow.dst_addr);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->getValue(key_fields.src_port, (u64 *)&flow.src_port);
    ASSERT_BF_STATUS(bf_status);

    bf_status = key->getValue(key_fields.dst_port, (u64 *)&flow.dst_port);
    ASSERT_BF_STATUS(bf_status);

    return flow;
  }
};

bf_status_t table_with_timeout_expiration_callback(
    const bf_rt_target_t &target, const bfrt::BfRtTableKey *key, void *cookie) {
  TableWithTimeout *table_with_timeout =
      static_cast<TableWithTimeout *>(cookie);

  DEBUG("EXPIRED!")

  begin_transaction();
  table_with_timeout->del(key);
  end_transaction();

  return BF_SUCCESS;
}

struct state_t {
  TableWithTimeout table_with_timeout;
};

state_t *state;

void sycon::nf_init() { state = new state_t(); }

void sycon::nf_cleanup() {
  state->table_with_timeout.dump();
  delete state;
}

bool sycon::nf_process(time_ns_t now, byte_t *pkt, u16 size) {
  cpu_hdr_t *cpu_hdr = (cpu_hdr_t *)packet_consume(pkt, sizeof(cpu_hdr_t));
  eth_hdr_t *eth_hdr = (eth_hdr_t *)packet_consume(pkt, sizeof(eth_hdr_t));
  ipv4_hdr_t *ipv4_hdr = (ipv4_hdr_t *)packet_consume(pkt, sizeof(ipv4_hdr_t));
  tcpudp_hdr_t *tcpudp_hdr =
      (tcpudp_hdr_t *)packet_consume(pkt, sizeof(tcpudp_hdr_t));

  flow_t flow = {SWAP_ENDIAN_32(ipv4_hdr->src_ip),
                 SWAP_ENDIAN_32(ipv4_hdr->dst_ip),
                 SWAP_ENDIAN_16(tcpudp_hdr->src_port),
                 SWAP_ENDIAN_16(tcpudp_hdr->dst_port)};

  state->table_with_timeout.add_fwd_entry(flow, nf_config.out_dev_port);

  cpu_hdr->out_port = SWAP_ENDIAN_16(nf_config.out_dev_port);

  return true;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
