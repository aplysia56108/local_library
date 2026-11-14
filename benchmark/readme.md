# B+tree Performance Benchmarks

代表的な操作を固定入力で10万回実行し、**処理時間を測定しながら出力の正しさを確認**する。通常の test / verify とは分離し、GitHub Actions の Performance benchmarks から手動でのみ実行する。

## ディレクトリ構成

`benchmark/btree/<種類>/<操作>/` ごとに4ファイルを置く。

```text
benchmark/
├── readme.md
├── run.py
└── btree/
    ├── generator_common.hpp    # 独立した参照実装・入力生成
    ├── benchmark_common.hpp    # B+tree実行・計測
    ├── set/
    │   ├── insert/
    │   ├── erase/
    │   └── access/
    ├── vector/
    │   ├── insert/
    │   ├── erase/
    │   ├── reverse/
    │   └── access/
    ├── segtree/
    │   ├── prod/
    │   ├── insert/
    │   ├── erase/
    │   ├── reverse/
    │   └── access/
    └── lazysegtree/
        ├── prod/
        ├── apply/
        ├── insert/
        ├── erase/
        ├── reverse/
        └── access/
```

各操作のディレクトリには **`generator.cpp` / `in.txt` / `out.txt` / `benchmark.cpp`** を置く。新規ケースの generator.cpp と benchmark.cpp は共通ヘッダの薄いラッパーで、入力・出力は各ケースにGit管理している。既存の `vector/insert` は旧形式のまま互換性を維持する。

- `generator.cpp`：固定seedから `in.txt` を生成し、独立した参照実装で `out.txt` を作る。時間がかかってもよい。B+tree本体は一切使用しない。
- `benchmark.cpp`：標準入力からクエリを読み、対象処理だけを `std::chrono::steady_clock` で測る。実行後の全要素または各prod結果を標準出力へ、`BENCHMARK_NS=...` を標準エラーへ出力する。
- `in.txt` と `out.txt`：すべてのCapで共通。ベンチマーク実行時は**毎回バイト単位で照合**し、不一致・異常終了・タイムアウトは失敗とする。

## 計測条件

- Capは **8、16、32、64**。`-DNODE_CAP=<Cap>` で**コンパイル時**に切り替える（ソースやフィクスチャの複製はしない）。
- 各操作10万回。`access` は10万要素の全インデックスに `operator[]` でアクセスする（要素の全列挙と区別する）。
- `insert` は空から10万要素へ。`erase` は20万要素から10万回削除して10万要素を残し、**計測外で全列挙**する。
- `reverse` は半開区間の**反転**（rotateではない）。区間はほとんどが長さ32以下、1000操作ごとにより長い区間も混ぜる。
- `prod` は10万要素に対して10万回。Segtreeでは `AffineMonoid<modint998244353>`（非可換な一次関数合成）、LazySegtreeでは `AffineSumLazySegtree<modint998244353>`（区間和＋一次関数作用）を用いる。
- `apply` は区間Affine作用を10万回行い、計測外で全列挙する。
- 初期構築、入力読み込み、最終出力、正解の生成、コンパイルは計測外。`prod` / `access` では戻り値を事前に確保した配列へ保存して計測後に出力する。
- generatorの正解は `std::vector`、独立したFenwickによる順位選択、単純区間走査、prefix sumなどで計算する。`set` は重複なしのキーを使う。

Cap別のベンチマークを1ケースとし、合計 **18操作 × 4Cap = 72ケース**、各ケース5回で **360回の計測**となる。

## 入力再生成

```bash
# 例: 新しいcase
cd benchmark/btree/lazysegtree/apply
g++ -std=c++23 -O2 generator.cpp -o /tmp/benchmark-generator
/tmp/benchmark-generator
# in.txt / out.txt の変更をチェックしてからコミット
```

generatorは実行中のディレクトリに `in.txt` と `out.txt` を書き出す。フィクスチャ変更時は新しいハッシュを持つ**別の最速記録**として扱う。

## ローカルで計測

```bash
# rootからすべてのCapを計測
python3 benchmark/run.py --target btree/vector/erase --caps all --runs 5

# 機能全体・指定Capだけ
python3 benchmark/run.py --target btree/segtree --caps 16,32 --runs 5
```

`--target` は `all` / `btree/<種類>` / `btree/<種類>/<操作>` を指定可能。`--caps` は `all` または `8,16,32,64` のカンマ区切り（個別指定可）。コンパイルはC++23, GCC `-O2`。実行可能ファイルやJSONは `out/benchmark/` に保存する。

## GitHub Actions と記録

Actions → Performance benchmarks → Run workflow を使って対象ブランチ・ライブラリ/操作・Capを選択する。5回の**中央値**で比較し、120%超で🟡、150%超で🔴とする（遅くなっても警告のみ、出力不一致は失敗）。

**mainの成功実行だけ** `benchmark-records` ブランチの `best.json` を更新する。開発ブランチは読み取り・比較のみ。フィクスチャ・Cap・CPUモデル・コンパイラ・フラグが同じ結果だけを比較する。過去の `btree/vector/insert`（Cap8固定）の旧記録は残すが、新方式の `btree/vector/insert/cap8` 以降とは区別する。

GitHub-hosted runnerではCPUモデルだけでなくホスト負荷も変化するので、**最速比は参考指標**とする。計測ログはArtifactに14日保存する。
