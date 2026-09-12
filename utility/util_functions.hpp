#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// ==========================================
// 1. vector / pair 演算子オーバーロード
// ==========================================

/**
 * @brief vector の全要素をインクリメント (前置 ++v)
 */
template <class T>
std::vector<T>& operator++(std::vector<T>& v) {
  for (auto& e : v) e++;
  return v;
}

/**
 * @brief vector の全要素をインクリメント (後置 v++)
 */
template <class T>
std::vector<T> operator++(std::vector<T>& v, signed) {
  auto res = v;
  for (auto& e : v) e++;
  return res;
}

/**
 * @brief vector の全要素をデクリメント (前置 --v)
 */
template <class T>
std::vector<T>& operator--(std::vector<T>& v) {
  for (auto& e : v) e--;
  return v;
}

/**
 * @brief vector の全要素をデクリメント (後置 v--)
 */
template <class T>
std::vector<T> operator--(std::vector<T>& v, signed) {
  auto res = v;
  for (auto& e : v) e--;
  return res;
}
namespace std {

/**
 * @brief pair の加算代入 (a += b)
 */
template <class T, class U>
std::pair<T, U>& operator+=(std::pair<T, U>& a, const std::pair<T, U>& b) {
  a.first += b.first;
  a.second += b.second;
  return a;
}

/**
 * @brief pair の減算代入 (a -= b)
 */
template <class T, class U>
std::pair<T, U>& operator-=(std::pair<T, U>& a, const std::pair<T, U>& b) {
  a.first -= b.first;
  a.second -= b.second;
  return a;
}

/**
 * @brief pair の加算 (a + b)
 */
template <class T, class U>
std::pair<T, U> operator+(const std::pair<T, U>& a, const std::pair<T, U>& b) {
  return {a.first + b.first, a.second + b.second};
}

/**
 * @brief pair の減算 (a - b)
 */
template <class T, class U>
std::pair<T, U> operator-(const std::pair<T, U>& a, const std::pair<T, U>& b) {
  return {a.first - b.first, a.second - b.second};
}
}  // namespace std
// ==========================================
// 2. グラフ・集計ユーティリティ
// ==========================================

/**
 * @brief 標準入力から辺情報（1-indexed）を読み込んで隣接リストを構築
 * @param g グラフの隣接リスト (サイズ N で初期化済みであること)
 * @param m 辺の数 (-1 の場合は N-1 本として処理)
 * @param bidirected 無向グラフの場合 true, 有向グラフの場合 false
 */
inline void read_edges(std::vector<std::vector<long long>>& g, int m = -1,
                       bool bidirected = true) {
  if (m == -1) m = (int)g.size() - 1;
  for (int i = 0; i < m; i++) {
    int u, v;
    std::cin >> u >> v;
    u--;
    v--;
    g[u].push_back(v);
    if (bidirected) g[v].push_back(u);
  }
}

/**
 * @brief 配列要素の出現頻度テーブルを作成 (バケット)
 * @param v 対象の配列 (非負整数)
 * @param mx 最大値の指定 (-1 の場合は自動算出)
 * @return std::vector<int> res[x] = x の出現回数
 */
inline std::vector<long long> count_freq(const std::vector<long long>& v,
                                         long long mx = -1) {
  if (v.empty()) return {};
  if (mx == -1) mx = *std::max_element(v.begin(), v.end());
  std::vector<long long> res(mx + 1, 0);
  for (auto x : v) {
    if (x >= 0 && x <= mx) res[x]++;
  }
  return res;
}

/**
 * @brief 任意の型のコンテナ要素を map で出現カウント
 */
template <class Container>
auto count_map(const Container& v) {
  using T = typename Container::value_type;
  std::map<T, int> res;
  for (const auto& x : v) res[x]++;
  return res;
}

/**
 * @brief s から始まる連続した n 個の整数配列を生成
 */
inline std::vector<long long> make_iota(long long n, long long s = 0) {
  std::vector<long long> a(n);
  std::iota(a.begin(), a.end(), s);
  return a;
}

// ==========================================
// 3. 合計・最大・最小・更新チェック
// ==========================================

/**
 * @brief 任意のコンテナの全要素の総和を取得 (mint や pair 等にも対応)
 * @param c 対象のコンテナ
 * @return T 総和。mapの場合はvalueのみの合計
 */
template <class Container, class T = typename Container::value_type>
T sumv(const Container& c) {
  return std::accumulate(std::begin(c), std::end(c), T{});
}

/**
 * @brief std::map 用の sumv (値 value の和のみを計算)
 */
template <class K, class V, class Compare, class Alloc>
V sumv(const std::map<K, V, Compare, Alloc>& m) {
  V res{};
  for (const auto& [key, val] : m) {
    res += val;
  }
  return res;
}

