# Internal / BottomInternal の CRTP ディスパッチ実験

ベース: `experiment/bplustree-internal-constexpr-dispatch`（#49）

## 現在のクラス構成（試作）

```text
Node<Traits>
    |
Internal<Traits> ─── InternalCRTP<Traits, Internal<Traits>, false>
    |
Traits::Internal (Set/Aggregate の metadata)
    |
BottomInternal<Traits> ─── InternalCRTP<Traits, BottomInternal<Traits>, true>
                              + links / ports_swapped
```

実際には二つの `InternalCRTP` は空のミックスインであり、ノード実体のデータを保持しない。既存の
`Traits::Internal` を継承しているのは `NodePools::internals()[id]` が引き続き
`Traits::Internal&` を返せるようにするため。上層と最下層は従来どおり別 Pool に格納する。

```cpp
template <class Traits, class Derived, bool IsBottom>
struct InternalCRTP {
  void refresh() {
    static_cast<Derived&>(*this).template refresh_typed<IsBottom>();
  }
};

// 上層: 子は Internal
struct Internal : InternalCRTP<Traits, Internal, false> { /* ... */ };

// 最下層: 子は Leaf（型がわかっている）
struct BottomInternal : Traits::Internal,
                        InternalCRTP<Traits, BottomInternal, true> {
  using BottomOps = InternalCRTP<Traits, BottomInternal, true>;
  using BottomOps::refresh;
};
```

## 呼び出し経路

- 具体型 `BottomInternal&` から `refresh()` を呼ぶと
  `InternalCRTP<..., true>::refresh()` → `refresh_typed<true>()` となり、
  子が Leaf である分岐がコンパイル時に確定する。
- 具体型 `Internal&` の上層ノードでは `InternalCRTP<..., false>` を経由する。
- 従来の `Traits::Internal&` 互換 API で最下層が来た場合だけ
  `level == 0` をチェックして `bottom().refresh()` に委譲する。
- `NodePools::with_internal(id, callback)` は具体型を持つ参照でコールバックするため、
  互換 API のチェックを省ける。

## 現時点の制約

これは**完全な兄弟型への分離ではない**。#49 の `*_typed<IsBottom>` 実装を CRTP が
呼び出すところまでが今回の実験で、処理本体を `InternalCRTP` に移動したわけではない。

本来の兄弟型へ進めるには、Set/Aggregate の共通 metadata を切り出すとともに
`NodePools::internals()` が `Traits::Internal&` を返す互換 API を廃止・置換する必要がある。
この段階では速度・メモリ節約を保証せず、main へマージしない。

## 動作確認

`data-structure/btree/test/bplustree-internal-crtp-test.cpp` は
上層/最下層の静的型、ノード操作、従来 `Internal&` 経由の反転・探索を検証する。
`linux-tests.yml` は PR のベースが `main` の場合にのみ起動するため、
この積み重ね PR（ベース #49）では GitHub Actions を手動実行する。
