# UpperNode / BottomNode: components 移行メモ

この文書は `perf,-feat-introducing-bottomInternal` 向けの **Draft PR** の実装契約を説明する。
元ブランチの Node 本体はテンプレートをコメントアウトして補完しながら開発中であり、
本 PR は Node 本体の完成を意味しない。

## 実装済みの境界

- `NodePools<Traits>` は Leaf / Bottom / Upper の独立した Pool を保持する。
- Internal ID の最上位ビットは Bottom を表す。Upper はタグなし。
  Leaf ID は別の空間にあり、同じ数値が存在しても意味が異なる。
- `Pools::get<Pools::NodeType::Bottom>(id)` /
  `Pools::get<Pools::NodeType::Upper>(id)` は、具象型の参照を返す。
  旧来の `get<0/1/2>()` も暫定的に維持。
- `Pools::with_internal(id, callback)` は、タグを一度確認して具体型を callback に渡す。
- `NodeAccess<Traits>::get(id)` は **共通フィールドを読むためだけ** に
  `Traits::Node&` を返す。更新・探索は `NodeAccess::with()` か
  `NodeAccess::refresh/materialize/locate/...()` で具象型を確保する。
- `Base` の挿入、削除、split/merge、reverse で使うノード操作は
  `NodeAccess` 経由。Root は tagged ID のまま保存できる。
- `Iterator` の経路上の反転・Lazy・集約更新も `NodeAccess` 経由。
- `AggregateTree` の積・区間作用・境界探索は ID を引き回して
  再帰のたびに具体型へ分岐する。
- Set / Aggregate のメタデータは `Traits::Node` という共有基底に置く。
  Upper/Bottom の両実体が継承し、値の重複を避ける。

## Node 側が提供すべき契約（未完）

```cpp
template<class Traits, class Derived, bool IsBottom>
struct NodeCRTP : Traits::Node {
  // this->children / this->count / this->size を利用
  // self() は Derived& を返す
};

template<class Traits>
struct UpperNode
    : NodeCRTP<Traits, typename Traits::UpperNode, false> { /* ... */ };

template<class Traits>
struct BottomNode
    : NodeCRTP<Traits, typename Traits::BottomNode, true> { /* ... */ };
```

- `Traits::NodeBase`、`Traits::Node`、`Traits::Leaf`、
  `Traits::UpperNode`、`Traits::BottomNode` を定義する。
- UpperNode は `level >= 1`、BottomNode は `level == 0`。
- NodeCRTP の `id()` は必ず `Pools::id(self())` を使う。
  Bottom の生 Pool index を親・子に直接保存するとタグ情報が落ちる。
- `Leaf::get_parent()` はタグを除去する `Pools::get<Bottom>()` を使う。
- `Leaf::size` および `Leaf::count` は `static constexpr` の 1 とする。
- `NodeCRTP::erase()` は Bottom なら `recycle<0>(leaf_id)`、
  Upper ならタグから Bottom/Upper を識別して `recycle(id)` を呼ぶ。
- `child_size()` / `child_count()` は Upper の `level == 1` なら Bottom 子、
  `level > 1` なら Upper 子を参照する。
- `locate()` は Bottom に達したら Leaf ID を直接返す。
- `NodeCRTP` は現在補完用に非テンプレート化されているので、
  最終的に上記のテンプレート版へ戻し、Set/Aggregate の派生ノードを実体化する。

## 要検証

- `linux-tests.yml` の全テストを GCC 15.2 / C++23 で実行すること。
- 特に split/merge、reverse を挟んだ iterator、
  Lazy の部分区間 apply、Set の最小キーキャッシュを重点確認。
- 以前の別 Pool 実験ではタグ付き ID がホットパスを遅くしたケースがある。
  メモリおよび A/B benchmark を計測し、必要ならタグの境界を減らす。
- 現在の `sizes[]` / `counts[]` はキャッシュ更新経路が未確定。
  正しさが取れるまでは探索に利用しない。

この PR は前提となる Node 本体の作業中につき **マージ不可**。
