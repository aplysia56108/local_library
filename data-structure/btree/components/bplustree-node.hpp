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

// template <class Traits>
struct NodeBase;
// template <class Traits>
struct Leaf;
// template <class Traits>
struct Node;
// template <class Traits>
struct UpperNode;
// template <class Traits>
struct BottomNode;
// template <class Traits>
struct NodeCRTP;

/**
 * @brief 位置操作だけを行う基本 Traits。遅延作用・独自集約値を持たない。
 * @tparam value_type 実要素の型。
 * @tparam Capacity Internal の子 ID 配列容量（3～255）。
 */
using value_type = long long;
constexpr std::size_t Capacity = 32;
// template <class value_type, std::size_t Capacity = 32>
struct Traits {
  static_assert(3 <= Capacity && Capacity <= 255);
  using T = value_type;
  static constexpr bool lazy = false;
  static constexpr pos_type kNodeCapacity = Capacity;
  static constexpr pos_type kNodeHalf = Capacity / 2;
  using NodeBase = Bplustree::NodeBase;
  using Leaf = Bplustree::Leaf;
  using Node = Bplustree::Node;
  using UpperNode = Bplustree::UpperNode;
  using BottomNode = Bplustree::BottomNode;
  using NodeCRTP = Bplustree::NodeCRTP;
  // using NodeBase = Bplustree::NodeBase<Traits>;
  // using Leaf = Bplustree::Leaf<Traits>;
  // using Node = Bplustree::Node<Traits>;
  // using UpperNode = Bplustree::UpperNode<Traits>;
  // using BottomNode = Bplustree::BottomNode<Traits>;
};

/**
 * @brief Leaf / Internal に共通の親 Internal ID と親内の位置。
 * @details Leaf / Internal は異なる Pool の ID 空間を使用する。
 * Pool の再確保では ID は変わらないが、ノードへの参照・ポインタは
 * 無効になり得る。id() / Pool::index() による逆引きは生存中の参照だけに使う。
 */
// template <class Traits>
struct NodeBase {
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;
  static constexpr id_type null_id = Pools::null_id;

  id_type parent = null_id;
  pos_type position = 0;
};

/**
 * @brief 1実要素、または前後番兵を表す Leaf。
 * @details 位置や親が変わっても Leaf の Pool ID は変わらない。
 * ただし削除された ID は再利用され得るため、削除後の iterator は無効。
 */
// template <class Traits>
struct Leaf : Traits::NodeBase {
  using T = typename Traits::T;
  using NB = typename Traits::NodeBase;
  using Parent = Traits::BottomNode;
  using Pools = NodePools<Traits>;

  std::size_t size = 0;
  pos_type count = 0;

 private:
  T value;

 public:
  explicit Leaf(const T& v) : NB(), value(v) {}

  void set(const T& v) {
    value = v;
  }
  const T& get() const {
    return value;
  }
  T& get() {
    return value;
  }
  Parent& get_parent() {
    return Pools::bottoms()[this->parent];
  }
};

/**
 * @brief 子 ID・部分木サイズ・反転フラグを保持する内部ノード。
 * @details level==0 では Leaf ID、level>0 では Internal ID を子とする。
 * 論理的に有効な子は children[0,count) だけで、残りは未規定。
 * 通常 count<NodeCap。挿入直後のみ count==NodeCap が一時的に許される。
 * 子の移動は ID を付け替えるだけで、Leaf の実体はコピーしない。
 */
// template <class Traits>
struct Node : Traits::NodeBase {
  using pos_type = std::uint8_t;
  using U = typename Traits::UpperNode;

  static constexpr pos_type NodeCap = Traits::kNodeCapacity;
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;

  static constexpr id_type null_id = Pools::null_id;

  pos_type count = 0;
  std::size_t size = 0;
  pos_type level = 0;
  bool is_reversed = false;
  // children[count,NodeCap) は未規定。null_id 終端は管理しない。
  std::array<id_type, NodeCap> children{};

  explicit Node(pos_type lvl = 0) : Traits::NodeBase(), level(lvl) {}

  U& get_parent() const {
    return Pools::uppers()[this->parent];
  }
};

/**
 * @brief Internal / BottomInternal の共通操作を派生型の静的型で選ぶ CRTP。
 * @details
 * Derived が持つ *_typed<IsBottom>() を呼び、Leaf 子と Internal 子で
 * 異なる処理を compile-time に確定する。フィールドを持たない mixin。
 * 既存の Internal& API から呼ぶ場合だけ、level による境界 dispatch を残す。
 *
 * これは実験版: 上層と最下層の具体型は兄弟クラスとし、
 * Traits::Internal を共通基底にして metadata・既存 API を共有する。
 */
