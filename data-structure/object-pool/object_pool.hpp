#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

template <class T>
class ObjectPool {
 public:
  using id_type = std::uint32_t;
  static constexpr id_type null_id = std::numeric_limits<id_type>::max();

 private:
  std::vector<T> pool_;
  std::vector<id_type> free_ids_;

 public:
  ObjectPool() {};

  template <class... Args>
  id_type emplace(Args&&... args) {
    if (!free_ids_.empty()) {
      id_type id = free_ids_.back();
      free_ids_.pop_back();

      pool_[id] = T(std::forward<Args>(args)...);
      return id;
    }

    assert(pool_.size() < std::numeric_limits<id_type>::max());

    id_type id = static_cast<id_type>(pool_.size());
    pool_.emplace_back(std::forward<Args>(args)...);
    return id;
  }

  void erase(id_type id) {
    assert(id < pool_.size());
    free_ids_.push_back(id);
  }

  T& operator[](id_type id) {
    return pool_[id];
  }

  const T& operator[](id_type id) const {
    return pool_[id];
  }

  std::size_t size() const {
    return pool_.size() - free_ids_.size();
  }
};