/**
 * @brief std::unordered_map 用の sumv (値 value の和のみを計算)
 */
template <class K, class V, class Hash, class KeyEqual, class Alloc>
V sumv(const std::unordered_map<K, V, Hash, KeyEqual, Alloc>& m) {
  V res{};
  for (const auto& [key, val] : m) {
    res += val;
  }
  return res;
}

/**
 * @brief コンテナの最大値を取得
 */
template <class Container>
auto maxv(const Container& a) {
  return *std::max_element(a.begin(), a.end());
}

/**
 * @brief std::map 用の maxv (値 value の最大値を取得)
 */
template <class K, class V, class Compare, class Alloc>
V maxv(const std::map<K, V, Compare, Alloc>& m) {
  assert(!m.empty());
  V res = m.begin()->second;
  for (const auto& [key, val] : m) {
    if (res < val) res = val;
  }
  return res;
}

/**
 * @brief std::unordered_map 用の maxv (値 value の最大値を取得)
 */
template <class K, class V, class Hash, class KeyEqual, class Alloc>
V maxv(const std::unordered_map<K, V, Hash, KeyEqual, Alloc>& m) {
  assert(!m.empty());
  V res = m.begin()->second;
  for (const auto& [key, val] : m) {
    if (res < val) res = val;
  }
  return res;
}

/**
 * @brief コンテナの最小値を取得
 */
template <class Container>
auto minv(const Container& a) {
  return *std::min_element(a.begin(), a.end());
}

/**
 * @brief std::map 用の minv (値 value の最小値を取得)
 */
template <class K, class V, class Compare, class Alloc>
V minv(const std::map<K, V, Compare, Alloc>& m) {
  assert(!m.empty());
  V res = m.begin()->second;
  for (const auto& [key, val] : m) {
    if (val < res) res = val;
  }
  return res;
}

/**
 * @brief std::unordered_map 用の minv (値 value の最小値を取得)
 */
template <class K, class V, class Hash, class KeyEqual, class Alloc>
V minv(const std::unordered_map<K, V, Hash, KeyEqual, Alloc>& m) {
  assert(!m.empty());
  V res = m.begin()->second;
  for (const auto& [key, val] : m) {
    if (val < res) res = val;
  }
  return res;
}

/**
 * @brief コンテナの最大値を与える最初のインデックスを取得
 */
template <class Container>
long long argmax(const Container& a) {
  return std::distance(a.begin(), std::max_element(a.begin(), a.end()));
}

/**
 * @brief コンテナの最小値を与える最初のインデックスを取得
 */
template <class Container>
long long argmin(const Container& a) {
  return std::distance(a.begin(), std::min_element(a.begin(), a.end()));
}

// int と long long の混用時用 max / min オーバーロード
inline long long max(signed x, long long y) {
  return std::max((long long)x, y);
}
inline long long max(long long x, signed y) {
  return std::max(x, (long long)y);
}
inline long long min(signed x, long long y) {
  return std::min((long long)x, y);
}
inline long long min(long long x, signed y) {
  return std::min(x, (long long)y);
}

/**
 * @brief a を max(a, b) で更新し、更新が起きたかを返す
 */
template <class T, class S>
bool chmax(T& a, const S& b) {
  if (a < (T)b) {
    a = (T)b;
    return true;
  }
  return false;
}

/**
 * @brief a を min(a, b) で更新し、更新が起きたかを返す
 */
template <class T, class S>
bool chmin(T& a, const S& b) {
  if ((T)b < a) {
    a = (T)b;
    return true;
  }
  return false;
}

/**
 * @brief 半開区間 [l, r) に x が含まれるか判定 (l <= x < r)
 */
constexpr bool in_range(long long l, long long x, long long r) {
  return l <= x && x < r;
}

/**
 * @brief 閉区間 [l, r] に x が含まれるか判定 (l <= x <= r)
 */
constexpr bool in_range_inclusive(long long l, long long x, long long r) {
  return l <= x && x <= r;
}

/**
 * @brief 任意型に対応して、long long型でサイズを取得
 */
template <class T>
inline long long sz(const T& x) {
  using std::size;
  return static_cast<long long>(size(x));
}

// initializer_list 用のオーバーロード (sz({1, 2, 3}) に対応)
template <class T>
inline long long sz(std::initializer_list<T> x) {
  return static_cast<long long>(x.size());
}

// ==========================================
// 4. 文字・文字列・数値相互変換
// ==========================================

inline long long ctoi(char c, char start = '0') {
  return c - start;
}
inline long long atoi(char c, char start = 'a') {
  return c - start;
}

/**
 * @brief 数字文字列を int の配列に変換 (範囲外文字は -1)
 */
