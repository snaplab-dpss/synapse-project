#pragma once

#include <LibBDD/BDD.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>
#include <LibCore/Debug.h>

#include <vector>
#include <string>
#include <sstream>
#include <optional>
#include <unordered_map>

namespace LibSynapse {
namespace Tofino {

struct ParserState;
using states_t = std::unordered_map<LibBDD::node_id_t, ParserState *>;

enum class ParserStateType { EXTRACT, SELECT, TERMINATE };

struct ParserState {
  LibBDD::node_ids_t ids;
  ParserStateType type;

  ParserState(LibBDD::node_id_t _id, ParserStateType _type) : ids({_id}), type(_type) {}

  ParserState(const LibBDD::node_ids_t &_ids, ParserStateType _type) : ids(_ids), type(_type) {}

  virtual ~ParserState() {}

  virtual std::string dump(int lvl = 0) const {
    std::stringstream ss;

    ss << std::string(lvl * 2, ' ');
    ss << "[";
    bool first = true;
    for (LibBDD::node_id_t id : ids) {
      if (first) {
        first = false;
      } else {
        ss << ", ";
      }
      ss << id;
    }
    ss << "] ";

    return ss.str();
  }

  virtual ParserState *clone() const = 0;

  virtual bool equals(const ParserState *other) const {
    if (!other || other->type != type) {
      return false;
    }

    return true;
  }

  virtual void record(states_t &states) {
    for (LibBDD::node_id_t id : ids) {
      states[id] = this;
    }
  }
};

struct ParserStateTerminate : public ParserState {
  bool accept;

  ParserStateTerminate(LibBDD::node_id_t _id, bool _accept) : ParserState(_id, ParserStateType::TERMINATE), accept(_accept) {}

  std::string dump(int lvl = 0) const override {
    std::stringstream ss;
    ss << ParserState::dump(lvl);
    ss << (accept ? "ACCEPT" : "REJECT");
    ss << "\n";
    return ss.str();
  }

  ParserState *clone() const { return new ParserStateTerminate(*this); }

  bool equals(const ParserState *other) const override {
    if (!ParserState::equals(other)) {
      return false;
    }

    const ParserStateTerminate *other_terminate = dynamic_cast<const ParserStateTerminate *>(other);
    return other_terminate->accept == accept;
  }
};

struct ParserStateSelect : public ParserState {
  klee::ref<klee::Expr> field;
  std::vector<int> values;
  ParserState *on_true;
  ParserState *on_false;
  bool negate;

  ParserStateSelect(LibBDD::node_id_t _id, klee::ref<klee::Expr> _field, const std::vector<int> &_values, bool _negate)
      : ParserState(_id, ParserStateType::SELECT), field(_field), values(_values), on_true(nullptr), on_false(nullptr), negate(_negate) {}

  std::string dump(int lvl = 0) const override {
    std::stringstream ss;

    ss << ParserState::dump(lvl);
    ss << "select (";
    ss << "field=";
    ss << LibCore::expr_to_string(field, true);
    ss << ", values=[";
    for (size_t i = 0; i < values.size(); i++) {
      ss << values[i];
      if (i < values.size() - 1)
        ss << ", ";
    }
    ss << "]";
    ss << ", negate=" << negate;
    ss << ")\n";

    lvl++;

    if (on_true) {
      ss << std::string(lvl * 2, ' ');
      ss << "true:\n";
      ss << on_true->dump(lvl + 1);
    }

    if (on_false) {
      ss << std::string(lvl * 2, ' ');
      ss << "false:\n";
      ss << on_false->dump(lvl + 1);
    }

    return ss.str();
  }

  ParserState *clone() const {
    ParserStateSelect *clone = new ParserStateSelect(*this);
    clone->on_true           = on_true ? on_true->clone() : nullptr;
    clone->on_false          = on_false ? on_false->clone() : nullptr;
    return clone;
  }

  bool equals(const ParserState *other) const override {
    if (!ParserState::equals(other)) {
      return false;
    }

    const ParserStateSelect *other_select = dynamic_cast<const ParserStateSelect *>(other);

    if (!LibCore::solver_toolbox.are_exprs_always_equal(field, other_select->field)) {
      return false;
    }

    if (values.size() != other_select->values.size()) {
      return false;
    }

    for (size_t i = 0; i < values.size(); i++) {
      if (values[i] != other_select->values[i]) {
        return false;
      }
    }

    return true;
  }

  void record(states_t &states) override {
    ParserState::record(states);
    if (on_true)
      on_true->record(states);
    if (on_false)
      on_false->record(states);
  }
};

struct ParserStateExtract : public ParserState {
  klee::ref<klee::Expr> hdr;
  ParserState *next;

