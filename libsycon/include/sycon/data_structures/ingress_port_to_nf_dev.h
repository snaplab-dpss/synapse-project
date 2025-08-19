#pragma once

#include "../primitives/table.h"

namespace sycon {

class IngressPortToNFDev : public Table {
public:
  IngressPortToNFDev() : Table("Ingress.ingress_port_to_nf_dev") {}

public:
  void add_entry(u16 ingress_port, u16 nf_dev) {
    buffer_t key(2);
    key.set(0, 2, ingress_port);

    buffer_t data(4);
    data.set(0, 4, nf_dev);

    const table_action_t set_ingress_dev = get_action("Ingress.set_ingress_dev");
    Table::add_entry(key, set_ingress_dev.name, {data});
  }

  void add_recirc_entry(u16 ingress_port) {
    buffer_t key(2);
    key.set(0, 2, ingress_port);

    const table_action_t set_ingress_dev_from_recirculation = get_action("Ingress.set_ingress_dev_from_recirculation");
    Table::add_entry(key, set_ingress_dev_from_recirculation.name, {});
  }
};

} // namespace sycon