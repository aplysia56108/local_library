#include <bits/stdc++.h>

#include <cstdint>

/** @brief 固定seedの疑似乱数。入力を変更する際は最速記録も別扱いにする。 */
inline std::uint32_t next_random(std::uint32_t& state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

/**
 * @brief 挿入クエリの入力と、std::vector による愚直な正解を生成する。
 * @details 実行ディレクトリに in.txt と out.txt を書き出す。
 *
 * ### 計算量
 * O(Q^2)。生成時間はベンチマークの計測対象に含まれない。
 */
signed main() {
  constexpr long long Q = 100'000;
  std::uint32_t state = 0x9e3779b9u;

  std::ofstream input("in.txt");
  std::ofstream output("out.txt");
  if (!input || !output) return 1;

  std::vector<long long> answer;
  input << Q << '\n';
  for (long long i = 0; i < Q; ++i) {
    const long long index = next_random(state) % (i + 1);
    const long long value = next_random(state);
    input << index << ' ' << value << '\n';
    answer.insert(answer.begin() + index, value);
  }
  for (long long value : answer) output << value << '\n';
}