inline std::vector<long long> ctoi(const std::string& s, char start = '0') {
  std::vector<long long> res(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    long long x = s[i] - start;
    res[i] = (x < 0 || x >= 10) ? -1 : x;
  }
  return res;
}

/**
 * @brief 英小文字文字列を int(0~25) の配列に変換 (範囲外文字は -1)
 */
inline std::vector<long long> atoi(const std::string& s, char start = 'a') {
  std::vector<long long> res(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    long long x = s[i] - start;
    res[i] = (x < 0 || x >= 26) ? -1 : x;
  }
  return res;
}

inline std::vector<std::vector<long long>> ctoi(
    const std::vector<std::string>& s, char start = '0') {
  std::vector<std::vector<long long>> res(s.size());
  for (size_t i = 0; i < s.size(); i++) res[i] = ctoi(s[i], start);
  return res;
}

inline std::vector<std::vector<long long>> atoi(
    const std::vector<std::string>& s, char start = 'a') {
  std::vector<std::vector<long long>> res(s.size());
  for (size_t i = 0; i < s.size(); i++) res[i] = atoi(s[i], start);
  return res;
}

inline std::string itoc(const std::vector<long long>& v, char start = '0') {
  std::string res(v.size(), ' ');
  for (size_t i = 0; i < v.size(); i++)
    res[i] = static_cast<char>(start + v[i]);
  return res;
}

inline std::string itoa(const std::vector<long long>& v, char start = 'a') {
  return itoc(v, start);
}

inline std::vector<std::string> itoc(
    const std::vector<std::vector<long long>>& v, char start = '0') {
  std::vector<std::string> res(v.size());
  for (size_t i = 0; i < v.size(); i++) res[i] = itoc(v[i], start);
  return res;
}

inline std::vector<std::string> itoa(
    const std::vector<std::vector<long long>>& v, char start = 'a') {
  return itoc(v, start);
}

// ==========================================
// 5. 反転・絶対値・数学補助
// ==========================================
#if defined(__GNUG__) && __GNUC__ < 15
/**
 * @brief 128bit整数用のabs
 */
constexpr __int128_t abs(__int128_t x) {
  return x < 0 ? -x : x;
}
#endif
/**
 * @brief 2の累乗判定 (i > 0 かつ i が 2^k の形か)
 */
constexpr bool ispow2(long long i) {
  return i > 0 && (i & -i) == i;
}

/**
 * @brief 10進数での桁数を取得
 */
inline long long countDigits(long long n) {
  return (long long)std::to_string(std::abs(n)).size();
}

/**
 * @brief lower_bound のインデックスを高速取得 (参照渡し)
 */
template <class T>
long long lower_idx(const std::vector<T>& v, const T& a) {
  return std::lower_bound(v.begin(), v.end(), a) - v.begin();
}

/**
 * @brief upper_bound のインデックスを高速取得 (参照渡し)
 */
template <class T>
long long upper_idx(const std::vector<T>& v, const T& a) {
  return std::upper_bound(v.begin(), v.end(), a) - v.begin();
}

// ==========================================
// 6. ビット演算ユーティリティ
// ==========================================

/**
 * @brief 立っているビットの個数 (popcount)
 */
inline long long pop_count(long long x) {
  return __builtin_popcountll(x);
}

/**
 * @brief 最上位ビット (MSB) の位置を 0-indexed で取得 (0 の場合は -1)
 */
inline long long top_bit(long long t) {
  return (t == 0 ? -1 : 63 - __builtin_clzll(t));
}

/**
 * @brief 最下位ビット (LSB) の位置を 0-indexed で取得 (0 の場合は 64)
 */
inline long long bottom_bit(long long t) {
  return (t == 0 ? 64 : __builtin_ctzll(t));
}

/**
 * @brief msk の i 番目のビットが立っているか判定 (0 または 1)
 */
constexpr long long get_bit(long long msk, long long i) {
  return (msk >> i) & 1;
}

/**
 * @brief 下位 x ビットがすべて 1 のマスクを取得 ((1 << x) - 1)
 */
constexpr long long mask(long long x) {
  return x >= 64 ? -1LL : ((1LL << x) - 1);
}

/**
 * @brief msk の i 番目のビットを 1 にした値を返す
 */
constexpr long long set_bit(int i, long long msk = 0) {
  return msk | (1LL << i);
}

/**
 * @brief i 番目のビットを 0 にした値を返す
 */
constexpr long long unset_bit(int i, long long msk) {
  assert(0 <= i && i < 64);
  return msk & ~(1LL << i);
}

/**
 * @brief i 番目のビットを反転した値を返す
 */
constexpr long long flip_bit(int i, long long msk = 0) {
  assert(0 <= i && i < 64);
  return msk ^ (1LL << i);
}