  ParserStateExtract(LibBDD::node_id_t _id, klee::ref<klee::Expr> _hdr)
      : ParserState(_id, ParserStateType::EXTRACT), hdr(_hdr), next(nullptr) {}

  std::string dump(int lvl = 0) const override {
    std::stringstream ss;

    ss << ParserState::dump(lvl);
    ss << "extract(" << LibCore::expr_to_string(hdr, true) << ")\n";

    lvl++;

    if (next) {
      ss << next->dump(lvl + 1);
    }

    return ss.str();
  }

  ParserState *clone() const {
    ParserStateExtract *clone = new ParserStateExtract(*this);
    clone->next               = next ? next->clone() : nullptr;
    return clone;
  }

  bool equals(const ParserState *other) const override {
    if (!ParserState::equals(other)) {
      return false;
    }

    const ParserStateExtract *other_extract = dynamic_cast<const ParserStateExtract *>(other);

    return LibCore::solver_toolbox.are_exprs_always_equal(hdr, other_extract->hdr);
  }

  void record(states_t &states) override {
    ParserState::record(states);
    if (next)
      next->record(states);
  }
};

class Parser {
private:
  ParserState *initial_state;
  states_t states;

public:
  Parser() : initial_state(nullptr) {}

  Parser(const Parser &other) : initial_state(nullptr) {
    if (other.initial_state) {
      initial_state = other.initial_state->clone();
      initial_state->record(states);
    }
  }

  ~Parser() {
    LibBDD::node_ids_t freed;

    // The states data structure can have duplicates, so we need to make sure
    for (const auto &[node_id, state] : states) {
      if (freed.find(node_id) == freed.end()) {
        freed.insert(state->ids.begin(), state->ids.end());
        delete state;
      }
    }
  }

  const ParserState *get_initial_state() const { return initial_state; }

  void add_extract(LibBDD::node_id_t leaf_id, LibBDD::node_id_t id, klee::ref<klee::Expr> hdr, std::optional<bool> direction) {
    ParserState *new_state = new ParserStateExtract(id, hdr);
    add_state(leaf_id, new_state, direction);
  }

  void add_extract(LibBDD::node_id_t id, klee::ref<klee::Expr> hdr) {
    ParserState *new_state = new ParserStateExtract(id, hdr);
    add_state(new_state);
  }

  void add_select(LibBDD::node_id_t leaf_id, LibBDD::node_id_t id, klee::ref<klee::Expr> field, const std::vector<int> &values,
                  std::optional<bool> direction, bool negate) {
    ParserStateSelect *new_state = new ParserStateSelect(id, field, values, negate);
    add_state(leaf_id, new_state, direction);
  }

  void add_select(LibBDD::node_id_t id, klee::ref<klee::Expr> field, const std::vector<int> &values, bool negate) {
    ParserState *new_state = new ParserStateSelect(id, field, values, negate);
    add_state(new_state);
  }

  void accept(LibBDD::node_id_t id) {
    if (already_terminated(id, true)) {
      return;
    }

    ParserState *new_state = new ParserStateTerminate(id, true);
    add_state(new_state);
  }

  void reject(LibBDD::node_id_t id) {
    if (already_terminated(id, false)) {
      return;
    }

    ParserState *new_state = new ParserStateTerminate(id, false);
    add_state(new_state);
  }

  void accept(LibBDD::node_id_t leaf_id, LibBDD::node_id_t id, std::optional<bool> direction) {
    if (already_terminated(leaf_id, id, direction, true)) {
      return;
    }

    ParserState *new_state = new ParserStateTerminate(id, true);
    add_state(leaf_id, new_state, direction);
  }

  void reject(LibBDD::node_id_t leaf_id, LibBDD::node_id_t id, std::optional<bool> direction) {
    if (already_terminated(leaf_id, id, direction, false)) {
      return;
    }

    ParserState *new_state = new ParserStateTerminate(id, false);
    add_state(leaf_id, new_state, direction);
  }

  void debug() const {
    std::cerr << "******  Parser ******\n";
    if (initial_state)
      std::cerr << initial_state->dump();
    std::cerr << "************************\n";
  }

private:
  bool already_terminated(LibBDD::node_id_t id, bool accepted) {
    if (!initial_state) {
      return false;
    }

    assert(initial_state->type == ParserStateType::TERMINATE && "Invalid parser");
    assert(dynamic_cast<ParserStateTerminate *>(initial_state)->accept == accepted && "Invalid parser");

    return true;
  }

