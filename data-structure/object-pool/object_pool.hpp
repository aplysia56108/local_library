#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
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
  ObjectPool(std::size_t buffer = 8) {
    pool_.reserve(buffer);
  };

  /** @brief 少なくとも n
   * スロット分の容量を事前に確保する。参照は無効化され得る。 */
  void reserve(std::size_t n) {
    assert(n < static_cast<std::size_t>(null_id));
    pool_.reserve(n);
  }

  /** @brief 既存スロットを再利用できない最悪の場合にも extra
   * 件追加できるよう予約する。 */
  void reserve_buffer(std::size_t buffer = 8) {
    if (pool_.capacity() - size() > buffer) return;

    reserve(pool_.reserve(pool_.capacity() << 1));
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