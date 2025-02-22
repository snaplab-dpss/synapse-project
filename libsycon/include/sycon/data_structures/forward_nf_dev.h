#pragma once

#include "../primitives/table.h"

namespace sycon {

class ForwardNFDev : public Table {
private:
  const std::string action_name;

public:
  ForwardNFDev() : Table("Ingress", "forward_nf_dev"), action_name("fwd") {}

  void add_entry(u16 nf_dev, u16 dev_port) {
    buffer_t key(2);
    key.set(0, 2, nf_dev);

    buffer_t data(2);
    data.set(0, 2, dev_port);

    Table::add_entry(key, action_name, {data});
  }
};

} // namespace sycon