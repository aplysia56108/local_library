# Benchmark

各ライブラリの代表的な操作を、固定された入力で計測・検証する仕組みです。通常の `test` / `verify` とは分離し、GitHub Actions の `Performance benchmarks` から **手動でのみ**実行します。

## ディレクトリ構成

各ケースを `benchmark/<ライブラリ>/<対象>/<操作>/` にまとめます。例えば次のように配置します。

```text
benchmark/
├── readme.md
├── run.py
└── btree/
    └── vector/
        └── insert/
            ├── generator.cpp
            ├── in.txt
            ├── out.txt
            └── benchmark.cpp
```

- `generator.cpp`: 同じ入力を再現可能な形で `in.txt` に生成し、**ライブラリの実装に依存しない愚直な処理**で正解 `out.txt` を作る。生成に時間がかかってもよい。
- `in.txt`: ベンチマークが標準入力から読む固定入力。Git管理する。
- `out.txt`: 正しい標準出力（バイト単位で比較）。Git管理する。
- `benchmark.cpp`: `in.txt` を標準入力から読み、各クエリを処理して、正しい結果を**標準出力**へ出す。計測時間は `BENCHMARK_NS=<ナノ秒>` の形式で**標準エラー出力**へ1行出す。

入力生成、正解出力の作成、コンパイル、入力の読み込み、結果の出力は計測に含めません。実際に評価したい処理の前後を `std::chrono::steady_clock` で囲みます。すべての実行で出力を `out.txt` と照合し、不一致・異常終了・タイムアウトがあれば失敗とします。

## 最初のケース: btree/vector/insert

`Bplustree_Vector<long long, 8>` に、ランダムな位置への挿入を **100,000回**行います。入力は `Q`、続いて `index value` が `Q` 行です（`index` は挿入直前の要素数以下）。出力は挿入後の全要素を1行ずつ表示したものです。

`generator.cpp` は固定seedのxorshift32でクエリを生成し、`std::vector::insert` を使って `out.txt` を作ります。`std::vector` によるO(Q²)の生成・正解計算は、速度測定の対象ではありません。

### 入力の再生成

```bash
cd benchmark/btree/vector/insert
g++ -std=c++23 -O2 generator.cpp -o generator
./generator
# in.txt と out.txt の変更内容を確認し、必要なら両方コミットする
```

生成した `generator` 実行ファイルはGitに追加しないでください。seed・生成規則・クエリ数を変更すると過去の値と直接比較できなくなるため、`in.txt` / `out.txt` の内容から計算したハッシュで記録を分けます。

### ローカル実行

リポジトリのrootから:

```bash
python3 benchmark/run.py --target btree/vector/insert --runs 5
```

`g++ -std=c++23 -O2` を用いてコンパイルし、5回の計測でそれぞれ正解を検証した上で、実行時間の**中央値**を採用します。`--target all` で全ケースを実行します。結果・実行ファイルは `out/benchmark/` に保存します。

## GitHub Actions と最速記録

1. `Actions` → `Performance benchmarks` → `Run workflow` を選択する（workflowはmainにマージ後に表示される）。
2. 任意のブランチと `target`（`all` または個別ケース）を選んで実行する。
3. Summaryに「過去最速・今回の中央値・最速比」を表示する。120%超は🟡、150%超は🔴で警告する。警告だけではジョブを失敗にしない。
4. **main上の成功した実行だけ**、専用の `benchmark-records` ブランチの `best.json` に最速の中央値を記録する。開発ブランチは比較のみで記録を更新しない。

最速比は `今回の中央値 / 過去最速の中央値 × 100%` です。CPUモデル・コンパイラのバージョン・コンパイルフラグ・入出力フィクスチャのハッシュが一致する記録だけを比較し、違う場合は別の記録として扱います。GitHub-hosted runnerにはノイズがあるため、**歴代最速との比較はあくまで参考値**です。

計測環境は `ubuntu-24.04` + `gcc:15.2` のコンテナです。ActionsのArtifactに計測ログ（14日保存）を残しますが、最速記録は専用ブランチで保持します。
