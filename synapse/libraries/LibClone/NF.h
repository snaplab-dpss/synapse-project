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
using NFId = std::string;

class NF {
private:
  const NFId id;
  const BDD bdd;

public:
  NF(const NFId &_id, const std::filesystem::path &_path, SymbolManager *symbol_manager) : id(_id), bdd(_path, symbol_manager) {}

  const NFId &get_id() const { return id; }
  const BDD &get_bdd() const { return bdd; }

  void debug() const { std::cerr << "NF{" << id << "}"; }
};

} // namespace LibClone
