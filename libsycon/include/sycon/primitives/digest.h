#pragma once

#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_learn.hpp>

#include "../time.h"
#include "../log.h"
#include "../config.h"
#include "../buffer.h"

namespace sycon {

struct digest_field_t {
  std::string name;
  bf_rt_id_t id;
  bits_t size;
};

class Digest {
private:
  const std::string name;

  const bf_rt_target_t dev_tgt;
  const bfrt::BfRtInfo *info;
  std::shared_ptr<bfrt::BfRtSession> session;

  const bfrt::BfRtLearn *learn_obj;
  const std::vector<digest_field_t> fields;

public:
  Digest(const std::string &_name)
      : name(_name), dev_tgt(cfg.dev_tgt), info(cfg.info), session(cfg.session), learn_obj(build_learn_obj(info, name)),
        fields(build_fields(learn_obj)) {
    LOG_DEBUG("");
    LOG_DEBUG("Digest: %s", name.c_str());
    LOG_DEBUG("Fields:");
    for (auto f : fields) {
      LOG_DEBUG("  %s (%lu bits) (id=%u)", f.name.c_str(), f.size, f.id);
    }
  }

  const std::string &get_name() const { return name; }
  const std::vector<digest_field_t> &get_fields() const { return fields; }

  void register_callback(const bfrt::bfRtCbFunction &callback_fn, void *cookie) const {
    bf_status_t bf_status = learn_obj->bfRtLearnCallbackRegister(session, dev_tgt, callback_fn, cookie);
    ASSERT_BF_STATUS(bf_status);
  }

  void notify_ack(bf_rt_learn_msg_hdl *const learn_msg_hdl) const {
    bf_status_t bf_status = learn_obj->bfRtLearnNotifyAck(session, learn_msg_hdl);
    ASSERT_BF_STATUS(bf_status);
  }

  buffer_t get_value(const bfrt::BfRtLearnData *data) const {
    assert(data && "Invalid data");

    bf_status_t bf_status;

    bytes_t digest_size = 0;
    for (const digest_field_t &field : fields) {
      digest_size += field.size / 8;
    }

    buffer_t digest_buffer(digest_size);

    bytes_t offset = 0;
    for (const digest_field_t &field : fields) {
      const bytes_t field_size = field.size / 8;

      bool is_ptr{false};
      bf_status = learn_obj->learnFieldIsPtrGet(field.id, &is_ptr);
      ASSERT_BF_STATUS(bf_status);

      if (is_ptr) {
        std::vector<u8> value(field_size);
        bf_status = data->getValue(field.id, field_size, value.data());
        ASSERT_BF_STATUS(bf_status);
        for (size_t i = 0; i < field_size; i++) {
          digest_buffer.set_big_endian(offset + i, 1, value[i]);
        }
      } else {
        u64 digest_field_value;
        bf_status = data->getValue(field.id, &digest_field_value);
        ASSERT_BF_STATUS(bf_status);
        digest_buffer.set(offset, field_size, digest_field_value);
      }

      offset += field_size;
    }

    return digest_buffer;
  }

private:
  static const bfrt::BfRtLearn *build_learn_obj(const bfrt::BfRtInfo *info, const std::string &name) {
    const bfrt::BfRtLearn *learn_obj;
    bf_status_t bf_status = info->bfrtLearnFromNameGet(name, &learn_obj);
    ASSERT_BF_STATUS(bf_status);
    return learn_obj;
  }

  static std::vector<digest_field_t> build_fields(const bfrt::BfRtLearn *learn_obj) {
    std::vector<digest_field_t> fields;

    std::vector<bf_rt_id_t> fields_ids;
    bf_status_t bf_status = learn_obj->learnFieldIdListGet(&fields_ids);
    ASSERT_BF_STATUS(bf_status);

    for (bf_rt_id_t id : fields_ids) {
      std::string name;
      bf_status = learn_obj->learnFieldNameGet(id, &name);
      ASSERT_BF_STATUS(bf_status);

      size_t size;
      bf_status = learn_obj->learnFieldSizeGet(id, &size);
      ASSERT_BF_STATUS(bf_status);

      assert(size > 0);

      if (size % 8 != 0) {
        size += 8 - (size % 8);
      }

      fields.push_back({name, id, static_cast<bits_t>(size)});
    }

    return fields;
  }
};

} // namespace sycon