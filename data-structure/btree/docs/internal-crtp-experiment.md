# Internal / BottomInternal の CRTP ディスパッチ実験

ベース: `experiment/bplustree-internal-constexpr-dispatch`（#49）

## 実際のクラス構成

```text
                  Traits::Node
                      |
               Internal<Traits>         ← 共通の構造・旧API
                      |
               Traits::Internal         ← Set/Aggregate の metadata
                  /       \
                 /         \
        UpperInternal    BottomInternal       ← 2つの具体型（兄弟）
               |               |
        InternalCRTP       InternalCRTP
        <...,false>        <...,true>          ← 型に応じた静的処理
               |               |
        ObjectPool<Upper> ObjectPool<Bottom>   ← 別プール
```

`Internal<Traits>` は旧API用の基底で、**実際に生成するオブジェクトは
`UpperInternal<Traits>` または `BottomInternal<Traits>`** になる。
別の `ObjectPool` を持ち、共通の `Traits::Internal` を通して
Set/Aggregate のメタデータを引き続き利用できる。

## CRTP の役割

```cpp
template <class Traits, class Derived, bool IsBottom>
struct InternalCRTP {
  void refresh() {
    static_cast<Derived&>(*this).template refresh_typed<IsBottom>();
  }
  // move_child, locate, split, erase なども同様
};

template <class Traits>
struct UpperInternal : Traits::Internal,
    InternalCRTP<Traits, UpperInternal<Traits>, false> {
  using UpperOps = InternalCRTP<Traits, UpperInternal<Traits>, false>;
  using UpperOps::refresh;
  // ...
};

template <class Traits>
struct BottomInternal : Traits::Internal,
    InternalCRTP<Traits, BottomInternal<Traits>, true> {
  using BottomOps = InternalCRTP<Traits, BottomInternal<Traits>, true>;
  using BottomOps::refresh;
  // ...
};
```

具体型 `BottomInternal&` から呼べば `refresh_typed<true>()` が使われ、
子を Leaf として扱う処理がコンパイル時に確定する。
`UpperInternal&` では `refresh_typed<false>()` が呼ばれる。

## 既存 API との接続

`NodePools::internals()[id]` は共通の `Traits::Internal&` を返す
互換ビューとして残す。ここで `level` による分岐が必要になる。
`NodePools::with_internal(id, callback)` では ID のタグで一度だけ振り分け、
具体型 `UpperInternal&` または `BottomInternal&` を渡す。

現在の `InternalCRTP` は既存の `*_typed<IsBottom>` に委譲する薄い層。
処理本体の完全な移設や、旧 `Internal&` 経路の削除は今後の課題。
したがってこの実験段階で性能改善が保証されるわけではない。

## テストと注意

`data-structure/btree/test/bplustree-internal-crtp-test.cpp` が
兄弟型、プール選択、動的・静的な操作経路を確認する。
PR #53 は #49 をベースとする積み重ね PR であり、
`linux-tests.yml` の pull_request トリガーは `main` 限定なので
この状態では Actions が自動起動しない。Actions の `Run workflow` で
`experiment/bplustree-internal-crtp-dispatch` を選んで検証する。

バグや性能悪化を許容する試作であり、main への直接マージは推奨しない。
