#pragma once

#include "LibCore/Symbol.h"
#include <LibBDD/BDD.h>

#include <LibCore/Debug.h>
#include <LibCore/Types.h>

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace LibClone {

using LibBDD::BDD;
using LibCore::symbol_t;
using LibCore::symbol_translation_t;
using LibCore::SymbolManager;
using NFId      = std::string;
using NFCounter = u16;

class NF {
private:
  NFCounter counter;
  const NFId id;
  BDD bdd;
  std::unordered_map<std::string, symbol_translation_t> symbol_translations;

public:
  NF(const NFCounter &_counter, const NFId &_id, const std::filesystem::path &_path, SymbolManager *symbol_manager)
      : counter(_counter), id(_id), bdd(_path, symbol_manager) {}

  const NFCounter &get_counter() const { return counter; }
  const NFId &get_id() const { return id; }
  const BDD &get_bdd() const { return bdd; }
  BDD &get_mutable_bdd() { return bdd; }

  const std::unordered_map<std::string, symbol_translation_t> &get_symbol_translations() const { return symbol_translations; }

  bool has_symbol_translation(const std::string &original_symbol) const {
    return symbol_translations.find(original_symbol) != symbol_translations.end();
  }

  const symbol_t &get_symbol_translation(const std::string &original_symbol) const { return symbol_translations.at(original_symbol).new_symbol; }

  void add_symbol_translation(const symbol_translation_t &translation) {
    assert_or_panic(symbol_translations.find(translation.old_symbol.name) == symbol_translations.end(), "Symbol already translated");
    symbol_translations.emplace(translation.old_symbol.name, translation);
  }

  void replace_symbol_translation(const symbol_translation_t &translation) {
    assert_or_panic(symbol_translations.find(translation.old_symbol.name) != symbol_translations.end(), "Symbol Translation not stored");
    symbol_translations.at(translation.old_symbol.name) = translation;
  }

  void add_or_replace_symbol_translation(const symbol_translation_t &translation) {
    if (has_symbol_translation(translation.old_symbol.name)) {
      replace_symbol_translation(translation);
    } else {
      add_symbol_translation(translation);
    }
  }

  friend std::ostream &operator<<(std::ostream &os, const NF &nf) {
    os << "NF{" << nf.id << "}";
    return os;
  }
};

} // namespace LibClone