/**
 * @brief 異質な型の tuple から i 番目の要素を variant で取得
 */
template <typename... Args>
auto tuple_at(const std::tuple<Args...>& t, size_t i) {
  constexpr size_t N = sizeof...(Args);
  assert(i < N);
  using VariantType = std::variant<Args...>;

  return [&]<size_t... Is>(std::index_sequence<Is...>) {
    VariantType res;
    bool found = false;
    ((Is == i ? (res = VariantType(std::in_place_index<Is>, std::get<Is>(t)),
                 found = true)
              : false),
     ...);
    return res;
  }(std::make_index_sequence<N>{});
}

namespace detail {

// 型昇格ルール:
// 1. const char* や char[] などの文字列リテラル -> std::string に変換
// 2. 整数型（bool以外） -> long long に変換
// 3. その他（double, mint, bool など） -> そのままの型
template <typename T>
struct promote_type {
  using type = std::conditional_t<
      std::is_same_v<std::decay_t<T>, const char*> ||
          std::is_same_v<std::decay_t<T>, char*>,
      std::string,
      std::conditional_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                         long long, T>>;
};

template <typename T>
using promote_type_t = typename promote_type<T>::type;

}  // namespace detail

/**
 * @brief 多次元 std::vector を一括生成するユーティリティ
 * @details 例: auto dp = make_vector(n, 3, inf); -> vector<vector<long
 * long>>(n, vector<long long>(3, inf))
 */
template <typename Size, typename T>
auto make_vector(Size n, const T& init_value) {
  using ValueType = detail::promote_type_t<T>;
  return std::vector<ValueType>(n, static_cast<ValueType>(init_value));
}

template <typename Head, typename Second, typename... Tail>
auto make_vector(Head head, Second second, Tail&&... tail) {
  auto inner = make_vector(second, std::forward<Tail>(tail)...);
  return std::vector<decltype(inner)>(head, inner);
}

// ショートカット用の別名（タイピング量削減用）
template <typename... Args>
auto makev(Args&&... args) {
  return make_vector(std::forward<Args>(args)...);
}

// ==========================================
// 7. 入出力ユーティリティ
// ==========================================

/**
 * @brief 2列の入力を受けてそれぞれの vector の pair を返す
 * @tparam T1 1列目の要素型（デフォルト: long long）
 * @tparam T2 2列目の要素型（デフォルト: long long）
 * @param m 行数
 * @return std::pair<std::vector<T1>, std::vector<T2>>
 */
template <typename T1 = long long, typename T2 = long long>
inline std::pair<std::vector<T1>, std::vector<T2>> read_column2(int m) {
  std::vector<T1> a(m);
  std::vector<T2> b(m);
  for (int i = 0; i < m; i++) std::cin >> a[i] >> b[i];

  return {a, b};
}

/**
 * @brief 3列の入力を受けてそれぞれの vector の tuple を返す
 * @tparam T1 1列目の要素型（デフォルト: long long）
 * @tparam T2 2列目の要素型（デフォルト: long long）
 * @tparam T3 3列目の要素型（デフォルト: long long）
 * @param m 行数
 * @return std::tuple<std::vector<T1>, std::vector<T2>, std::vector<T3>>
 */
template <typename T1 = long long, typename T2 = long long,
          typename T3 = long long>
inline std::tuple<std::vector<T1>, std::vector<T2>, std::vector<T3>>
read_column3(int m) {
  std::vector<T1> a(m);
  std::vector<T2> b(m);
  std::vector<T3> c(m);
  for (int i = 0; i < m; i++) std::cin >> a[i] >> b[i] >> c[i];
  return {a, b, c};
}

/**
 * @brief 4列の入力を受けてそれぞれの vector の tuple を返す
 * @tparam T1 1列目の要素型（デフォルト: long long）
 * @tparam T2 2列目の要素型（デフォルト: long long）
 * @tparam T3 3列目の要素型（デフォルト: long long）
 * @tparam T4 4列目の要素型（デフォルト: long long）
 * @param m 行数
 * @return std::tuple<std::vector<T1>, std::vector<T2>, std::vector<T3>,
 * std::vector<T4>>
 */
template <typename T1 = long long, typename T2 = long long,
          typename T3 = long long, typename T4 = long long>
inline std::tuple<std::vector<T1>, std::vector<T2>, std::vector<T3>,
                  std::vector<T4>>
read_column4(int m) {
  std::vector<T1> a(m);
  std::vector<T2> b(m);
  std::vector<T3> c(m);
  std::vector<T4> d(m);
  for (int i = 0; i < m; i++) {
    std::cin >> a[i] >> b[i] >> c[i] >> d[i];
  }
  return {a, b, c, d};
}