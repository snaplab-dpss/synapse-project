#pragma once

#include "../primitives/table.h"

namespace sycon {

class IngressPortToNFDev : public Table {
private:
  const std::string action_name;

public:
  IngressPortToNFDev() : Table("Ingress", "ingress_port_to_nf_dev"), action_name("set_ingress_dev") {}

public:
  void add_entry(u16 ingress_port, u16 nf_dev) {
    buffer_t key(2);
    key.set(0, 2, ingress_port);

    buffer_t data(2);
    data.set(0, 2, nf_dev);

    Table::add_entry(key, action_name, {data});
  }
};

} // namespace sycon