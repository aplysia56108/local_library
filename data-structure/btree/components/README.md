# B+ tree共通部品

`Bplustree::Base<Traits, IsReversible = false>` は位置で操作する木の基盤です。
node・iteratorの実装を再利用し、rootのみを所有します。sizeとlevelはrootの
metadataを参照します。既存の公開 `Bplustree_Vector` / Set / Segtreeはまだ
このBaseへ移行していません。

```cpp
using Sequence = Bplustree::Base<Bplustree::Traits<long long, 32>, true>;
Sequence sequence{1, 2, 3};
sequence(1) += 4;
sequence.reverse(0, 3);
```

`IsReversible == false` の型にはreverse APIがありません。
Baseのiteratorと添字参照は常に読み取り専用です。集約値やソート順を壊す
直接代入を防ぎ、更新はsetまたはsetterを通します。Base自体は位置指定の
insert/setを公開するので、setを作る場合はprotected継承して、安全なAPIだけを
usingで公開してください。比較器やキーの重複規則は派生クラスが管理します。

## Traitsと継承

Traitsは `T`, `kNodeCapacity`, `Node`, `Leaf`, `Internal` を持ちます。
基本形は `Bplustree::Traits<T, Capacity>`、拡張形は次の構造です。

```cpp
struct MyTraits {
  using T = long long;
  static constexpr std::size_t kNodeCapacity = 32;
  struct Node;
  struct Leaf;
  struct Internal;
};
struct MyTraits::Node : Bplustree::Node<MyTraits> {
  using Bplustree::Node<MyTraits>::Node;
  long long aggregate = 0;
};
struct MyTraits::Leaf : Bplustree::Leaf<MyTraits> {
  void after_sync(); // values[0,count)から集約値を更新
};
struct MyTraits::Internal : Bplustree::Internal<MyTraits> {
  void after_sync(); // children[0,count)の集約値から更新
};
```

派生コンテナはprotectedのroot_、Node/Leaf/Internal型、materialize、refresh、
refresh_path、build、split_node、join_nodesを利用できます。集約値を持つ
ノードではafter_syncを実装します。split/insert/erase/set/build/mergeの各経路で
派生Leaf/Internalのフックが呼ばれます。通常状態はcount < NodeCapで、
insertがtrueを返した場合は直ちにsplitします。

BaseのsplitはBase型を返します。派生型はBaseのムーブコンストラクタへ渡す
引数を受けるコンストラクタを用意すると、追加の全要素コピーなしで結果を
引き継げます。状態を持つ比較器などは派生型側で引き継ぎを管理してください。

## 遅延更新と反転のフック

push_reverseはフラグを付け、apply_reverseが並び替えと子へのフラグ伝播を行います。
基本Leaf/Internalは反転を実装済みです。遅延作用は派生ノードのapply_lazyなどで
実装してください。Baseは構造変更前に対象経路の反転・遅延作用を実体化します。

遅延タグを付けるときは、そのノードの集約値を直ちに論理上の値へ更新してください。
非可換モノイドでの反転では順方向・逆方向の集約値を保持し、派生push_reverseで
基本フラグを反転するとともに両集約値を交換します。after_syncは残っている
遅延タグも考慮し、子の論理上の集約値から計算してください。

## Setter

`Bplustree::Setter<Tree, Value>` はコピー・ムーブ不可の一時proxyです。
代入と複合代入はget/set、prod系はprod_right/prod_left、apply系は
apply/apply_leftへ委譲します。各操作はTreeが対応する場合だけ利用可能です。
applyの作用型はValueと異なっていても構いません。

派生クラスはoperator()を次のように定義して、派生先の更新処理へ委譲します。

```cpp
Bplustree::Setter<MyTree, value_type> operator()(std::size_t index) {
  assert(index < size());
  return {*this, index};
}
```

setterをさらに継承する場合は、コンストラクタとoperator=をusingで引き継ぎます。
既存ライブラリのglobalなSegtreeSetterとの衝突を避け、新部品は名前空間内で
定義しています。

## 計算量と検証

位置探索・点更新・挿入削除・split/mergeは通常 O(NodeCap × 木の高さ)、
全体反転はO(1)、部分反転はsplit/mergeを使います。一括構築はO(N)、
resizeによる追加は点挿入を繰り返します。連続した削除で疎なノードが残るため、
メモリを詰め直す場合はrebuildを使います。反転などの構造変更後はiteratorを
取り直してください。例外を投げる要素型に対する強い例外保証は提供しません。

test/にはvectorとの操作列比較、Setの安全なAPI公開例、加算・区間和の遅延拡張、
文字列連結の非可換集約と反転、setterの作用型・制約の回帰テストがあります。
