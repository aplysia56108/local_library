# B+tree 系コンテナ

1要素1Leaf・Pool ID 管理による B+tree 系の公開コンテナをまとめています。
**内部設計・不変条件・計算量・iterator の仕様は [components/README.md](components/README.md) を参照してください。**

| ヘッダ | 主な型 | 用途 |
| --- | --- | --- |
| [bplustree-vector.hpp](bplustree-vector.hpp) | `Bplustree_Vector<T, Capacity>` | 位置指定の動的配列。挿入・削除・反転・split / merge |
| [bplustree-set.hpp](bplustree-set.hpp) | `Bplustree_Set<Key, Type, Compare, Capacity>`、`BSet`、`BMultiSet` | キー順序の集合・重複許容集合・順序付き split / merge |
| [bplustree-segtree.hpp](bplustree-segtree.hpp) | `Bplustree_Segtree<Monoid, Capacity>`、`BSegtree` | 動的区間積、境界探索、反転・split / merge |
| [bplustree-lazysegtree.hpp](bplustree-lazysegtree.hpp) | `Bplustree_LazySegtree<Monoid, Action, R, Capacity>`、`BLazySegtree` | 区間作用・区間積・反転・split / merge |

## 設計上の重要事項

- 各オブジェクトは **固定された前後番兵 Leaf ID** を持ちます。反転・分割・結合・swap でも番兵の ID は変わりません。
- 実要素の iterator は Leaf ID を保持するため、要素が削除されない限り構造変更後も同じ要素を追跡します。ただし `assign` / `clear` / `rebuild` は旧要素 ID を回収します。
- iterator は `std::random_access_iterator` コンセプトに対応しますが、任意距離移動・差分が O(1) という意味ではありません。異なる木の iterator 間の差分・順序比較は行わないでください。
- Set の `merge` は **順序付き連結**で、`std::set::merge` のような重複判定・再挿入はしません。比較器の順序が互換であることと、左右の境界条件を呼び出し側が保証します。
- Segtree の値変更は `set(index,value)` または `operator()(index)` の Setter を使い、iterator・添字参照から直接は変更しません。

## 簡単な使用例

```cpp
#include "data-structure/btree/bplustree-vector.hpp"
#include "data-structure/btree/bplustree-set.hpp"
#include "data-structure/btree/bplustree-segtree.hpp"

Bplustree_Vector<long long> seq{1, 2, 3};
seq(1) += 5;
auto tail = seq.split(2);
seq.merge(tail);

BSet<long long> keys;
keys.insert(10);
keys.insert(20);
auto ge20 = keys.split_by_key(20);
keys.merge(ge20);

SumSegtree sums{1, 2, 3};
auto x = sums.prod(0, 3);
```

## テスト・検証

軽量な単体テストは [test/](test/) と [components/test/](components/test/)、
問題別の検証は [verify/](verify/) を参照してください。
性能比較は専用ベンチマークで実施します。