using Derived = Traits::UpperNode;
constexpr bool IsBottom = false;
// template <class Traits, class Derived, bool IsBottom>
struct NodeCRTP : Traits::Node {
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;

 private:
  Derived& self();
  const Derived& self() const;

 public:
  std::size_t child_size(pos_type index) const;
  pos_type child_count(pos_type index) const;
  id_type id() const;
  template <bool FromOther = true, bool ConsiderOtherSync = true>
  void move_child(pos_type index, id_type child_id,
                  id_type self_id = Pools::null_id);
  void refresh();
  void apply_reverse();
  void push_reverse();
  void materialize();
  bool insert(id_type child, pos_type index);
  id_type split(pos_type left_count = Traits::kNodeCapacity / 2);
  bool erase(pos_type index);
  template <bool WithLazy = false>
  id_type locate(std::size_t index, bool* value_clean = nullptr);
  void integrate();
};

// template <class Traits>
struct BottomNode : Traits::NodeCRTP {};

// template <class Traits>
struct UpperNode : Traits::NodeCRTP {
  using pos_type = std::uint8_t;
  static constexpr pos_type NodeCap = Traits::kNodeCapacity;
  static constexpr pos_type HalfNodeCap = NodeCap / 2;
  using NB = typename Traits::NodeBase;
  using L = typename Traits::Leaf;
  using N = typename Traits::Node;
  using U = typename Traits::UpperNode;
  using B = typename Traits::BottomNode;
  using CRTP = Traits::NodeCRTP;
  using Pools = NodePools<Traits>;
  using id_type = typename Pools::id_type;

  static constexpr id_type null_id = Pools::null_id;

  std::array<id_type, NodeCap> sizes{};
  std::array<pos_type, NodeCap> counts{};

  explicit UpperNode(pos_type lvl) : N() {};

  template <Pools::NodeType Type>
  auto& get_child(pos_type index) const {
    assert(this->level > 0 && index < this->count);
    static_assert(Type == Pools::NodeType::Bottom ||
                  Type == Pools::NodeType::Upper);
    assert((level == 1 && Type == Pools::NodeType::Bottom) ||
           (level > 1 && Type == Pools::NodeType::Upper));

    return Pools::get<Type>(this->children[index]);
  }

  /**
   * @brief 派生 Internal が保持する追加情報の再計算フック。
   * @details 基本 Internal は何も保持しないため空実装。
   */
  void refresh_metadata() {}

  void push_reverse_impl() {}

  void apply_lazy() {}

  // TODO:一旦時nodeのキャッシュは使わない。今後キャッシュを使う予定
  /** @brief 子の要素数（level==0 なら常に1、その他は子 Internal の size）。 */
  using CRTP::child_count;
  using CRTP::child_size;

  /**
   * @brief 自身の Internal Pool ID を、現在のオブジェクト参照から逆引きする。
   * @details Pool の再確保前に取得した古い参照に対して呼んではいけない。
   */
  using CRTP::id;

  /**
   * @brief 子 ID を配置し、子の親・位置と必要な count / size を更新する。
   * @tparam FromOther false は同じ親内での位置変更（size 不変）。
   * @tparam ConsiderOtherSync true は元の親の suffix の切り離しを同期する。
   * @details FromOther=true, ConsiderOtherSync=false は新しい子や
   * 別木の根の配置に使う。途中では children に一時的な穴を許す。
   * 構造が完成するまで、未確定の配列に対して検索・集約を行わない。
   */
  using CRTP::move_child;

  /** @brief 派生 refresh_metadata() を呼ぶ。size の再集計は行わない。 */
  using CRTP::refresh;

  /**
   * @brief 保留中の反転を children に反映し、必要なら子へ反転を伝搬する。
   * @details is_reversed が false なら何もしない。O(count)。
   */
  using CRTP::apply_reverse;
  /**
   * @brief 自身の論理順序を反転するフラグと派生の集約情報を更新する。
   * @details children の並べ替え自体は apply_reverse() まで遅延する。
   */
  using CRTP::push_reverse;

  /** @brief 保留中の反転を確定し、続いて派生 Lazy 作用を子へ伝搬する。 */
  using CRTP::materialize;

  /**
   * @brief index に子を挿入する。
   * @return count==NodeCap となり即座のノード分割が必要なら true。
   */
  using CRTP::insert;

