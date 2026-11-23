# B+tree ライブラリ（共通部品と公開 API）

このディレクトリの `Base<Traits>` / `Iterator` / `Setter` / `NodePools` / `Internal` は、
位置によって操作する B+tree の共通実装です。公開コンテナは親ディレクトリの
`Bplustree_Vector`、`Bplustree_Set` / `Bplustree_MultiSet`、
`Bplustree_Segtree`、`Bplustree_LazySegtree` です。

## データ構造と所有権

- **1要素1Leaf**：Leaf は実要素1個を持ち、内部ノード（Internal）が子ID・部分木サイズ・必要な集約情報を保持します。旧 Vector の複数要素 Leaf は使用しません。
- **Pool**：同じ Traits を使う木は `NodePools<Traits>` の Leaf / Internal Pool を共有します。両者の ID 空間は別です。`id_type` は `std::uint32_t`、`position` / `level` は `std::uint8_t`。基本 Traits の NodeCapacity は 3～255 を想定します。
- **所有者**：各 Base は `root_` が指すノード木を所有し、さらに **木ごとに固有な2個の番兵 Leaf ID**（`bbegin_` / `end_`）を保持します。`size()` は番兵2個を除いた実要素数です。
- **固定番兵**：`bbegin()` は先頭要素の直前、`end()` は末尾の直後。`swap`、`reverse`、`split`、`merge` でも各オブジェクトの番兵 ID は変わりません。内部では番兵の **位置** を交換して整合させます。
- **安定 ID**：実要素の Leaf ID は木の構造変更で移動せず、iterator はその ID を保持します。`split` / `merge` / `swap` で要素が別の木へ移った場合も、その要素を指す iterator は同じ ID を追跡します。
- **失効**：要素自身を削除するとその iterator は無効です。`clear` / `assign` / `rebuild` などで作り直した旧要素の iterator も無効になります。Pool は消去済み ID を再利用するため、失効後に使ってはいけません。

Pool は `inline static constinit ObjectPool` で管理し、関数内 static の初期化ガードを避けています。
Pool 内部の `std::vector` 拡張で **Leaf / Internal への C++ 参照・ポインタ** は無効になり得ます。
一方で、まだ解放されていない Leaf / Internal の **ID** は変わりません。
ノード参照を保持したまま追加確保をする処理は、再取得または事前確保が必要です。

## 主な公開 API

### Vector（位置指定の動的配列）

```cpp
Bplustree_Vector<long long, 32> v{1, 2, 3};
v(1) += 4;              // 1番目を 6 に更新
v.reverse();            // 全要素を反転
auto suffix = v.split(1);
v.merge(suffix);        // suffix は空になる
```

`get(i)`、`operator[](i)`、iterator の参照は **読み取り専用** です。
更新には `set(i, value)`、`operator()(i)` の Setter、`insert`、`erase` を使用します。
`split(index)` / `split(iterator)` は index / iterator 以降を新しい木として返し、
`merge(right)` は right の全要素を末尾につなぎ、right を空にします。
`reverse(first, last)` は半開区間を反転します。全体 `reverse()` はフラグと番兵位置の交換で O(1) です。

### Set / Multiset（比較器順序の集合）

```cpp
BSet<long long> s{ };        // 必要なキーは insert で追加
s.insert(3);
s.insert(7);
s.insert(10);
auto ge = s.lower_bound(7);
auto right = s.split_by_key(7);  // s={3}, right={7,10}
s.merge(right);                 // s={3,7,10}, right={}
```

`Bplustree_Set<Key, Type, Compare, NodeCapacity>` は protected 継承で位置指定の書換え・
反転を公開せず、iterator も const です。`SetType::Set` は等価キーを重複排除し、
`SetType::MultiSet` は重複を保持します。順序は `Compare` が定義する厳密弱順序です。

