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

**合計57ワークロード × 4Cap（8/16/32/64）=228ケース**。各5回測定では1140回。第3段階の追加分は20ワークロード × 4Cap = 80ケース。重いので、開発時は `--target` と `--caps` で対象を絞ることを推奨する。

## フィクスチャと計測範囲

- `generator.cpp` は **B+tree に依存せず**、`std::vector` / Fenwick / `std::lower_bound` / prefix sum などを用いて `in.txt` / `out.txt` を生成する。
- 通常は `in.txt` / `out.txt` をコミットする。第2段階の `range_erase` 3ケース、第3段階の `mixed_*` 20ケース、および `iterate_interleaved_*` 4ケースは入力が大きいため **2ファイルをコミットせず、実行時に生成する**。`run.py` は両方なければジェネレータを1回コンパイル・実行して `out/benchmark/generated-fixtures/` に保存し、同一ジョブ内の各Capで使い回す。片方だけ存在する場合はエラー。
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

**Iterator更新後走査の回帰検証**

- `iterate_interleaved_reverse`（Vector / Segtree / LazySegtree）：10万要素に対して64回、`reverse(l, r)` と `for (const auto& value : tree)` による全走査を交互に実行。全区間・左右半分・両端・空区間・ランダム短区間を織り交ぜる。
- `iterate_interleaved_apply`（LazySegtree）：同じ走査パターンで64回、Affine区間更新（`a = 0, 1, 2`、`b = 0..1008`）と全走査を交互に実行する。
- 更新前に取得した要素のiteratorを保持し、**同じLeafへの参照が継続すること**と、コピーして`++`したときに**現在の順序の直後の要素へ進むこと**も毎回検証する（reverse時の位置は独立参照側で追跡）。
- 各周回の走査結果は順序依存の64bit FNV-1aハッシュで検証し、最終状態は全要素を直接比較する。ハッシュは衝突可能性があるため、中間の全値一致を数学的に保証するものではない。
- 計測には更新・全走査・ハッシュ計算・保存済みiteratorの確認を含む。初期構築・入出力は計測しない。現行`iterate`の純粋な走査速度とセットで評価する。生成器は独立した`std::vector`で期待値を計算する。


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

# reverse → 全走査を64回繰り返して比較
python3 benchmark/run.py --target btree/vector/iterate_interleaved_reverse --caps 8,16 --runs 5

# lazy作用 → 全走査を64回繰り返す
python3 benchmark/run.py --target btree/lazysegtree/iterate_interleaved_apply --caps 8 --runs 5

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

GitHub Actionsの実行一覧では、**実行待ち（Queued / Pending）の段階から** `run-name` に操作対象・Cap・実行回数を表示する。通常版は実行ブランチ、A/B版は `base_ref` と `development_ref` の両方を表示する。例：`A/B btree/vector/iterate cap=16 runs=5 | main vs perf/bplustree-static-pool-no-guard`。実行名は起動時の入力を示し、実行開始後にブランチ先端が変わっても表示を変更しない。実際に比較したSHAはA/B結果のサマリーで確認する。

Actions → Performance benchmarks → Run workflow から対象ブランチ・種類/操作・Capを選択する。時間とPeak RSSをそれぞれ5回の**中央値**で比較し、各ベスト比120%超で🟡、150%超で🔴（遅くなっても警告のみ、結果の不一致は失敗）。

**main の成功実行だけ**（通常のPerformance benchmarksとA/B比較のmain側の測定結果）`benchmark-records` ブランチの `best.json` を更新する。時間の `best_ns` とメモリの `best_rss_kb` は**独立して最小値を更新**し、元コミット・実行URLも別々に保持する。既存の `best_ns` のみの記録は読み込み可能で、次回からRSSベストを追加する。PRブランチでは読み取り・比較のみ。既存の `btree/vector/insert`（Cap8固定）の旧記録は残し、新方式の `btree/vector/insert/cap8` と区別する。

GitHub-hosted runnerのCPUモデルやホスト負荷は変化するため、最速比は参考指標。計測ログはArtifactに14日保存する。

## 同一ランナーのA/B比較（Performance A/B comparison）

Actions → **Performance A/B comparison** → Run workflow から、**workflowの実行ブランチは`main`を選択**する。比較対象は入力欄の2つのrefで決める。どちらもブランチ名または40文字のコミットSHAを指定できる。

| 入力 | 内容 | 既定値 |
| --- | --- | --- |
| `base_ref` | 比較元（旧実装） | `main` |
| `development_ref` | 比較先（新実装） | 必須 |
| `target` | 種類・操作の絞り込み | `all` |
| `caps` | ノード容量 | `all`（8/16/32/64） |
| `runs` | 両版の測定回数 | `5`（3/5/7） |

例えば、`get<Type>()` 最適化後のコミットと、`child_size()` 二重取得解消後のコミットを比較するなら以下のように設定する。

```text
Workflow branch: main
base_ref: 5057440515850611598c2516e9977f6f3b61c259
development_ref: 2e95a0b846c5e96e70c412c50cb2d43a6ce198ab
target: btree/vector/access_random
caps: all
runs: 5
```

- **比較スクリプト・ワークロード・フィクスチャは実行元のmainに固定**する。比較対象の2つのrefは、同一GCC 15.2コンテナ・ランナー内のそれぞれ独立したWorktreeにcheckoutし、各ライブラリのヘッダーだけを切り替えてコンパイルする。開発ブランチ側にベンチマーク定義をマージする必要はない。
- 出力・異常終了を毎回検証する。実行順はBase→Dev、Dev→Baseを交互に繰り返す。操作時間とPeak RSSのそれぞれの中央値、**Dev/Base比**、構造別の幾何平均比、RSSの削減量を表示する。
- 結果サマリーと`results.json`には、**Base・Devの入力refと実際のコミットSHA、およびベンチマーク定義元mainのSHA**を記録する。比較開始後にブランチが更新されても、解決したSHAの実装だけを測定する。
- **BaseのSHAがベンチマーク定義元mainのSHAと一致するときのみ**、main側の測定値で`benchmark-records`の時間・Peak RSSベストを更新できる。Devの実測値や推定値は記録しない。全ケース成功時だけ共通の更新アクションを起動する。
- **Baseがmain以外のコミットなら`benchmark-records`を一切更新しない。** この場合は履歴上のmainベストと推定Devベストも表示しない（比較対象と一致しないため）。mainがBaseの場合だけ、履歴mainベスト ×（今回のDev/Base）を推定Devベストとして表示する。
- 履歴ベストの参照には、コンパイラ・フラグ・フィクスチャSHA256の一致を必須とし、同CPUモデルを優先する。異なるCPUやmainコミットの記録による推定は参考値として扱う。
- 通常版とA/B比較は同じ`benchmark-records` concurrencyグループで直列化する。対象とCapを絞って小さく確認し、必要なら全ケースを測定する。サンプルと詳細は`out/benchmark/compare/results.json`に保存し、Artifactで14日保持する。


ピークRSSはプロセスの生存期間全体での最大値であり、B+treeの内部確保容量や、操作区間だけのメモリ使用量を測っているものではない。ノードのみの容量比較が必要になった場合は、別途専用の測定を追加する。
