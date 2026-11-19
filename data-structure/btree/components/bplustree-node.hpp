#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include "data-structure/btree/components/bplustree-node-pools.hpp"

namespace Bplustree {

using pos_type = std::uint8_t;

template <class Traits>
struct Node;
template <class Traits>
struct Leaf;
template <class Traits>
struct Internal;

/**
 * @brief 位置操作だけを行う基本 Traits。遅延作用・独自集約値を持たない。
 * @tparam value_type 実要素の型。
 * @tparam Capacity Internal の子 ID 配列容量（3～255）。
 */
template <class value_type, std::size_t Capacity = 32>
struct Traits {
  static_assert(3 <= Capacity && Capacity <= 255);
  using T = value_type;
  static constexpr bool lazy = false;
  static constexpr pos_type kNodeCapacity = Capacity;
  static constexpr pos_type kNodeHalf = Capacity / 2;
  using Node = Bplustree::Node<Traits>;
  using Leaf = Bplustree::Leaf<Traits>;
  using Internal = Bplustree::Internal<Traits>;
};

/**
 * @brief Leaf / Internal に共通の親 Internal ID と親内の位置。
 * @details Leaf / Internal は異なる Pool の ID 空間を使用する。
 * Pool の再確保では ID は変わらないが、ノードへの参照・ポインタは
 * 無効になり得る。id() / Pool::index() による逆引きは生存中の参照だけに使う。
 */
template <class Traits>
struct Node {
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;
  static constexpr id_type null_id = Pools::null_id;

  id_type parent = null_id;
  pos_type position = 0;

  Traits::Internal& get_parent() const {
    assert(parent != null_id);
    return Pools::internals()[parent];
  }
};

/**
 * @brief 1実要素、または前後番兵を表す Leaf。
 * @details 位置や親が変わっても Leaf の Pool ID は変わらない。
 * ただし削除された ID は再利用され得るため、削除後の iterator は無効。
 */
template <class Traits>
struct Leaf : Traits::Node {
  using T = typename Traits::T;
  using N = typename Traits::Node;

 private:
  T value;

 public:
  explicit Leaf(const T& v) : N(), value(v) {}
  void set(const T& v) {
    value = v;
  }
  const T& get() const {
    return value;
  }
  T& get() {
    return value;
  }
};

/**
 * @brief 子 ID・部分木サイズ・反転フラグを保持する内部ノード。
 * @details level==0 では Leaf ID、level>0 では Internal ID を子とする。
 * 論理的に有効な子は children[0,count) だけで、残りは未規定。
 * 通常 count<NodeCap。挿入直後のみ count==NodeCap が一時的に許される。
 * 子の移動は ID を付け替えるだけで、Leaf の実体はコピーしない。
 */
template <class Traits>
struct Internal : Traits::Node {
  using pos_type = std::uint8_t;

  static constexpr pos_type NodeCap = Traits::kNodeCapacity;
  static constexpr pos_type HalfNodeCap = NodeCap / 2;
  using N = typename Traits::Node;
  using L = typename Traits::Leaf;
  using I = typename Traits::Internal;
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;

  static constexpr id_type null_id = Pools::null_id;

  pos_type count = 0;
  std::size_t size = 0;
  pos_type level = 0;
  bool is_reversed = false;
  // children[count,NodeCap) は未規定。null_id 終端は管理しない。
  std::array<id_type, NodeCap> children{};

  explicit Internal(pos_type lvl = 0) : N(), level(lvl) {}

  L& get_leaf(pos_type index) const {
    assert(level == 0 && index < count);
    return Pools::leaves()[children[index]];
  }
  I& get_child(pos_type index) const {
    assert(level > 0 && index < count);
    return Pools::internals()[children[index]];
  }

  /** @brief 子の要素数（level==0 なら常に1、その他は子 Internal の size）。 */
  std::size_t child_size(pos_type index) const {
    return level == 0 ? 1 : get_child(index).size;
  }
  pos_type child_count(pos_type index) const {
    return level == 0 ? 1 : get_child(index).count;
  }

  /**
   * @brief 自身の Internal Pool ID を、現在のオブジェクト参照から逆引きする。
   * @details Pool の再確保前に取得した古い参照に対して呼んではいけない。
   */
  id_type id() const {
    return Pools::internals().index(*static_cast<const I*>(this));
  }