- `lower_bound(key)` / `upper_bound(key)`、`find`、`contains`、`count` を提供。`max_lt` / `max_le` で該当キーがなければ `bbegin()` を返し、`min_ge` / `min_gt` でなければ `end()` を返します。
- `split(iterator)`：その位置以降、`split_by_index(k)`：最初の k 個より後、`split_by_key(key)`：比較器順で key 以上（等価キーを含む）を右側に分離します。右側には比較器を **コピー** するため `Compare` はコピー構築可能である必要があります。
- **`merge(right)` は `std::set::merge` と異なる「順序付き連結」** です。左右が同じ順序関係を持ち、Set では `compare_(max(left), min(right))`、Multiset では `!compare_(min(right), max(left))` を満たすことが前提です。right は空になります。要素ごとの再検索・重複解決は行いません。
- Set の Internal は最小キーの参照ではなく **最左 Leaf ID `min_leaf_id`** をキャッシュします。構造変更時の `refresh_metadata()` と、番兵位置交換時の `refresh_on_leaf_swap` フックでキャッシュを保ちます。
- 通常のコピーは要素をコピーします。ムーブ構築は空のオブジェクトとの交換で、元の木を空にします。**ムーブ代入は Base の swap ベース**なので、移動元には代入先の旧要素が残ります。比較器の移動後の状態については、その比較器型の要件に従います。

異なる状態の比較器（順序関係が異なるもの）を持つ Set を `merge` してはいけません。
境界の値だけでは比較器全体の整合性を証明できないため、呼び出し側が保証します。

### Segtree / LazySegtree（集約・遅延更新）

```cpp
SumSegtree seg{1, 2, 3};
auto total = seg.prod(0, 3);  // [0,3) のモノイド積
seg(1).prod_right(5);        // a[1] = op(a[1], 5)
seg.reverse(0, 3);
auto back = seg.reverse_prod(0, 3);

AddSumLazySegtree lazy{1, 2, 3};
lazy.apply(0, 2, 10);        // [0,2) に作用
lazy(2).apply(4);            // 1点に作用
lazy.all_apply(1);
```

`AggregateTree` は順方向・逆方向の区間積を Internal に保持し、Leaf 自身には集約値を持ちません。
`prod(l,r)` / `reverse_prod(l,r)` は半開区間の積、`all_prod()` / `all_reverse_prod()` は全体積です。
`max_right(l, pred)` と `min_left(r, pred)` は境界探索です。
`right_bound` / `left_bound` はその別名です。
単位元だけを渡したときの `pred(e()) == true` は要求しません。実際に判定する非空区間について、区間を広げたときに
`true` から `false` へのみ遷移する述語を使ってください。

LazySegtree の `Action::mapping(f, x, size)` は長さ size の区間積への作用、
`Action::composition(f, g)` は **先に g、次に f** を適用する合成です。
Leaf では未伝搬作用を持たず、作用が届いた時点で実要素を書き換えます。
Internal は `pending` と積を管理します。作用の単位元や等価比較の実装は必須ではありません。
反転後の区間積も正しくなるよう mapping が定義されている必要があります。
`ReverseInvariant = true` は、区間積が要素の並び順の反転に対して不変である場合だけ指定してください。

## Setter と iterator

`Bplustree::Setter<Tree>` は **Iterator を保持する点更新 Proxy** です。
`operator=(value)`、`+=` などの複合代入、前置 `++` / `--` は要素を更新して Setter を返します。
後置 `++` / `--` は **更新前の値** を返します。
`val()` は現在値を値として返し、`prod(value)` / `prod_right(value)` / `prod_left(value)` は
モノイド演算を右・左から適用する **void の更新操作** です。
Action に対応する木では `.apply(f)` が1要素への作用を行います。
Setter の演算子・メソッドは、要素型・Monoid・Action が対応する場合だけ利用できます。

iterator は Leaf Pool ID によって要素を識別し、コンテナへのポインタは持ちません。
C++20 の `std::random_access_iterator` コンセプトを満たしますが、**任意距離移動や差分が O(1) とは限りません**。
`++` / `--` は同じ最下層 Internal 内であれば直接 Leaf ID を移動できます（Lazy なしの場合）。
異なる部分木へ移るときは祖先をたどり、必要な反転・遅延情報を処理します。
**別々の木に所属する iterator 同士の引き算・順序比較は行わないでください**。

### position / value epoch

`IteratorEpoch<Traits>` の `position` と `value` は Traits 単位で共有されます。
`reverse()` は position だけ、区間 `apply()` は value だけを失効させます。
構造操作（`insert`、`erase`、`split`、`merge`、`swap`、`assign` 等）は両方を失効させます。
iterator の参照や移動時に世代が合わなければ経路を再検証し、
未伝搬の反転（position）と作用（value）を必要に応じて実体化します。

