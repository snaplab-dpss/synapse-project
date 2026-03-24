#pragma once

#include <LibCore/Types.h>

#include <LibBDD/BDD.h>

#include <LibSynapse/Target.h>

#include <unordered_map>

namespace LibClone {

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::BDDNode;
using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::Route;

using LibSynapse::TargetType;

struct EmbeddingCost {
  u32 processing;
  u32 memory;

  EmbeddingCost(u32 p = 0, u32 m = 0) : processing(p), memory(m) {}

  bool operator==(const EmbeddingCost &other) const { return processing == other.processing && memory == other.memory; }

  std::string to_string() const { return "{ processing: " + std::to_string(processing) + ", memory: " + std::to_string(memory) + " }"; }
};

using EmbeddingCosts = std::unordered_map<bdd_node_id_t, EmbeddingCost>;

enum class DataStructureType {
  //========================
  //          x86
  //========================

  x86_Map,
  x86_Vector,
  x86_DChain,
  x86_CMS,
  x86_CHT,
  x86_TokenBucket,
  x86_LPM,
  x86_BloomFilter,
};

struct DataStructure {
  DataStructureType type;
  virtual ~DataStructure() = default;

  virtual DataStructureType get_type() const { return type; };

  virtual std::string to_string() const     = 0;
  virtual EmbeddingCost alloc_cost() const  = 0;
  virtual EmbeddingCost access_cost() const = 0;
};

using DataStructures = std::unordered_map<addr_t, std::unique_ptr<const DataStructure>>;

class EmbeddingProfiler {
public:
  EmbeddingProfiler()          = default;
  virtual ~EmbeddingProfiler() = default;

  virtual const EmbeddingCost visitMapAllocate(const Call *node)                     = 0;
  virtual const EmbeddingCost visitMapGet(const Call *node)                          = 0;
  virtual const EmbeddingCost visitMapPut(const Call *node)                          = 0;
  virtual const EmbeddingCost visitMapErase(const Call *node)                        = 0;
  virtual const EmbeddingCost visitExpireItemsSingleMap(const Call *node)            = 0;
  virtual const EmbeddingCost visitExpireItemsSingleMapIteratively(const Call *node) = 0;

  virtual const EmbeddingCost visitVectorAllocate(const Call *node) = 0;
  virtual const EmbeddingCost visitVectorBorrow(const Call *node)   = 0;
  virtual const EmbeddingCost visitVectorReturn(const Call *node)   = 0;
  virtual const EmbeddingCost visitVectorClear(const Call *node)    = 0;
  virtual const EmbeddingCost visitVectorSampleLt(const Call *node) = 0;

  virtual const EmbeddingCost visitDchainAllocate(const Call *node)         = 0;
  virtual const EmbeddingCost visitDchainAllocateNewIndex(const Call *node) = 0;
  virtual const EmbeddingCost visitDchainRejuvenateIndex(const Call *node)  = 0;
  virtual const EmbeddingCost visitDchainExpireOneIndex(const Call *node)   = 0;
  virtual const EmbeddingCost visitDchainIsIndexAllocated(const Call *node) = 0;
  virtual const EmbeddingCost visitDchainFreeIndex(const Call *node)        = 0;

  virtual const EmbeddingCost visitCMSAllocate(const Call *node)        = 0;
  virtual const EmbeddingCost visitCMSIncrement(const Call *node)       = 0;
  virtual const EmbeddingCost visitCMSCountMin(const Call *node)        = 0;
  virtual const EmbeddingCost visitCMSPeriodicCleanup(const Call *node) = 0;

  virtual const EmbeddingCost visitLPMAllocate(const Call *node) = 0;
  virtual const EmbeddingCost visitLPMFree(const Call *node)     = 0;
  virtual const EmbeddingCost visitLPMFromFile(const Call *node) = 0;
  virtual const EmbeddingCost visitLPMUpdate(const Call *node)   = 0;
  virtual const EmbeddingCost visitLPMLookup(const Call *node)   = 0;

  virtual const EmbeddingCost visitTokenBucketAllocate(const Call *node)       = 0;
  virtual const EmbeddingCost visitTokenBucketIsTracing(const Call *node)      = 0;
  virtual const EmbeddingCost visitTokenBucketTrace(const Call *node)          = 0;
  virtual const EmbeddingCost visitTokenBucketUpdateAndCheck(const Call *node) = 0;
  virtual const EmbeddingCost visitTokenBucketExpire(const Call *node)         = 0;

  const EmbeddingCost visitGenericCall(const Call *node);

  const EmbeddingCost visitBranch(const Branch *node) const { return EmbeddingCost(1, 0); }
  const EmbeddingCost visitRoute(const Route *node) const { return EmbeddingCost(1, 0); }

  const EmbeddingCosts compute_all_costs(const BDD &bdd);

  void debug_embedding_costs(const EmbeddingCosts &costs) const;

  virtual TargetType get_target_type() const = 0;

protected:
  // void add_cost(bdd_node_id_t node_id, const EmbeddingCost &cost);
  void register_data_structure(addr_t address, std::unique_ptr<const DataStructure> ds);

  static u32 get_u32(const Call *node, const std::string &param);
  addr_t get_allocation_address(const Call *node, const std::string &arg_name) const;
  addr_t get_address(const Call *node, const std::string &arg_name) const;

  bool has_data_structure(addr_t address) const;
  const DataStructure *lookup_data_structure(addr_t address) const;

private:
  DataStructures data_structures;
};

struct EmbeddingProfilers {
  std::vector<std::unique_ptr<EmbeddingProfiler>> elements;

  EmbeddingProfilers();

  std::vector<EmbeddingProfiler *> get_profilers() const;
};

} // namespace LibClone