  /**
   * @brief left_count 個の子を自身に残し、残りを新しい右兄弟へ移す。
   * @return 新しい右 Internal の Pool ID（同じ level）。
   */
  using CRTP::split;

  /**
   * @brief index 番目の子とその部分木を Pool に回収し、後続を詰める。
   * @return 自身の count が 0 になった場合 true。
   */
  using CRTP::erase;

  /**
   * @brief この部分木の index 番目の Leaf ID を探す（番兵を含む位置）。
   * @tparam WithLazy true なら経路上の Lazy 作用を Leaf まで確定する。
   * @param value_clean WithLazy=false 時、未伝搬の作用があれば false にする。
   * @details 反転は探索に必要なため常に確定する。
   */

  using CRTP::locate;

  /**
   * @brief 隣の兄弟と1ノードに統合できる場合、子を付け替えて余剰 ID
   * を回収する。
   * @details 統合できない場合は何もしない。対象は level>0 の Internal 子。
   */
  void integrate_child(pos_type index) {
    assert(this->level > 0 && index < this->count);

    auto f = [this](auto& left, auto& right, const pos_type left_count,
                    const pos_type right_count, const bool to_left) -> void {
      left.materialize();
      right.materialize();
      // 右側のcountが移動ごとに縮むので、開始時の個数を保存する。
      if (to_left) {
        const id_type left_id = left.id();
        for (pos_type i = right_count; i > 0;) {
          --i;
          left.move_child(left_count + i, right.children[i], left_id);
        }
        left.refresh();
      } else {
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
      }
    };

    const std::size_t left_count = index ? child_count(index - 1) : 1000;
    const std::size_t middle_count = child_count(index);
    const std::size_t right_count =
        index + 1 < count ? child_count(index + 1) : 1000;
    if (left_count + middle_count < static_cast<std::size_t>(NodeCap)) {
      if (level == 1) {
        auto& left = get_child<Pools::NodeType::Bottom>(index - 1);
        auto& right = get_child<Pools::NodeType::Bottom>(index);
        f(left, right, left_count, middle_count, true);
      } else {
        auto& left = get_child<Pools::NodeType::Upper>(index - 1);
        auto& right = get_child<Pools::NodeType::Upper>(index);
        f(left, right, left_count, middle_count, true);
      }
    } else if (middle_count + right_count < static_cast<std::size_t>(NodeCap)) {
      if (level == 1) {
        auto& left = get_child<Pools::NodeType::Bottom>(index);
        auto& right = get_child<Pools::NodeType::Bottom>(index + 1);
        f(left, right, middle_count, right_count, false);
      } else {
        auto& left = get_child<Pools::NodeType::Upper>(index);
        auto& right = get_child<Pools::NodeType::Upper>(index + 1);
        f(left, right, middle_count, right_count, false);
      }
    } else {
      return;
    }
    erase(index);
  }

  /**
   * @brief 自身を隣の兄弟と統合できる場合に親へ委譲する。
   * @details 成功時にはこの Internal ID が回収され、自身の参照が失効し得る。
   */
  using CRTP::integrate;
};

inline Derived& NodeCRTP::self() {
  return static_cast<Derived&>(*this);
}

inline const Derived& NodeCRTP::self() const {
  return static_cast<const Derived&>(*this);
}

inline std::size_t NodeCRTP::child_size(pos_type index) const {
  if constexpr (IsBottom) {
    return 1;
  } else {
    return self().get_child<Pools::NodeType::Upper>(index).size;
  }
}

inline pos_type NodeCRTP::child_count(pos_type index) const {
  if constexpr (IsBottom) {
    return 1;
  } else {
    return self().get_child<Pools::NodeType::Upper>(index).count;
  }
}

inline NodeCRTP::id_type NodeCRTP::id() const {
  if constexpr (IsBottom) {
    return Pools::bottoms().index(self());
  } else {
    return Pools::uppers().index(self());
  }
}

template <bool FromOther, bool ConsiderOtherSync>
inline void NodeCRTP::move_child(pos_type index, id_type child_id,
                                 id_type self_id) {
  assert(index < NodeCap && child_id != null_id);
  this->children[index] = child_id;
  if (index >= this->count) this->count = index + 1;

  auto f = [this, index, self_id](auto& child) -> void {
    if constexpr (FromOther) {
      this->size += child.size;
      if constexpr (ConsiderOtherSync) {
        assert(child.parent != null_id);
        auto& other = child.get_parent();
        assert(&other != this && other.size >= child.size);
        other.size -= child.size;
        if (child.position < other.count) other.count = child.position;
      }
      child.parent = self_id == null_id ? id() : self_id;
    }
    child.position = index;
  };

  if constexpr (IsBottom) {
    auto& child = Pools::get<0>(child_id);
    f(child);
  } else if (level == 1) {
    auto& child = Pools::get<1>(child_id);
    self().sizes[index] = child.size;
    self().counts[index] = child.count;
    f(child);
  } else {
    auto& child = Pools::get<2>(child_id);
    self().sizes[index] = child.size;
    self().counts[index] = child.count;
    f(child);
  }
}