  bool already_terminated(LibBDD::node_id_t leaf_id, LibBDD::node_id_t id, std::optional<bool> direction, bool accepted) {
    assert(initial_state && "Invalid parser");
    assert(states.find(leaf_id) != states.end() && "Invalid parser");

    ParserState *leaf = states[leaf_id];

    switch (leaf->type) {
    case ParserStateType::EXTRACT: {
      assert(!direction.has_value() && "Invalid parser");
      ParserStateExtract *extractor = dynamic_cast<ParserStateExtract *>(leaf);

      if (!extractor->next || extractor->next->type != ParserStateType::TERMINATE) {
        return false;
      }

      assert(dynamic_cast<ParserStateTerminate *>(extractor->next)->accept == accepted && "Invalid parser");
    } break;
    case ParserStateType::SELECT: {
      assert(direction.has_value() && "Invalid parser");
      ParserStateSelect *condition = dynamic_cast<ParserStateSelect *>(leaf);

      if ((*direction && !condition->on_true) || (!*direction && !condition->on_false)) {
        return false;
      }

      ParserState *next = *direction ? condition->on_true : condition->on_false;

      if (!next || next->type != ParserStateType::TERMINATE) {
        return false;
      }
    } break;
    case ParserStateType::TERMINATE: {
      assert(dynamic_cast<ParserStateTerminate *>(leaf)->accept == accepted && "Invalid parser");
    } break;
    }

    return true;
  }

  void add_state(ParserState *new_state) {
    assert(!initial_state && "Invalid parser");
    assert(states.empty() && "Invalid parser");
    assert(!new_state->ids.empty() && "Invalid parser");

    initial_state                   = new_state;
    states[*new_state->ids.begin()] = new_state;
  }

  void set_next(ParserState *&next_state, ParserState *new_state) {
    if (next_state && next_state->equals(new_state)) {
      assert(new_state->ids.size() == 1 && "Invalid parser");

      LibBDD::node_id_t new_id = *new_state->ids.begin();
      assert(next_state->ids.find(new_id) == next_state->ids.end() && "Invalid parser");

      next_state->ids.insert(new_id);

      // Fix the incorrect previous recording
      states[new_id] = next_state;
      delete new_state;

      return;
    }

    ParserState *old_next_state = next_state;
    next_state                  = new_state;

    if (!old_next_state) {
      return;
    }

    assert(old_next_state->type == ParserStateType::TERMINATE && "Invalid parser");
    assert(dynamic_cast<ParserStateTerminate *>(old_next_state)->accept == true && "Invalid parser");

    switch (new_state->type) {
    case ParserStateType::EXTRACT: {
      ParserStateExtract *extractor = dynamic_cast<ParserStateExtract *>(new_state);
      assert(!extractor->next && "Invalid parser");
      extractor->next = old_next_state;
    } break;
    case ParserStateType::SELECT: {
      ParserStateSelect *condition = dynamic_cast<ParserStateSelect *>(new_state);
      assert(!condition->on_true && "Invalid parser");
      assert(!condition->on_false && "Invalid parser");
      condition->on_true  = next_state;
      condition->on_false = next_state;
    } break;
    case ParserStateType::TERMINATE: {
      panic("Cannot add state to terminating state");
    } break;
    }
  }

  void add_state(LibBDD::node_id_t leaf_id, ParserState *new_state, std::optional<bool> direction) {
    assert(initial_state && "Invalid parser");
    assert(states.find(leaf_id) != states.end() && "Invalid parser");
    assert(!new_state->ids.empty() && "Invalid parser");
    for (LibBDD::node_id_t id : new_state->ids) {
      assert(states.find(id) == states.end() && "Invalid parser");
    }

    states[*new_state->ids.begin()] = new_state;

    ParserState *leaf = states[leaf_id];

    switch (leaf->type) {
    case ParserStateType::EXTRACT: {
      assert(!direction.has_value() && "Invalid parser");
      ParserStateExtract *extractor = dynamic_cast<ParserStateExtract *>(leaf);
      set_next(extractor->next, new_state);
    } break;
    case ParserStateType::SELECT: {
      assert(direction.has_value() && "Invalid parser");
      ParserStateSelect *condition = dynamic_cast<ParserStateSelect *>(leaf);
      if (*direction) {
        set_next(condition->on_true, new_state);
      } else {
        set_next(condition->on_false, new_state);
      }
    } break;
    case ParserStateType::TERMINATE: {
      panic("Cannot add state to terminating state");
    } break;
    }
  }
};

} // namespace Tofino
} // namespace LibSynapse