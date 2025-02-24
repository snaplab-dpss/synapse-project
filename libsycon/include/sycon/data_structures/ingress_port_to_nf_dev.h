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

    buffer_t data(2);
    data.set(0, 2, nf_dev);

    assert(actions.size() == 1);
    Table::add_entry(key, actions[0].name, {data});
  }
};

} // namespace sycon