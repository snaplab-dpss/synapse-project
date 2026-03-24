#include "LibClone/EmbeddingProfiler.h"
#include <LibClone/Profilers/x86EmbeddingProfiler.h>
#include <LibCore/Debug.h>
#include <memory>

namespace LibClone {

using LibClone::EmbeddingCost;

// ============================================
// x86Map Implementation
// ============================================

x86EmbeddingProfiler::x86Map::x86Map(u32 c, u32 k) : capacity(c), key_size(k) { type = DataStructureType::x86_Map; }

std::string x86EmbeddingProfiler::x86Map::to_string() const {
  return "x86Map{capacity=" + std::to_string(capacity) + ", key_size=" + std::to_string(key_size) + "}";
}

EmbeddingCost x86EmbeddingProfiler::x86Map::alloc_cost() const {
  const u64 busybits_mem = (u64)capacity * sizeof(int);
  u64 keyps_mem          = (u64)capacity * sizeof(void *);
  u64 khs_mem            = (u64)capacity * sizeof(unsigned);
  u64 chns_mem           = (u64)capacity * sizeof(int);
  u64 vals_mem           = (u64)capacity * sizeof(int);
  u64 keys_mem           = (u64)capacity * key_size;

  u64 struct_mem = sizeof(void *) * 5 + sizeof(unsigned) * 3;

  u64 total_mem = busybits_mem + keyps_mem + khs_mem + chns_mem + vals_mem + keys_mem + struct_mem;
  return EmbeddingCost(2, total_mem);
}

EmbeddingCost x86EmbeddingProfiler::x86Map::access_cost() const { return EmbeddingCost(2, 0); }

// ============================================
// x86Vector Implementation
// ============================================

x86EmbeddingProfiler::x86Vector::x86Vector(u32 e, u32 c) : elem_size(e), capacity(c) { type = DataStructureType::x86_Vector; }

std::string x86EmbeddingProfiler::x86Vector::to_string() const {
  return "x86Vector{elem_size=" + std::to_string(elem_size) + ", capacity=" + std::to_string(capacity) + "}";
}

EmbeddingCost x86EmbeddingProfiler::x86Vector::alloc_cost() const {
  u64 mem     = (u64)elem_size * capacity;
  u32 process = (u32)capacity;
  return EmbeddingCost(process, mem);
}

EmbeddingCost x86EmbeddingProfiler::x86Vector::access_cost() const { return EmbeddingCost(1, 0); }

// ============================================
// x86DChain Implementation
// ============================================

x86EmbeddingProfiler::x86DChain::x86DChain(u32 r) : index_range(r) { type = DataStructureType::x86_DChain; }

std::string x86EmbeddingProfiler::x86DChain::to_string() const { return "x86DChain{index_range=" + std::to_string(index_range) + "}"; }

EmbeddingCost x86EmbeddingProfiler::x86DChain::alloc_cost() const {
  u64 mem = (u64)(index_range + 2) * 16 + (u64)index_range * 8;
  return EmbeddingCost(1, mem);
}

EmbeddingCost x86EmbeddingProfiler::x86DChain::access_cost() const { return EmbeddingCost(1, 0); }

// ============================================
// x86CMS Implementation
// ============================================

x86EmbeddingProfiler::x86CMS::x86CMS(u32 h, u32 w) : vector(8, h * w), height(h), width(w) { type = DataStructureType::x86_CMS; }

std::string x86EmbeddingProfiler::x86CMS::to_string() const {
  return "x86CMS{vector=" + vector.to_string() + ", height=" + std::to_string(height) + ", width=" + std::to_string(width) + "}";
}

EmbeddingCost x86EmbeddingProfiler::x86CMS::alloc_cost() const {
  u64 mem = vector.alloc_cost().memory + 3 * (u64)sizeof(u32);
  return EmbeddingCost(height, mem);
}

EmbeddingCost x86EmbeddingProfiler::x86CMS::access_cost() const { return EmbeddingCost(height, 0); }

// ============================================
// x86BloomFilter Implementation
// ============================================

x86EmbeddingProfiler::x86BloomFilter::x86BloomFilter(u32 h, u32 w) : height(h), width(w) { type = DataStructureType::x86_BloomFilter; }

std::string x86EmbeddingProfiler::x86BloomFilter::to_string() const {
  return "x86BloomFilter{height=" + std::to_string(height) + ", width=" + std::to_string(width) + "}";
}

EmbeddingCost x86EmbeddingProfiler::x86BloomFilter::alloc_cost() const {
  u64 mem = (u64)height * width;
  return EmbeddingCost(height, mem);
}

EmbeddingCost x86EmbeddingProfiler::x86BloomFilter::access_cost() const { return EmbeddingCost(height, 0); }

// ============================================
// x86LPM Implementation
// ============================================

x86EmbeddingProfiler::x86LPM::x86LPM() { type = DataStructureType::x86_LPM; }

std::string x86EmbeddingProfiler::x86LPM::to_string() const { return "x86LPM{}"; }

EmbeddingCost x86EmbeddingProfiler::x86LPM::alloc_cost() const {
  u64 mem = (u64)16777216 * 2 + 65536 * 2;
  return EmbeddingCost(2, mem);
}

EmbeddingCost x86EmbeddingProfiler::x86LPM::access_cost() const { return EmbeddingCost(2, 0); }

// ============================================
// x86TokenBucket Implementation
// ============================================

x86EmbeddingProfiler::x86TokenBucket::x86TokenBucket(u32 c, u32 k) : map(c, k), keys(k, c), buckets(16, c), dchain(c) {
  type = DataStructureType::x86_TokenBucket;
}

std::string x86EmbeddingProfiler::x86TokenBucket::to_string() const {
  return "x86TokenBucket{map=" + map.to_string() + ", keys=" + keys.to_string() + ", buckets=" + buckets.to_string(),
         +", chain= " + dchain.to_string() + " }";
}

EmbeddingCost x86EmbeddingProfiler::x86TokenBucket::alloc_cost() const {
  EmbeddingCost map_cost     = map.alloc_cost();
  EmbeddingCost keys_cost    = keys.alloc_cost();
  EmbeddingCost buckets_cost = buckets.alloc_cost();
  EmbeddingCost dchain_cost  = dchain.alloc_cost();

  u64 total_mem        = map_cost.memory + keys_cost.memory + buckets_cost.memory + dchain_cost.memory;
  u32 total_processing = map_cost.processing + keys_cost.processing + buckets_cost.processing + dchain_cost.processing;

  return EmbeddingCost(total_processing, total_mem);
}

EmbeddingCost x86EmbeddingProfiler::x86TokenBucket::access_cost() const { return EmbeddingCost(4, 0); }

// ============================================
// Map Operations
// ============================================

const EmbeddingCost x86EmbeddingProfiler::visitMapAllocate(const Call *node) {
  u32 capacity = get_u32(node, "capacity");
  u32 key_size = get_u32(node, "key_size");

  addr_t addr = get_allocation_address(node, "map_out");

  std::unique_ptr<const x86Map> map = std::make_unique<const x86Map>(capacity, key_size);

  const EmbeddingCost map_alloc_cost = map->alloc_cost();

  register_data_structure(addr, std::move(map));

  return map_alloc_cost;
}

const EmbeddingCost x86EmbeddingProfiler::visitMapGet(const Call *node) {
  addr_t addr = get_address(node, "map");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown x86Map");

  const DataStructure *map = lookup_data_structure(addr);
  return map->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitMapPut(const Call *node) {
  addr_t addr = get_address(node, "map");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown x86Map");

  const DataStructure *map = lookup_data_structure(addr);
  return map->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitMapErase(const Call *node) {
  addr_t addr = get_address(node, "map");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown x86Map");

  const DataStructure *map = lookup_data_structure(addr);
  return map->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitExpireItemsSingleMap(const Call *node) {
  addr_t chain_addr  = get_address(node, "chain");
  addr_t vector_addr = get_address(node, "vector");
  addr_t map_addr    = get_address(node, "map");

  assert_or_panic(has_data_structure(chain_addr), "Accessing Unknown x86DChain");
  assert_or_panic(has_data_structure(vector_addr), "Accessing Unknown x86Vector");
  assert_or_panic(has_data_structure(map_addr), "Accessing Unknown x86Map");

  const DataStructure *chain_ds  = lookup_data_structure(chain_addr);
  const DataStructure *vector_ds = lookup_data_structure(vector_addr);
  const DataStructure *map_ds    = lookup_data_structure(map_addr);

  EmbeddingCost chain_cost  = chain_ds->access_cost();
  EmbeddingCost vector_cost = vector_ds->access_cost();
  EmbeddingCost map_cost    = map_ds->access_cost();

  assert_or_panic(chain_ds->get_type() == DataStructureType::x86_DChain, "Stored DS is not a DChain");

  const x86DChain *dchain = dynamic_cast<const x86DChain *>(chain_ds);

  u32 index_range = dchain->index_range;

  u32 total_processing = index_range * (chain_cost.processing + vector_cost.processing * 2 + map_cost.processing);

  u64 total_memory = index_range * (chain_cost.memory + vector_cost.memory * 2 + map_cost.memory);

  return EmbeddingCost(total_processing, total_memory);
}

const EmbeddingCost x86EmbeddingProfiler::visitExpireItemsSingleMapIteratively(const Call *node) {
  u32 n_elems = get_u32(node, "n_elems");
  u32 start   = get_u32(node, "start");

  addr_t vector_addr = get_address(node, "vector");
  addr_t map_addr    = get_address(node, "map");

  assert_or_panic(has_data_structure(vector_addr), "Accessing Unknown x86Vector");
  assert_or_panic(has_data_structure(map_addr), "Accessing Unknown x86Map");

  const DataStructure *vector = lookup_data_structure(vector_addr);
  const DataStructure *map    = lookup_data_structure(map_addr);

  EmbeddingCost vector_cost = vector->access_cost();
  EmbeddingCost map_cost    = map->access_cost();

  u32 total_processing = (n_elems - start) * (vector_cost.processing * 2 + map_cost.processing);

  u64 total_memory = (n_elems - start) * (vector_cost.memory * 2 + map_cost.memory);

  return EmbeddingCost(total_processing, total_memory);
}

const EmbeddingCost x86EmbeddingProfiler::visitVectorAllocate(const Call *node) {
  u32 elem_size = get_u32(node, "elem_size");
  u32 capacity  = get_u32(node, "capacity");

  addr_t addr = get_allocation_address(node, "vector_out");

  std::unique_ptr<const x86Vector> vector = std::make_unique<const x86Vector>(elem_size, capacity);

  const EmbeddingCost vector_cost = vector->alloc_cost();
  register_data_structure(addr, std::move(vector));

  return vector_cost;
}

const EmbeddingCost x86EmbeddingProfiler::visitVectorBorrow(const Call *node) {
  addr_t addr = get_address(node, "vector");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown x86Vector");

  const DataStructure *vector = lookup_data_structure(addr);
  return vector->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitVectorReturn(const Call *node) { return EmbeddingCost(0, 0); }

const EmbeddingCost x86EmbeddingProfiler::visitVectorClear(const Call *node) {
  addr_t addr = get_address(node, "vector");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown x86Vector");

  const DataStructure *vector = lookup_data_structure(addr);
  return vector->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitVectorSampleLt(const Call *node) {
  addr_t addr = get_address(node, "vector");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown x86Vector");

  const DataStructure *ds = lookup_data_structure(addr);
  assert_or_panic(ds->get_type() == DataStructureType::x86_Vector, "Stored DS is not a vector");

  const x86Vector *vector = dynamic_cast<const x86Vector *>(ds);

  u32 total_processing = vector->access_cost().processing * vector->elem_size;

  return EmbeddingCost(total_processing, 0);
}

const EmbeddingCost x86EmbeddingProfiler::visitDchainAllocate(const Call *node) {
  u32 index_range = get_u32(node, "index_range");
  addr_t addr     = get_allocation_address(node, "chain_out");

  std::unique_ptr<const x86DChain> dchain = std::make_unique<const x86DChain>(index_range);

  EmbeddingCost dchain_alloc_cost = dchain->alloc_cost();
  register_data_structure(addr, std::move(dchain));

  return dchain_alloc_cost;
}

const EmbeddingCost x86EmbeddingProfiler::visitDchainAllocateNewIndex(const Call *node) {
  addr_t addr = get_address(node, "chain");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown DChain");
  const DataStructure *ds = lookup_data_structure(addr);
  return ds->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitDchainRejuvenateIndex(const Call *node) {
  addr_t addr = get_address(node, "chain");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown DChain");
  const DataStructure *ds = lookup_data_structure(addr);
  return ds->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitDchainExpireOneIndex(const Call *node) { panic("TODO: implement visitDchainExpireOneIndex"); }

const EmbeddingCost x86EmbeddingProfiler::visitDchainIsIndexAllocated(const Call *node) {
  addr_t addr = get_address(node, "chain");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown DChain");
  const DataStructure *ds = lookup_data_structure(addr);
  return ds->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitDchainFreeIndex(const Call *node) {
  addr_t addr = get_address(node, "chain");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown DChain");
  const DataStructure *ds = lookup_data_structure(addr);
  return ds->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitCMSAllocate(const Call *node) {
  u32 height  = get_u32(node, "height");
  u32 width   = get_u32(node, "width");
  addr_t addr = get_allocation_address(node, "cms_out");

  std::unique_ptr<const x86CMS> cms = std::make_unique<const x86CMS>(height, width);

  EmbeddingCost cms_alloc_cost = cms->alloc_cost();
  register_data_structure(addr, std::move(cms));

  return cms_alloc_cost;
}

const EmbeddingCost x86EmbeddingProfiler::visitCMSIncrement(const Call *node) {
  addr_t addr = get_address(node, "cms");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown CMS");
  const DataStructure *ds = lookup_data_structure(addr);

  assert_or_panic(ds->get_type() == DataStructureType::x86_CMS, "Stored DS is not a cms");
  const x86CMS *cms = dynamic_cast<const x86CMS *>(ds);

  u32 total_processing = cms->height * cms->vector.access_cost().processing * 2;

  return EmbeddingCost(total_processing, 0);
}

const EmbeddingCost x86EmbeddingProfiler::visitCMSCountMin(const Call *node) {
  addr_t addr = get_address(node, "cms");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown CMS");
  const DataStructure *ds = lookup_data_structure(addr);

  assert_or_panic(ds->get_type() == DataStructureType::x86_CMS, "Stored DS is not a cms");
  const x86CMS *cms = dynamic_cast<const x86CMS *>(ds);

  u32 total_processing = cms->height * cms->vector.access_cost().processing * 2;

  return EmbeddingCost(total_processing, 0);
}

const EmbeddingCost x86EmbeddingProfiler::visitCMSPeriodicCleanup(const Call *node) { return EmbeddingCost(1, 0); }

const EmbeddingCost x86EmbeddingProfiler::visitLPMAllocate(const Call *node) {
  addr_t addr                       = get_allocation_address(node, "lpm_out");
  std::unique_ptr<const x86LPM> lpm = std::make_unique<const x86LPM>();

  EmbeddingCost lpm_alloc_cost = lpm->alloc_cost();
  register_data_structure(addr, std::move(lpm));

  return lpm_alloc_cost;
}

const EmbeddingCost x86EmbeddingProfiler::visitLPMFree(const Call *node) { panic("TODO: implement lpm_free"); }

const EmbeddingCost x86EmbeddingProfiler::visitLPMFromFile(const Call *node) { return EmbeddingCost(100, 0); }

const EmbeddingCost x86EmbeddingProfiler::visitLPMUpdate(const Call *node) {
  addr_t addr = get_address(node, "lpm");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown LPM");
  const DataStructure *ds = lookup_data_structure(addr);

  return ds->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitLPMLookup(const Call *node) {
  addr_t addr = get_address(node, "lpm");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown LPM");
  const DataStructure *ds = lookup_data_structure(addr);

  return ds->access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitTokenBucketAllocate(const Call *node) {
  u32 capacity = get_u32(node, "capacity");
  u32 key_size = get_u32(node, "key_size");
  addr_t addr  = get_allocation_address(node, "tb_out");

  std::unique_ptr<const x86TokenBucket> tb = std::make_unique<const x86TokenBucket>(capacity, key_size);

  EmbeddingCost tb_alloc_cost = tb->alloc_cost();
  register_data_structure(addr, std::move(tb));

  return tb_alloc_cost;
}

const EmbeddingCost x86EmbeddingProfiler::visitTokenBucketIsTracing(const Call *node) {
  addr_t addr = get_address(node, "tb");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown TB");

  const DataStructure *ds = lookup_data_structure(addr);
  assert_or_panic(ds->get_type() == DataStructureType::x86_TokenBucket, "Store DS is not a TB");

  const x86TokenBucket *tb = dynamic_cast<const x86TokenBucket *>(ds);

  return tb->map.access_cost();
}

const EmbeddingCost x86EmbeddingProfiler::visitTokenBucketTrace(const Call *node) {
  addr_t addr = get_address(node, "tb");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown TB");

  const DataStructure *ds = lookup_data_structure(addr);
  assert_or_panic(ds->get_type() == DataStructureType::x86_TokenBucket, "Store DS is not a TB");

  const x86TokenBucket *tb = dynamic_cast<const x86TokenBucket *>(ds);

  u64 total_memory = 4 * tb->keys.access_cost().memory + tb->map.access_cost().memory + tb->dchain.access_cost().memory;

  u32 total_processing = 4 * tb->keys.access_cost().processing + tb->map.access_cost().processing + tb->dchain.access_cost().processing;

  return EmbeddingCost(total_processing, total_memory);
}

const EmbeddingCost x86EmbeddingProfiler::visitTokenBucketUpdateAndCheck(const Call *node) {
  addr_t addr = get_address(node, "tb");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown TB");

  const DataStructure *ds = lookup_data_structure(addr);
  assert_or_panic(ds->get_type() == DataStructureType::x86_TokenBucket, "Store DS is not a TB");

  const x86TokenBucket *tb = dynamic_cast<const x86TokenBucket *>(ds);

  u64 total_memory = 2 * tb->keys.access_cost().memory + tb->dchain.access_cost().memory;

  u32 total_processing = 2 * tb->keys.access_cost().processing + tb->dchain.access_cost().processing;

  return EmbeddingCost(total_processing, total_memory);
}

const EmbeddingCost x86EmbeddingProfiler::visitTokenBucketExpire(const Call *node) {
  addr_t addr = get_address(node, "tb");
  assert_or_panic(has_data_structure(addr), "Accessing Unknown TB");

  const DataStructure *ds = lookup_data_structure(addr);
  assert_or_panic(ds->get_type() == DataStructureType::x86_TokenBucket, "Store DS is not a TB");

  const x86TokenBucket *tb = dynamic_cast<const x86TokenBucket *>(ds);

  EmbeddingCost chain_cost  = tb->dchain.access_cost();
  EmbeddingCost vector_cost = tb->keys.access_cost();
  EmbeddingCost map_cost    = tb->map.access_cost();

  u32 index_range = tb->dchain.index_range;

  u32 total_processing = index_range * (chain_cost.processing + vector_cost.processing * 2 + map_cost.processing);

  u64 total_memory = index_range * (chain_cost.memory + vector_cost.memory * 2 + map_cost.memory);

  return EmbeddingCost(total_processing, total_memory);
}
} // namespace LibClone
