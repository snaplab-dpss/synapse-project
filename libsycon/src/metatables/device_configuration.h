#pragma once

#include "../../include/sycon/args.h"
#include "../../include/sycon/log.h"
#include "../../include/sycon/primitives/meta_table.h"
#include "../../include/sycon/time.h"
#include "../../include/sycon/util.h"

namespace sycon {

class Device_Configuration : MetaTable {
private:
  // Data field ids
  bf_rt_id_t sku;
  bf_rt_id_t num_pipes;
  bf_rt_id_t num_stages;
  bf_rt_id_t num_max_ports;
  bf_rt_id_t num_front_ports;
  bf_rt_id_t pcie_cpu_port;
  bf_rt_id_t eth_cpu_port_list;
  bf_rt_id_t internal_port_list;
  bf_rt_id_t external_port_list;
  bf_rt_id_t recirc_port_list;
  bf_rt_id_t intr_based_link_monitoring;
  bf_rt_id_t flow_learn_intr_mode;
  bf_rt_id_t lrt_dr_timeout_usec;
  bf_rt_id_t flow_learn_timeout_usec;
  bf_rt_id_t inactive_node_delete;
  bf_rt_id_t selector_member_order;

public:
  Device_Configuration() : MetaTable(TOFINO_ARCH(args.tna_version) + ".dev.device_configuration") {
    init_data({{"sku", &sku},
               {"num_pipes", &num_pipes},
               {"num_stages", &num_stages},
               {"num_max_ports", &num_max_ports},
               {"num_front_ports", &num_front_ports},
               {"pcie_cpu_port", &pcie_cpu_port},
               {"eth_cpu_port_list", &eth_cpu_port_list},
               {"internal_port_list", &internal_port_list},
               {"external_port_list", &external_port_list},
               {"recirc_port_list", &recirc_port_list},
               {"intr_based_link_monitoring", &intr_based_link_monitoring},
               {"flow_learn_intr_mode", &flow_learn_intr_mode},
               {"lrt_dr_timeout_usec", &lrt_dr_timeout_usec},
               {"flow_learn_timeout_usec", &flow_learn_timeout_usec},
               {"inactive_node_delete", &inactive_node_delete},
               {"selector_member_order", &selector_member_order}});
  }

  void set_digest_timout(time_us_t timeout) {
    bf_status_t bf_status = table->dataReset(data.get());
    ASSERT_BF_STATUS(bf_status);

    data->setValue(flow_learn_timeout_usec, timeout);
    ASSERT_BF_STATUS(bf_status);

    u64 flags = 0;
    BF_RT_FLAG_CLEAR(flags, BF_RT_FROM_HW);

    bf_status = table->tableDefaultEntrySet(*session, dev_tgt, *data);
    ASSERT_BF_STATUS(bf_status);
  }
};

} // namespace sycon