#pragma once

#include "../primitives/table.h"

namespace sycon {

class IngressPortToNFDev : public Table {
public:
  IngressPortToNFDev() : Table("Ingress", "ingress_port_to_nf_dev") {}

public:
  void add_entry(u16 ingress_port, u16 nf_dev) {
    buffer_t key(2);
    key.set(0, 2, ingress_port);

    buffer_t data(4);
    data.set(0, 4, nf_dev);

    std::optional<table_action_t> set_ingress_dev;
    for (const table_action_t &action : actions) {
      if (action.name.find("set_ingress_dev") != std::string::npos) {
        set_ingress_dev = action;
        break;
      }
    }
    assert(set_ingress_dev.has_value());

    Table::add_entry(key, set_ingress_dev->name, {data});
  }
};

} // namespace sycon