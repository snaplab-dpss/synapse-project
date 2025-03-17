#pragma once

#include "../primitives/table.h"

namespace sycon {

class ForwardNFDev : public Table {
public:
  ForwardNFDev() : Table("Ingress.forward_nf_dev") {}

  void add_entry(u32 nf_dev, u16 dev_port) {
    buffer_t key(4);
    key.set(0, 4, nf_dev);

    buffer_t data(2);
    data.set(0, 2, dev_port);

    assert(actions.size() == 1);
    Table::add_entry(key, actions[0].name, {data});
  }
};

} // namespace sycon