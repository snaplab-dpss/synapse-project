#pragma once

#include <LibClone/EmbeddingProfiler.h>

namespace LibClone {

class x86EmbeddingProfiler : public EmbeddingProfiler {
public:
  x86EmbeddingProfiler()  = default;
  ~x86EmbeddingProfiler() = default;

  TargetType get_target_type() const override { return TargetType::x86; }

  const EmbeddingCost visitMapAllocate(const Call *node) override;
  const EmbeddingCost visitMapGet(const Call *node) override;
  const EmbeddingCost visitMapPut(const Call *node) override;
  const EmbeddingCost visitMapErase(const Call *node) override;
  const EmbeddingCost visitExpireItemsSingleMap(const Call *node) override;
  const EmbeddingCost visitExpireItemsSingleMapIteratively(const Call *node) override;

  const EmbeddingCost visitVectorAllocate(const Call *node) override;
  const EmbeddingCost visitVectorBorrow(const Call *node) override;
  const EmbeddingCost visitVectorReturn(const Call *node) override;
  const EmbeddingCost visitVectorClear(const Call *node) override;
  const EmbeddingCost visitVectorSampleLt(const Call *node) override;

  const EmbeddingCost visitDchainAllocate(const Call *node) override;
  const EmbeddingCost visitDchainAllocateNewIndex(const Call *node) override;
  const EmbeddingCost visitDchainRejuvenateIndex(const Call *node) override;
  const EmbeddingCost visitDchainExpireOneIndex(const Call *node) override;
  const EmbeddingCost visitDchainIsIndexAllocated(const Call *node) override;
  const EmbeddingCost visitDchainFreeIndex(const Call *node) override;

  const EmbeddingCost visitCMSAllocate(const Call *node) override;
  const EmbeddingCost visitCMSIncrement(const Call *node) override;
  const EmbeddingCost visitCMSCountMin(const Call *node) override;
  const EmbeddingCost visitCMSPeriodicCleanup(const Call *node) override;

  const EmbeddingCost visitLPMAllocate(const Call *node) override;
  const EmbeddingCost visitLPMFree(const Call *node) override;
  const EmbeddingCost visitLPMFromFile(const Call *node) override;
  const EmbeddingCost visitLPMUpdate(const Call *node) override;
  const EmbeddingCost visitLPMLookup(const Call *node) override;

  const EmbeddingCost visitTokenBucketAllocate(const Call *node) override;
  const EmbeddingCost visitTokenBucketIsTracing(const Call *node) override;
  const EmbeddingCost visitTokenBucketTrace(const Call *node) override;
  const EmbeddingCost visitTokenBucketUpdateAndCheck(const Call *node) override;
  const EmbeddingCost visitTokenBucketExpire(const Call *node) override;

  // EmbeddingCost visitGenericCall(const Call *node, DataStructures &data_structures) override;

private:
  // std::unique_ptr<AllocParams> extract_params(const Call *node) const;

  struct x86Map : public DataStructure {
    u32 capacity;
    u32 key_size;

    x86Map(u32 c, u32 k);

    std::string to_string() const override;
    EmbeddingCost alloc_cost() const override;
    EmbeddingCost access_cost() const override;
  };

  struct x86Vector : public DataStructure {
    u32 elem_size;
    u32 capacity;

    x86Vector(u32 e, u32 c);

    std::string to_string() const override;
    EmbeddingCost alloc_cost() const override;
    EmbeddingCost access_cost() const override;
  };

  struct x86DChain : public DataStructure {
    u32 index_range;

    x86DChain(u32 r);

    std::string to_string() const override;
    EmbeddingCost alloc_cost() const override;
    EmbeddingCost access_cost() const override;
  };

  struct x86CMS : public DataStructure {
    x86Vector vector;
    u32 height;
    u32 width;

    x86CMS(u32 h, u32 w);

    std::string to_string() const override;
    EmbeddingCost alloc_cost() const override;
    EmbeddingCost access_cost() const override;
  };

  struct x86BloomFilter : public DataStructure {
    u32 height;
    u32 width;

    x86BloomFilter(u32 h, u32 w);

    std::string to_string() const override;
    EmbeddingCost alloc_cost() const override;
    EmbeddingCost access_cost() const override;
  };

  struct x86LPM : public DataStructure {
    x86LPM();

    std::string to_string() const override;
    EmbeddingCost alloc_cost() const override;
    EmbeddingCost access_cost() const override;
  };

  struct x86TokenBucket : public DataStructure {
    x86Map map;
    x86Vector keys;
    x86Vector buckets;
    x86DChain dchain;

    x86TokenBucket(u32 c, u32 k);

    std::string to_string() const override;
    EmbeddingCost alloc_cost() const override;
    EmbeddingCost access_cost() const override;
  };
};

} // namespace LibClone
