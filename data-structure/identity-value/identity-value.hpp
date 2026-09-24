#pragma once

#include <optional>

template <class T>
struct IdentityValue {
  using io_via_val = void;
  using unwrap_via_val = void;

  std::optional<T> value;

  IdentityValue() = default;
  IdentityValue(const T& value) : value(value) {}

  const T& val() const {
    assert(has_value());
    return value.value();
  }

  operator T() const {
    return val();
  }
  bool has_value() const {
    return value.has_value();
  }

  friend bool operator==(const IdentityValue& x, const IdentityValue& y) {
    if (!x.has_value() || !y.has_value()) {
      if (!x.has_value() && !y.has_value()) return true;
      return false;
    }
    return x.val() == y.val();
  }
};