  /**
   * @brief 子 ID を配置し、子の親・位置と必要な count / size を更新する。
   * @tparam FromOther false は同じ親内での位置変更（size 不変）。
   * @tparam ConsiderOtherSync true は元の親の suffix の切り離しを同期する。
   * @details FromOther=true, ConsiderOtherSync=false は新しい子や
   * 別木の根の配置に使う。途中では children に一時的な穴を許す。
   * 構造が完成するまで、未確定の配列に対して検索・集約を行わない。
   */
  template <bool FromOther = true, bool ConsiderOtherSync = true>
  void move_child(pos_type index, id_type child_id, id_type self_id = null_id) {
    assert(index < NodeCap && child_id != null_id);
    children[index] = child_id;
    if (index >= count) count = index + 1;
    if (level) {
      auto& child = Pools::internals()[child_id];
      if constexpr (FromOther) {
        size += child.size;
        if constexpr (ConsiderOtherSync) {
          assert(child.parent != null_id);
          auto& other = Pools::internals()[child.parent];
          assert(&other != this && other.size >= child.size);
          other.size -= child.size;
          if (child.position < other.count) other.count = child.position;
        }
        child.parent = self_id == null_id ? id() : self_id;
      }
      child.position = index;
    } else {
      auto& child = Pools::leaves()[child_id];
      if constexpr (FromOther) {
        ++size;
        if constexpr (ConsiderOtherSync) {
          assert(child.parent != null_id);
          auto& other = Pools::internals()[child.parent];
          assert(&other != this && other.size > 0);
          --other.size;
          if (child.position < other.count) other.count = child.position;
        }
        child.parent = self_id == null_id ? id() : self_id;
      }
      child.position = index;
    }
  }

  /**
   * @brief 派生 Internal が保持する追加情報の再計算フック。
   * @details 基本 Internal は何も保持しないため空実装。
   */
  void refresh_metadata() {}
  /** @brief 派生 refresh_metadata() を呼ぶ。size の再集計は行わない。 */
  void refresh() {
    static_cast<I*>(this)->refresh_metadata();
  }

  /**
   * @brief 保留中の反転を children に反映し、必要なら子へ反転を伝搬する。
   * @details is_reversed が false なら何もしない。O(count)。
   */
  void apply_reverse() {
    if (!is_reversed) return;
    std::reverse(children.begin(), children.begin() + count);
    is_reversed = false;
    for (pos_type i = 0; i < count; ++i) {
      this->template move_child<false>(i, children[i]);
      if (level) get_child(i).push_reverse();
    }
  }
  /**
   * @brief 自身の論理順序を反転するフラグと派生の集約情報を更新する。
   * @details children の並べ替え自体は apply_reverse() まで遅延する。
   */
  void push_reverse() {
    is_reversed = !is_reversed;
    static_cast<I*>(this)->push_reverse_impl();
  }
  void push_reverse_impl() {}
  void apply_lazy() {}
  /** @brief 保留中の反転を確定し、続いて派生 Lazy 作用を子へ伝搬する。 */
  void materialize() {
    apply_reverse();
    static_cast<I*>(this)->apply_lazy();
  }

  /**
   * @brief index に子を挿入する。
   * @return count==NodeCap となり即座のノード分割が必要なら true。
   */
  bool insert(id_type child, pos_type index) {
    assert(child != null_id && index <= count && count < NodeCap);
    // move_child により count が増えるので、ループ上限を退避する。
    const pos_type old_count = count;
    for (pos_type i = old_count; i > index; --i)
      this->template move_child<false>(i, children[i - 1]);
    this->template move_child<true, false>(index, child);
    if (count == NodeCap) return true;
    refresh();
    return false;
  }

