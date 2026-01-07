#pragma once

#include "../primitives/table.h"

#include <unordered_map>

namespace sycon {

class IngressPortToNFDev : public Table {
public:
  std::unordered_map<u16, u16> port_to_nf_dev;
  std::unordered_map<u16, u16> nf_dev_to_port;

  IngressPortToNFDev() : Table("Ingress.ingress_port_to_nf_dev") {}

public:
  void add_entry(u16 ingress_port, u16 nf_dev) {
    buffer_t key(2);
    key.set(0, 2, ingress_port);

    buffer_t data(4);
    data.set(0, 4, nf_dev);

    const table_action_t set_ingress_dev = get_action("Ingress.set_ingress_dev");
    Table::add_entry(key, set_ingress_dev.name, {data});

    port_to_nf_dev[ingress_port] = nf_dev;
    nf_dev_to_port[nf_dev]       = ingress_port;
  }

  void add_recirc_entry(u16 ingress_port) {
    buffer_t key(2);
    key.set(0, 2, ingress_port);

    const table_action_t set_ingress_dev_from_recirculation = get_action("Ingress.set_ingress_dev_from_recirculation");
    Table::add_entry(key, set_ingress_dev_from_recirculation.name, {});
  }

  u16 get_nf_dev(u16 ingress_port) const {
    auto it = port_to_nf_dev.find(ingress_port);
    if (it == port_to_nf_dev.end()) {
      ERROR("No NF device mapped for ingress port %u", ingress_port);
    }
    return it->second;
  }

  u16 get_ingress_port(u16 nf_dev) const {
    auto it = nf_dev_to_port.find(nf_dev);
    if (it == nf_dev_to_port.end()) {
      ERROR("No ingress port mapped for NF device %u", nf_dev);
    }
    return it->second;
  }
};

} // namespace sycon