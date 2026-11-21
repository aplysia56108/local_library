#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

template <class T,
          std::uint32_t MaxId = std::numeric_limits<std::uint32_t>::max() - 1,
          std::uint32_t NullId = std::numeric_limits<std::uint32_t>::max()>
class ObjectPool {
 public:
  using id_type = std::uint32_t;
  static constexpr id_type null_id = NullId;
  static constexpr id_type max_id = MaxId;
  static_assert(max_id < null_id);

 private:
  std::vector<T> pool_;
  std::vector<id_type> free_ids_;

 public:
  // std::vectorのconstexprデフォルト構築により静的Poolをconstinit可能にする。
  constexpr ObjectPool() = default;

  /** @brief 少なくとも n
   * スロット分の容量を事前に確保する。参照は無効化され得る。 */
  void reserve(std::size_t n) {
    assert(n <= max_id + 1);
    pool_.reserve(n);
  }

  /** @brief 再利用可能スロットと未使用容量の合計が buffer
   * 以上になるよう確保する。 */
  void reserve_buffer(std::size_t buffer = 8) {
    const std::size_t required = size() + buffer;
    if (pool_.capacity() >= required) return;

    reserve(std::max(required, pool_.capacity() * 2));
  }

  /** @brief 有効な参照から ID を取得する。O(1)。reserve/emplace
   * による移動後の古い参照は不可。 */
  id_type index(const T& value) const {
    const T* ptr = std::addressof(value);
    // 呼び出し側は、この Pool に現在格納されているオブジェクトを渡す。
    const auto offset = ptr - pool_.data();
    assert(offset >= 0 && static_cast<std::size_t>(offset) < pool_.size());
    return static_cast<id_type>(offset);
  }

  template <class... Args>
  id_type emplace(Args&&... args) {
    if (!free_ids_.empty()) {
      id_type id = free_ids_.back();
      free_ids_.pop_back();

      pool_[id] = T(std::forward<Args>(args)...);
      return id;
    }

    assert(pool_.size() <= max_id);

    id_type id = static_cast<id_type>(pool_.size());
    pool_.emplace_back(std::forward<Args>(args)...);
    return id;
  }

  void erase(id_type id) {
    assert(id < pool_.size());
    free_ids_.push_back(id);
  }

  T& operator[](id_type id) {
    assert(id <= max_id && id < pool_.size());
    return pool_[id];
  }

  const T& operator[](id_type id) const {
    assert(id <= max_id && id < pool_.size());
    return pool_[id];
  }

  std::size_t size() const {
    return pool_.size() - free_ids_.size();
  }
};