  /**
   * @brief left_count 個の子を自身に残し、残りを新しい右兄弟へ移す。
   * @return 新しい右 Internal の Pool ID（同じ level）。
   */
  id_type split(pos_type left_count = HalfNodeCap) {
    assert(count <= NodeCap && left_count <= count);
    assert(left_count < NodeCap && count - left_count < NodeCap);
    // 移動元のcountが縮むので、先に移動数を保存する。
    const pos_type right_count = count - left_count;
    // Pool 再確保で this が無効化されるため、ID と level を先に保存する。
    const id_type left_id = id();
    const pos_type current_level = level;
    const id_type right_id = Pools::make_internal(current_level);
    auto& left = Pools::internals()[left_id];
    auto& right = Pools::internals()[right_id];
    // 右端から取り出すと移動元の[0,count)を常に有効に保てる。
    for (pos_type i = right_count; i > 0;) {
      --i;
      right.move_child(i, left.children[left_count + i], right_id);
    }
    left.refresh();
    right.refresh();
    return right_id;
  }

  /**
   * @brief index 番目の子とその部分木を Pool に回収し、後続を詰める。
   * @return 自身の count が 0 になった場合 true。
   */
  bool erase(pos_type index) {
    assert(count < NodeCap && index < count);
    size -= child_size(index);
    Pools::recycle(children[index], level == 0);
    for (pos_type i = index + 1; i < count; ++i)
      this->template move_child<false>(i - 1, children[i]);
    --count;
    refresh();
    return count == 0;
  }

  /**
   * @brief この部分木の index 番目の Leaf ID を探す（番兵を含む位置）。
   * @tparam WithLazy true なら経路上の Lazy 作用を Leaf まで確定する。
   * @param value_clean WithLazy=false 時、未伝搬の作用があれば false にする。
   * @details 反転は探索に必要なため常に確定する。
   */
  template <bool WithLazy = false>
  id_type locate(std::size_t index, bool* value_clean = nullptr) {
    apply_reverse();
    if constexpr (WithLazy) {
      static_cast<I*>(this)->apply_lazy();
    } else if constexpr (requires { Traits::lazy; }) {
      if constexpr (Traits::lazy) {
        if (value_clean && *value_clean && static_cast<I*>(this)->pending)
          *value_clean = false;
      }
    }
    if (level == 0) {
      assert(index < count);
      return children[index];
    }
    for (pos_type i = 0; i < count; ++i) {
      auto& child = get_child(i);
      if (index < child.size)
        return child.template locate<WithLazy>(index, value_clean);
      index -= child.size;
    }
    assert(false);
    return null_id;
  }

  /**
   * @brief 隣の兄弟と1ノードに統合できる場合、子を付け替えて余剰 ID を回収する。
   * @details 統合できない場合は何もしない。対象は level>0 の Internal 子。
   */
  void integrate_child(pos_type index) {
    assert(level > 0 && index < count);
    if (index && child_count(index - 1) + child_count(index) < NodeCap) {
      auto& left = get_child(index - 1);
      auto& right = get_child(index);
      left.materialize();
      right.materialize();
      // 右側のcountが移動ごとに縮むので、開始時の個数を保存する。
      const pos_type left_count = left.count;
      const pos_type right_count = right.count;
      const id_type left_id = left.id();
      for (pos_type i = right_count; i > 0;) {
        --i;
        left.move_child(left_count + i, right.children[i], left_id);
      }
      left.refresh();
    } else if (index + 1 < count &&
               child_count(index + 1) + child_count(index) < NodeCap) {
      auto& left = get_child(index);
      auto& right = get_child(index + 1);
      left.materialize();
      right.materialize();
      // shiftで右側のcountが増え、移動で左側のcountが縮む。
      // 両方の移動前の個数を保存する。
      const pos_type left_count = left.count;
      const pos_type right_count = right.count;
      const id_type right_id = right.id();
      for (pos_type i = right_count; i > 0;) {
        --i;
        right.template move_child<false>(left_count + i, right.children[i]);
      }
      for (pos_type i = left_count; i > 0;) {
        --i;
        right.move_child(i, left.children[i], right_id);
      }
      right.refresh();
    } else {
      return;
    }
    erase(index);
  }

  /**
   * @brief 自身を隣の兄弟と統合できる場合に親へ委譲する。
   * @details 成功時にはこの Internal ID が回収され、自身の参照が失効し得る。
   */
  void integrate() {
    if (this->parent == null_id) return;
    const auto parent_id = this->parent;
    const auto position = this->position;
    Pools::internals()[parent_id].integrate_child(position);
  }
};

}  // namespace Bplustree
