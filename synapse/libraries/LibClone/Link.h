#pragma once

#include <LibCore/Types.h>
#include <LibCore/Debug.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace LibClone {

using LinkId = size_t;

class Link {
private:
  static LinkId id_counter;

  const LinkId id;
  const std::string node1;
  const std::string node2;
  const u32 port1;
  const u32 port2;

public:
  Link(const std::string &_node1, const u32 _port1, const std::string &_node2, const u32 _port2)
      : id(id_counter++), node1(_node1), node2(_node2), port1(_port1), port2(_port2) {
    if (node1 == node2 && port1 == port2) {
      panic("Link cannot connect the same node and port to itself: %s:%u", node1.c_str(), port1);
    }
  }

  ~Link() = default;

  LinkId get_id() const { return id; }
  std::string get_node1() const { return node1; }
  std::string get_node2() const { return node2; }
  u32 get_port1() const { return port1; }
  u32 get_port2() const { return port2; }

  void debug() const { std::cerr << "Link{" << id << " " << node1 << ":" << port1 << " <--> " << node2 << ":" << port2 << "}"; }
};

} // namespace LibClone