inline void NodeCRTP::refresh() {
  self().refresh_metadata();
}

inline void NodeCRTP::apply_reverse() {
  if (!this->is_reversed) return;
  std::reverse(this->children.begin(), this->children.begin() + this->count);
  this->is_reversed = false;

  for (pos_type i = 0; i < this->count; ++i) {
    move_child<false>(i, this->children[i]);
    if constexpr (!IsBottom) {
      if (level == 1)
        self().get_child<Pools::NodeType::Bottom>(i).push_reverse();
      else
        self().get_child<Pools::NodeType::Upper>(i).push_reverse();
    }
  }
}

inline void NodeCRTP::push_reverse() {
  this->is_reversed ^= true;
  self().push_reverse_impl();
}

inline void NodeCRTP::materialize() {
  apply_reverse();
  self().apply_lazy();
}
inline bool NodeCRTP::insert(id_type child, pos_type index) {
  assert(child != null_id && index <= this->count && this->count < NodeCap);
  // move_child により count が増えるので、ループ上限を退避する。
  const pos_type old_count = this->count;
  for (pos_type i = old_count; i > index; --i)
    move_child<false>(i, this->children[i - 1]);
  move_child<true, false>(index, child);
  if (this->count == NodeCap) return true;
  refresh();
  return false;
}
inline NodeCRTP::id_type NodeCRTP::split(pos_type left_count) {
  assert(this->count <= NodeCap && left_count <= this->count);
  assert(left_count < NodeCap && this->count - left_count < NodeCap);
  // 移動元のcountが縮むので、先に移動数を保存する。
  const pos_type right_count = this->count - left_count;
  // Pool 再確保で this が無効化されるため、ID と level を先に保存する。
  const id_type left_id = id();
  const pos_type current_level = this->level;

  auto f = [right_count, left_count](auto& left, auto& right,
                                     const id_type& right_id) -> void {
    for (pos_type i = 0; i < right_count; ++i) {
      right.move_child(i, left.children[left_count + i], right_id);
    }
    left.refresh();
    right.refresh();
  };
  id_type right_id;
  if constexpr (IsBottom) {
    right_id = Pools::make_bottom();
    auto& left = Pools::get<1>(left_id);
    auto& right = Pools::get<1>(right_id);
    f(left, right, right_id);
  } else {
    right_id = Pools::make_upper(current_level);
    auto& left = Pools::get<2>(left_id);
    auto& right = Pools::get<2>(right_id);
    f(left, right, right_id);
  }
  return right_id;
}

inline bool NodeCRTP::erase(pos_type index) {
  assert(this->count < NodeCap && index < this->count);
  this->size -= child_size(index);
  Pools::recycle(this->children[index]);
  for (pos_type i = index + 1; i < this->count; ++i)
    this->template move_child<false>(i - 1, this->children[i]);
  --this->count;
  refresh();
  return this->count == 0;
}

template <bool WithLazy>
inline NodeCRTP::id_type NodeCRTP::locate(std::size_t index,
                                          bool* value_clean) {
  apply_reverse();
  if constexpr (WithLazy) {
    self().apply_lazy();
  } else if constexpr (requires { Traits::lazy; }) {
    if constexpr (Traits::lazy) {
      if (value_clean && *value_clean && static_cast<Node*>(this)->pending)
        *value_clean = false;
    }
  }
  for (pos_type i = 0; i < this->count; ++i) {
    if (index < child_size(i)) {
      if (level == 1)
        return self()
            .get_child<Pools::NodeType::Bottom>(i)
            .template locate<WithLazy>(index, value_clean);
      return self()
          .get_child<Pools::NodeType::Upper>(i)
          .template locate<WithLazy>(index, value_clean);
    }
    index -= child_size(i);
  }
  assert(false);
  return null_id;
}

inline void NodeCRTP::integrate() {
  if (this->parent == null_id) return;
  get_parent().integrate_child(this->position);
}

}  // namespace Bplustree
