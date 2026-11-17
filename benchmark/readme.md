# B+tree Performance Benchmarks

B+tree の代表的な処理を固定 seed の入力で実行し、**処理時間と結果の正しさ**を調べる。通常の test / verify とは分離し、GitHub Actions の `Performance benchmarks` から手動でのみ実行する。

## 構成

`benchmark/btree/<種類>/<操作>/` に操作ごとのファイルを配置する。共通ロジックは `generator_common.hpp` と `benchmark_common.hpp` にまとめ、`generator.cpp` / `benchmark.cpp` はマクロを選択する薄いラッパーとする。

| 種類 | 第1段階 | 第2段階 | 第3段階 |
| --- | --- | --- | --- |
| set | insert / erase / access | lower_bound / iterate | mixed_uniform / mixed_edges / mixed_hotspot / mixed_growth / mixed_shrink |
| vector | insert / erase / reverse / access | split_merge / range_erase / iterate | mixed_uniform / mixed_edges / mixed_hotspot / mixed_growth / mixed_shrink |
| segtree | prod / insert / erase / reverse / access | split_merge / range_erase / max_right / min_left / iterate | mixed_uniform / mixed_edges / mixed_hotspot / mixed_growth / mixed_shrink |
| lazysegtree | prod / apply / insert / erase / reverse / access | split_merge / range_erase / max_right / min_left / iterate | mixed_uniform / mixed_edges / mixed_hotspot / mixed_growth / mixed_shrink |

**合計53ワークロード × 4Cap（8/16/32/64）=212ケース**。各5回測定では1060回。第3段階の追加分は20ワークロード × 4Cap = 80ケース。重いので、開発時は `--target` と `--caps` で対象を絞ることを推奨する。

## フィクスチャと計測範囲

- `generator.cpp` は **B+tree に依存せず**、`std::vector` / Fenwick / `std::lower_bound` / prefix sum などを用いて `in.txt` / `out.txt` を生成する。
- 通常は `in.txt` / `out.txt` をコミットする。第2段階の `range_erase` 3ケースと第3段階の `mixed_*` 20ケースは入力が大きいため **2ファイルをコミットせず、実行時に生成する**。`run.py` は両方なければジェネレータを1回コンパイル・実行して `out/benchmark/generated-fixtures/` に保存し、同一ジョブ内の各Capで使い回す。片方だけ存在する場合はエラー。
- すべてのケースで、**毎回バイト単位で期待出力を照合**。不一致・異常終了・タイムアウトは失敗。
- 入力読み込み・初期構築・ジェネレータ実行・最終結果の出力・コンパイルは計測外。操作を `std::chrono::steady_clock` で測定し、標準エラーへ `BENCHMARK_NS=...` を出力する。
- 出力内容は操作ごとのクエリ結果、または全要素の最終状態（`iterate` は走査結果）。結果は計測中に事前確保した配列へ保存し、計測後に出力する。
- 比較の同一性には CPU、コンパイラ、フラグ、入力と正解の SHA256 を用いる。生成したフィクスチャも同じ方法で識別する。
- 時間はベンチマーク本体の `BENCHMARK_NS`（処理部分だけ）、**Peak RSSはGNU timeの `%M`（プロセス全体の最大常駐メモリ量、KiB）**で取得する。両方とも5回測定の中央値を採用する。Peak RSSには入力・クエリ・出力配列、実行時ライブラリ、アロケータの管理領域も含まれる。

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

**第3段階：混合操作・入力分布・要素数推移（各100,000操作、初期100,000要素）**

- `mixed_uniform`：位置・キーを一様ランダムに選択。
- `mixed_edges`：位置は先頭/末尾の64要素以内に偏らせる。Setのeraseは最小/最大キーを選び、lower_boundは両端のキー付近を検索。
- `mixed_hotspot`：直前の位置から±64以内にアクセスし、近傍の反復操作を再現。Setでは直前のキーの近傍を検索・削除。
- `mixed_growth`：一様ランダム分布で、挿入を増やして要素数を増加させる。
- `mixed_shrink`：一様ランダム分布で、削除を増やして要素数を減少させる。
- `mixed_uniform` / `mixed_edges` / `mixed_hotspot` は共通の**ほぼ維持型**操作比率。`mixed_growth` / `mixed_shrink` は空間分布を一様に固定し、サイズ推移のみを変更する。
- 20操作を1ブロックとし、ブロック内を固定seedでシャッフル。以下の比率を**維持型の3分布**で共通に適用する。

| 種類 | insert | erase | range_erase | reverse | split_merge | access | prod | apply | lower_bound |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| set | 30% | 30% | — | — | — | — | — | — | 40% |
| vector | 30% | 20% | 10% | 20% | 5% | 15% | — | — | — |
| segtree | 25% | 15% | 5% | 15% | 5% | 15% | 20% | — | — |
| lazysegtree | 25% | 15% | 5% | 10% | 5% | 10% | 15% | 15% | — |


増加型 / 減少型では以下の比率に変更する（各行の合計100%、未記載の操作は実施しない）。

