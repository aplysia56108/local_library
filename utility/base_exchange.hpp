#pragma once

#include <algorithm>
#include <string>

/**
 * @brief 10進数の数値をN進数（2〜36進数）表現の文字列に変換する
 * @tparam T 整数型 (int, long long 等)
 * @param x 変換対象の数値 (負の値にも対応)
 * @param b 基数 (デフォルト: 10, 2〜36進数に対応)
 * @return b進数表現の文字列 (負数の場合は先頭に '-' が付与される)
 * @example to_baseB(10, 2) -> "1010", to_baseB(31, 16) -> "1F", to_baseB(-10,
 * 2) -> "-1010"
 */
template <class T>
inline std::string to_baseB(T x, int b = 10) {
  std::string ans;
  bool is_minus = false;
  if (x < 0) {
    x *= -1;
    is_minus = true;
  }
  do {
    int num = x % b;
    ans += (char)((num <= 9) ? ('0' + num) : ('A' + num - 10));
    x /= b;
  } while (x != 0);

  if (is_minus) ans += '-';
  reverse(ans.begin(), ans.end());
  return ans;
}

/**
 * @brief b進数表現の文字列を10進数の数値に変換する
 * @param x b進数を表す文字列 (例: "1010", "1F")
 * @param b 基数 (デフォルト: 10, 2〜36進数に対応)
 * @return 10進数に変換された値 (long long)
 * @example to_base10("1010", 2) -> 10, to_base10("1F", 16) -> 31
 */
inline long long to_base10(const std::string& x, int b = 10) {
  long long ans = 0, base = 1;
  for (int i = x.length() - 1; i >= 0; --i) {
    int num = ('0' <= x[i] && x[i] <= '9') ? (x[i] - '0') : (x[i] - 'A' + 10);
    ans += base * num;
    base *= b;
  }
  return ans;
}

/**
 * @brief 10進数の数値を2進数の文字列に変換する
 * @param n 変換対象の整数 (0以上の整数)
 * @param d 最小桁数指定 (デフォルト: -1)。指定した桁数に満たない場合は上位桁を
 * '0' パディングする
 * @return 2進数表現の文字列
 * @example bin(10) -> "1010", bin(5, 8) -> "00000101"
 */
inline std::string bin(long long n, int d = -1) {
  if (n == 0) {
    std::string res = "0";
    if (d >= 0) {
      while ((int)res.size() < d) res = "0" + res;
    }
    return res;
  }

  std::string res;
  while (n) {
    res.push_back('0' + (n & 1));
    n >>= 1;
  }
  if (d >= 0) {
    while ((int)res.size() < d) res.push_back('0');
  }
  reverse(res.begin(), res.end());
  return res;
}