`Traits::lazy = false`（未指定時も false）なら、値 epoch の追跡・確認を `if constexpr` で除外します。
遅延作用を実装するカスタム Traits は `static constexpr bool lazy = true;` を明示してください。
世代は **キャッシュの妥当性** の管理であり、iterator の等値性は Leaf ID で判断します。
同じ Traits の無関係な木の更新でも、保守的にキャッシュが失効する場合があります。

## 独自 Traits と内部実装

Traits は `T`、`kNodeCapacity`、`Node` / `Leaf` / `Internal` を定義します。
`lazy` の未指定は false として扱い、遅延作用を実装する拡張では `lazy = true` を明示します。反転により特殊なメタデータ更新が必要な場合は派生ノードの
`push_reverse_impl()`、メタデータ再計算には `refresh_metadata()` を使います。
**旧 `IsReversible` / `IsMergable` フラグは現在のインターフェースでは使いません。**

```cpp
struct MyTraits {
  using T = long long;
  static constexpr std::size_t kNodeCapacity = 32;
  static constexpr bool lazy = false;
  using Node = Bplustree::Node<MyTraits>;
  using Leaf = Bplustree::Leaf<MyTraits>;
  struct Internal;
};
struct MyTraits::Internal : Bplustree::Internal<MyTraits> {
  using Bplustree::Internal<MyTraits>::Internal;
  long long aggregate = 0;
  void refresh_metadata();  // 子の値・集約を読み取って再計算
};
```

`Internal::move_child<FromOther, ConsiderOtherSync>` は子ID・count・size・親ID・親内位置を
差分更新します。`refresh()` は派生側 `refresh_metadata()` の再計算だけを行い、
祖先へのサイズ差分は構造操作側が伝搬します。旧 `sync()` はありません。

- `move_child<false>`：同一親内の位置更新。size を変更しません。
- `move_child<true, false>`：新しい子や別木の根などを追加し、移動元の count/size は触りません。
- `move_child<true, true>`：移動元の suffix を切り離しながら追加します。移動元の末尾側から処理します。

`children[0, count)` が論理的な子の範囲で、それ以降の内容は未規定です。
ノード分割・統合の途中では一時的に穴ができるため、その途中で検索・積計算をしてはいけません。
`push_reverse()` はフラグの反転と派生 hook、`apply_reverse()` は実際の順序反転と子への伝搬、
`materialize()` は反転と遅延作用の伝搬を行います。
`NodePools::recycle()` は指定部分木を走査して Pool の ID を再利用可能にします。

## 計算量・検証方針

NodeCapacity を B、木の高さを h（通常 O(log_B N)）とした目安です。

| 操作 | 計算量（概ね） |
| --- | --- |
| `size`、`empty`、`all_prod`、`all_reverse_prod` | O(1) |
| 全体 `reverse()`（Vector / Segtree / LazySegtree） | O(1) |
| `get`、`nth`、`set`、位置指定 `insert` / `erase` | O(Bh) |
| `split` / 順序付き `merge`、部分 `reverse` | O(Bh) |
| `prod`、`reverse_prod`、区間 `apply`、境界探索 | O(Bh)（演算・述語 O(1) を仮定） |
| `assign` / 一括構築、`rebuild` | O(N)（入力順が確定している場合） |
| Set / Multiset の範囲 `assign` | ソート済み O(N)、その他 O(N log N) |

`resize` の増加は単点挿入を繰り返します（空の木に対する一括 `assign` の場合を除く）。
`clear` / `assign` は旧要素の再利用可能 ID を回収します。
現在の実装は一般的な例外安全性の強い保証を目的としていません。

軽量な機能・回帰テストは `data-structure/btree/components/test/` および
`data-structure/btree/test/` に置きます。重い乱択・ストレス・性能比較は
`verify/` またはベンチマーク側で実施します。

ライブラリを変更するときは **通常の操作列だけでなく、保持済み iterator、固定番兵、
split / merge 後の木の再利用、Set の min_leaf_id、反転と Lazy の混在** も確認してください。