| 種類 | 傾向 | insert | erase | range_erase | reverse | split_merge | access | prod | apply | lower_bound |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| set | 増加 | 60% | 10% | — | — | — | — | — | — | 30% |
| set | 減少 | 10% | 60% | — | — | — | — | — | — | 30% |
| vector | 増加 | 50% | 10% | 5% | 15% | 5% | 15% | — | — | — |
| vector | 減少 | 10% | 40% | 10% | 20% | 5% | 15% | — | — | — |
| segtree | 増加 | 50% | 10% | 5% | 10% | 5% | 10% | 10% | — | — |
| segtree | 減少 | 10% | 40% | 10% | 10% | 5% | 10% | 15% | — | — |
| lazysegtree | 増加 | 40% | 10% | 5% | 10% | 5% | 10% | 10% | 10% | — |
| lazysegtree | 減少 | 10% | 30% | 10% | 10% | 5% | 10% | 10% | 15% | — |

- `range_erase` は長さ1〜3、通常の区間操作は長さ1〜32。操作番号が200の倍数となる区間処理では要素数の約1/4を使用。
- Insert/eraseの割合は対象別に調整しており、Vector/Segtree/LazySegtreeのサイズはおおむね維持される（range_eraseで緩やかに減少するケースもある）。
- Setの新しいキーは一意に生成。正解は `std::set`、他3種類は `std::vector` と独立したモノイド・作用の逐次計算で生成。**検索結果と全要素の最終状態**をバイト単位で照合する。
- フィクスチャは実行時生成し、4Cap間で同一の入力を共有。`generator.cpp` / `benchmark.cpp` は薄いラッパーで、共通実装は `mixed_generator.hpp` / `mixed_benchmark.hpp` に置く。

## 実行方法

```bash
# 第3段階：LazySegtreeの局所集中ケースを2Capで試す
python3 benchmark/run.py --target btree/lazysegtree/mixed_hotspot --caps 8,16 --runs 1

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

Actions → Performance benchmarks → Run workflow から対象ブランチ・種類/操作・Capを選択する。時間とPeak RSSをそれぞれ5回の**中央値**で比較し、各ベスト比120%超で🟡、150%超で🔴（遅くなっても警告のみ、結果の不一致は失敗）。

**main の成功実行だけ**（通常のPerformance benchmarksとA/B比較のmain側の測定結果）`benchmark-records` ブランチの `best.json` を更新する。時間の `best_ns` とメモリの `best_rss_kb` は**独立して最小値を更新**し、元コミット・実行URLも別々に保持する。既存の `best_ns` のみの記録は読み込み可能で、次回からRSSベストを追加する。PRブランチでは読み取り・比較のみ。既存の `btree/vector/insert`（Cap8固定）の旧記録は残し、新方式の `btree/vector/insert/cap8` と区別する。

GitHub-hosted runnerのCPUモデルやホスト負荷は変化するため、最速比は参考指標。計測ログはArtifactに14日保存する。

## 同一ランナーのA/B比較（Performance A/B comparison）

メモリプールなどの大規模変更を評価する際は、Actions → **Performance A/B comparison** → Run workflow から**workflowのブランチはmainを選択**し、`development_ref` に開発ブランチ名（例：`refactor/bplustree-node-pool-ids`）または40文字のコミットSHAを入力する。 `target`（デフォルト`all`）、`caps`、`runs`（3/5/7、デフォルト5）を指定できる。対象は既存のベンチマークと同じ。

- 同じGCC 15.2コンテナ・ランナーに開発版を別Worktreeでcheckoutし、**Main上の同じベンチマークソース**を両版でコンパイルする。コンパイル時の `-I.` は各版のWorktreeを指すため、ライブラリのヘッダだけが版ごとに切り替わる。入力・期待出力もMainのフィクスチャを共用し、**毎回両者の出力を完全照合**する。
- 実行順序はmain→開発版、開発版→mainを繰り返す。操作時間とPeak RSSのそれぞれの中央値、開発版/main比、構造別の幾何平均比、RSSの削減量を表示する。
- **開発ブランチにベンチマーク定義を取り込む必要はない。** 新しい操作（例：`access_random`）もMainに追加されていれば、そのまま古い開発ブランチと比較できる。ベンチマーク本体・フィクスチャはMainのものを両版で共用するため、ワークロードの食い違いは起こらない。開発版のライブラリがそのAPIを持たずコンパイルできない場合は、明確にビルドエラーとして失敗する。
- **推定ベスト**は、既存mainベスト ×（今回の開発版/今回のmain）として計算する。時間とPeak RSSの両方に適用。ただし実測記録ではなく、CPU間で相対性能差が維持されるという仮定に基づく**参考値**である。
- ベスト記録の検索では、コンパイラ・フラグ・フィクスチャSHA256の一致を必須とし、CPUモデルが一致する記録を優先する。異なるCPUやmainコミットの記録を使った推定はその旨を表示する。過去のメモリベストがなければ推定メモリ値を表示しない。
- **A/B比較でもmain側の実測中央値がベストを更新した場合は記録する。** 時間とPeak RSSは独立に更新し、開発版の実測値・推定ベストは記録しない。すべての対象ケースが正常終了した場合だけ `benchmark-records/best.json` を更新する。通常ベンチマークとA/B比較は同一のconcurrencyグループ（`benchmark-records`）で直列化し、共通の更新処理を用いる。過去ベストはA/B実行開始時の値を比較・推定に利用する。計測値・サンプル・元ベストの詳細は `out/benchmark/compare/results.json` に保存し、Artifactで14日保持する。重いので最初は対象・Capを絞るとよい。

ピークRSSはプロセスの生存期間全体での最大値であり、B+treeの内部確保容量や、操作区間だけのメモリ使用量を測っているものではない。ノードのみの容量比較が必要になった場合は、別途専用の測定を追加する。
