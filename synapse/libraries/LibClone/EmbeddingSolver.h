#pragma once

#include <LibClone/MetaNode.h>
#include <LibClone/PhysicalNetwork.h>

namespace LibClone {

// Forward declaration
class MetaNode;
enum class MetaNodeType;
enum class DeviceType;

struct EmbeddingSolution {
  std::unordered_map<MetaNodeId, DeviceId> node_to_device;
  std::unordered_map<DeviceId, std::vector<MetaNodeId>> device_to_nodes;
  std::unordered_map<DeviceId, bool> device_active;
  double objective_value;
  bool feasible;

  void debug() const;
};

class EmbeddingSolver {
public:
  EmbeddingSolver(const PhysicalNetwork &_phys_net);

  EmbeddingSolver(const EmbeddingSolver &)            = delete;
  EmbeddingSolver &operator=(const EmbeddingSolver &) = delete;

  void set_time_limit(double seconds);
  void set_verbose(bool verbose);

private:
  const PhysicalNetwork &phys_net;
  double time_limit = 3600;
  bool verbose      = true;
};

} // namespace LibClone
