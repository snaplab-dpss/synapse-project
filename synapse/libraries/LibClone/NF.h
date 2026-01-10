#pragma once

#include <LibBDD/BDD.h>
#include <LibCore/Types.h>

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace LibClone {

using LibBDD::BDD;
using LibCore::SymbolManager;
using NFId      = std::string;
using NFCounter = u16;

class NF {
private:
  NFCounter counter;
  const NFId id;
  const BDD bdd;

public:
  NF(const NFCounter &_counter, const NFId &_id, const std::filesystem::path &_path, SymbolManager *symbol_manager)
      : counter(_counter), id(_id), bdd(_path, symbol_manager) {}

  const NFCounter &get_counter() const { return counter; }
  const NFId &get_id() const { return id; }
  const BDD &get_bdd() const { return bdd; }

  friend std::ostream &operator<<(std::ostream &os, const NF &nf) {
    os << "NF{" << nf.id << "}";
    return os;
  }
};

} // namespace LibClone
