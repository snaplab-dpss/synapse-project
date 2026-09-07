#pragma once

#include <memory>
#include <utility>

namespace LibCore {

// Copy-on-write holder: copies share the value, and the first mutation through `mutate()`
// detaches (deep-copies) it. For members that are copied far more often than changed.
template <typename T> class Cow {
private:
  std::shared_ptr<T> value;

public:
  Cow() : value(std::make_shared<T>()) {}
  template <typename... Args> explicit Cow(Args &&...args) : value(std::make_shared<T>(std::forward<Args>(args)...)) {}
  Cow(const Cow &other)            = default;
  Cow(Cow &&other)                 = default;
  Cow &operator=(const Cow &other) = default;
  Cow &operator=(Cow &&other)      = default;

  const T &operator*() const { return *value; }
  const T *operator->() const { return value.get(); }
  const T &get() const { return *value; }

  // The value, private to this holder from now on.
  T &mutate() {
    if (value.use_count() > 1) {
      value = std::make_shared<T>(*value);
    }
    return *value;
  }

  void set(const T &new_value) { value = std::make_shared<T>(new_value); }
  void set(T &&new_value) { value = std::make_shared<T>(std::move(new_value)); }
};

} // namespace LibCore
