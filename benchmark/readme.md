# B+tree Performance Benchmarks

B+tree の代表的な処理を固定 seed の入力で実行し、**処理時間と結果の正しさ**を調べる。通常の test / verify とは分離し、GitHub Actions の `Performance benchmarks` から手動でのみ実行する。

## 構成

`benchmark/btree/<種類>/<操作>/` に操作ごとのファイルを配置する。共通ロジックは `generator_common.hpp` と `benchmark_common.hpp` にまとめ、`generator.cpp` / `benchmark.cpp` はマクロを選択する薄いラッパーとする。

| 種類 | 第1段階 | 第2段階 |
| --- | --- | --- |
| set | insert / erase / access | lower_bound / iterate |
| vector | insert / erase / reverse / access | split_merge / range_erase / iterate |
| segtree | prod / insert / erase / reverse / access | split_merge / range_erase / max_right / min_left / iterate |
| lazysegtree | prod / apply / insert / erase / reverse / access | split_merge / range_erase / max_right / min_left / iterate |

**合計33操作 × 4Cap（8/16/32/64）=132ケース**。各5回測定では660回。重いので、開発時は `--target` と `--caps` で対象を絞ることを推奨する。

## フィクスチャと計測範囲

- `generator.cpp` は **B+tree に依存せず**、`std::vector` / Fenwick / `std::lower_bound` / prefix sum などを用いて `in.txt` / `out.txt` を生成する。
- 通常は `in.txt` / `out.txt` をコミットする。第2段階の `range_erase` 3ケースは入力が大きいため **2ファイルをコミットせず、実行時に生成する**。`run.py` は両方なければジェネレータを1回コンパイル・実行して `out/benchmark/generated-fixtures/` に保存し、同一ジョブ内の各Capで使い回す。片方だけ存在する場合はエラー。
- すべてのケースで、**毎回バイト単位で期待出力を照合**。不一致・異常終了・タイムアウトは失敗。
- 入力読み込み・初期構築・ジェネレータ実行・最終結果の出力・コンパイルは計測外。操作を `std::chrono::steady_clock` で測定し、標準エラーへ `BENCHMARK_NS=...` を出力する。
- 出力内容は操作ごとのクエリ結果、または全要素の最終状態（`iterate` は走査結果）。結果は計測中に事前確保した配列へ保存し、計測後に出力する。
- 比較の同一性には CPU、コンパイラ、フラグ、入力と正解の SHA256 を用いる。生成したフィクスチャも同じ方法で識別する。

## 各ベンチマークの入力

各操作は原則10万クエリを使用する。

**第1段階**
- `insert`: 空から10万要素を挿入。 `erase`: 20万要素から10万回削除。 `access`: 10万要素へ `operator[]` でアクセス。
- `reverse`: 半開区間の反転。ほぼ32要素以下、1000操作に1回長い区間。
- `prod`: 非可換 `AffineMonoid<modint998244353>`（Segtree）、区間和 `AffineSumLazySegtree<modint998244353>`（LazySegtree）。
- `apply`: 区間Affine作用。操作後の全要素を検証。

**第2段階**
- `split_merge`: 10万要素に対して、位置 `0`、`N`、`1`、`N-1`、ランダム位置を織り交ぜて `split(index)` → `merge(right)` を10万回。元の順序が完全に復元されることを検証。
- `range_erase`: 50万要素から、長さ1〜6のランダムな半開区間を10万回削除。残存要素を全列挙して検証。独立したFenwickで削除位置を追跡する。
- `lower_bound`: Setの10万要素へ、存在するキー・存在しないキー・範囲外のキーを10万回検索。見つかった値、存在しなければ `-1` を検証。
- `max_right` / `min_left`: SegtreeとLazySegtreeに **非負整数の SumMonoid** を用い、`sum <= limit` という単調述語で10万回境界探索。独立したprefix sumと二分探索で正解を生成。`prod` の非可換Affineとは意図的に別のモノイドを使う。
- `iterate`: 10万要素を `begin()`〜`end()` で全走査し、取得した値を順番どおりに事前確保した配列へ保存。 `access`（`operator[]`）とは分けて測定。

Setでは `split / merge` と `erase(first,last)` を公開していないため、これらは対象外とする。

## 実行方法

```bash
# 第2段階の1操作だけ、2つのCapを計測
python3 benchmark/run.py --target btree/vector/split_merge --caps 8,16 --runs 5

# 境界探索をまとめて計測
python3 benchmark/run.py --target btree/segtree/max_right --caps all --runs 5

# 大きな range_erase のフィクスチャを自動生成して計測
python3 benchmark/run.py --target btree/lazysegtree/range_erase --caps 8 --runs 1
```

`--target` は `all`、種類、操作ディレクトリのprefixを指定できる。`--caps` は `all` または `8,16,32,64` のカンマ区切り（個別指定可）。GCC 15.2 / C++23 / `-O2` を想定している。

コミット済みフィクスチャの再生成は、該当ディレクトリで以下のように行う。

```bash
g++ -std=c++23 -O2 generator.cpp -o /tmp/benchmark-generator
/tmp/benchmark-generator
```

## GitHub Actions と記録

Actions → Performance benchmarks → Run workflow から対象ブランチ・種類/操作・Capを選択する。5回の**中央値**で比較し、最速記録比120%超で🟡、150%超で🔴（遅くなっても警告のみ、結果の不一致は失敗）。

**main の成功実行だけ** `benchmark-records` ブランチの `best.json` を更新する。PRブランチでは読み取り・比較のみ。既存の `btree/vector/insert`（Cap8固定）の旧記録は残し、新方式の `btree/vector/insert/cap8` と区別する。

GitHub-hosted runnerのCPUモデルやホスト負荷は変化するため、最速比は参考指標。計測ログはArtifactに14日保存する。
