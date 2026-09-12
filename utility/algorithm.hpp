#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

using namespace std;

// ==========================================
// 1. 探索・最適化アルゴリズム
// ==========================================

/**
 * @brief 整数値に対する二分探索
 * @param ok 条件を満たすことが確定している初期値
 * @param ng 条件を満たさないことが確定している初期値
 * @param f 条件判定関数 bool f(long long mid)
 * @return 条件を満たす限界の境界値 (ok)
 * @note 時間計算量: O(log|ok - ng|)
 */
template <class F>
long long bin_search(long long ok, long long ng, const F& f) {
  while (abs(ok - ng) > 1) {
    long long mid = ok + (ng - ok) / 2;
    (f(mid) ? ok : ng) = mid;
  }
  return ok;
}

/**
 * @brief 実数値（浮動小数点）に対する指定回数二分探索
 * @param ok 条件を満たす境界の初期値
 * @param ng 条件を満たさない境界の初期値
 * @param f 条件判定関数 bool f(double mid)
 * @param iter ループの試行回数 (デフォルト: 90回で精度十分)
 * @return 条件を満たす境界値 (ok)
 */
template <class T, class F>
T bin_search_double(T ok, T ng, const F& f, int iter = 90) {
  while (iter--) {
    T mid = (ok + ng) / 2;
    (f(mid) ? ok : ng) = mid;
  }
  return ok;
}

/**
 * @brief 黄金分割探索（凸関数の極値探索）
 * @param f 評価関数 T f(long long x)
 * @param min 探索範囲の最小値
 * @param max 探索範囲の最大値
 * @param comp 比較関数 (デフォルト: std::less で極小値を探索)
 * @return pair<long long, T> {極値を達成するインデックス x, その評価値 f(x)}
 * @note 時間計算量: O(log(max - min))
 */
template <class F,
          class T = decltype(std::declval<F>()(std::declval<long long>())),
          class Compare = std::less<T>>
pair<long long, T> golden_section_search(F f, long long min, long long max,
                                         Compare comp = Compare()) {
  assert(min <= max);
  long long a = min - 1, x, b;
  {
    long long s = 1, t = 2;
    while (t < max - min + 2) {
      std::swap(s += t, t);
    }
    x = a + t - s;
    b = a + t;
  }
  T fx = f(x), fy;
  while (a + b != 2 * x) {
    const long long y = a + b - x;
    if (max < y || comp(fx, (fy = f(y)))) {
      b = a;
      a = y;
    } else {
      a = x;
      x = y;
      fx = fy;
    }
  }
  return {x, fx};
}

// ==========================================
// 2. 配列・ベクター操作
// ==========================================

/**
 * @brief 配列をソートした際の元のインデックス（順序）を取得 (argsort)
 * @param v 対象の配列
 * @param ascending_order true: 昇順, false: 降順
 * @return vector<int> 元のインデックス配列
 * @note 時間計算量: O(N log N)
 */
template <class T>
vector<long long> argsort(const vector<T>& v, bool ascending_order = true) {
  vector<long long> res(v.size());
  iota(res.begin(), res.end(), 0);
  if (ascending_order)
    sort(res.begin(), res.end(), [&](long long i, long long j) {
      return v[i] < v[j];
    });
  else
    sort(res.begin(), res.end(), [&](long long i, long long j) {
      return v[i] > v[j];
    });
  return res;
}

/**
 * @brief 配列の全要素の総和を計算
 * @param v 対象の配列
 * @return T 総和
 */
template <class T>
T sumv(const vector<T>& v) {
  return accumulate(v.begin(), v.end(), T(0));
}

/**
 * @brief 配列から重複を除去してソートした結果を取得
 * @param v 対象の配列
 * @return vector<T> ソート済みの重複なし配列
 * @note 時間計算量: O(N log N)
 */
template <class T>
vector<T> uniq(vector<T> v) {
  sort(v.begin(), v.end());
  v.erase(unique(v.begin(), v.end()), v.end());
  return v;
}

/**
 * @brief 順列の逆置換（Inverse Permutation）の計算
 * @param p 0 ~ N-1 の順列
 * @return vector<long long> p[inv[i]] = i となる逆順列 inv
 */
inline vector<long long> inverse(const vector<long long>& p) {
  long long n = p.size();
  vector<long long> inv(n);
  for (long long i = 0; i < n; i++) inv[p[i]] = i;
  return inv;
}

// ==========================================
// 3. 累積和 (1次元 / 2次元)
// ==========================================

/**
 * @brief 1次元配列の累積和（サイズ N、元と同じインデックス位置で返す）
 * @details res[i] = v[0] + ... + v[i]
 */
template <typename T>
vector<T> acc0(const vector<T>& v) {
  if (v.empty()) return {};
  vector<T> res(v.size());
  res[0] = v[0];
  for (size_t i = 1; i < v.size(); i++) res[i] = res[i - 1] + v[i];
  return res;
}

/**
 * @brief 1次元配列の半開区間累積和（サイズ N+1、先頭 0）
 * @details res[i] = v[0] + ... + v[i-1] (半開区間 [l, r) の和が res[r] - res[l]
 * で計算可能)
 */
template <typename T>
vector<T> acc1(const vector<T>& v) {
  vector<T> res(v.size() + 1, 0);
  for (size_t i = 0; i < v.size(); i++) res[i + 1] = res[i] + v[i];
  return res;
}

/**
 * @brief 2次元グリッドの累積和（H x W、インデックスそのまま）
 */
template <typename T>
vector<vector<T>> acc0(vector<vector<T>> v) {
  if (v.empty() || v[0].empty()) return v;
  int h = v.size(), w = v[0].size();
  for (int i = 0; i < h; i++) {
    for (int j = 1; j < w; j++) v[i][j] += v[i][j - 1];
  }
  for (int i = 1; i < h; i++) {
    for (int j = 0; j < w; j++) v[i][j] += v[i - 1][j];
  }
  return v;
}

/**
 * @brief 2次元グリッドの半開区間累積和（(H+1) x (W+1)、先頭行・列 0）
 * @details 矩形領域 [r1, r2) x [c1, c2) の和を高速に求めるための構造
 */
template <typename T>
vector<vector<T>> acc1(const vector<vector<T>>& v) {
  if (v.empty() || v[0].empty()) return {};
  int h = v.size(), w = v[0].size();
  vector<vector<T>> res(h + 1, vector<T>(w + 1, 0));
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      res[i + 1][j + 1] = v[i][j] + res[i + 1][j] + res[i][j + 1] - res[i][j];
    }
  }
  return res;
}

// ==========================================
// 4. 数学・計算用ユーティリティ
// ==========================================

/**
 * @brief 2乗計算 (n^2)
 */
template <class T>
constexpr T sq(T n) {
  return n * n;
}

/**
 * @brief 整数精度での平方根 floor(sqrt(n))
 * @return long long m^2 <= n を満たす最大の整数 m
 */
inline long long sqr(long long n) {
  if (n <= 0) return 0;
  long long res = sqrtl(n);
  while ((res + 1) <= n / (res + 1)) res++;
  while (res * res > n) res--;
  return res;
}

/**
 * @brief 切り上げ除算 ceil(x / y)
 * @note 負の数にも対応
 */
inline long long ceil(long long x, long long y) {
  if (y < 0) return ceil(-x, -y);
  return (x >= 0 ? (x + y - 1) / y : x / y);
}

/**
 * @brief 切り捨て除算 floor(x / y)
 * @note 負の数にも対応
 */
inline long long floor(long long x, long long y) {
  if (y < 0) return floor(-x, -y);
  return (x >= 0 ? x / y : (x - y + 1) / y);
}

/**
 * @brief 1 から n までの和 (三角数: n*(n+1)/2)
 */
constexpr long long tri(long long n) {
  return n * (n + 1) / 2;
}

/**
 * @brief l から r までの連続整数の和
 */
constexpr long long tri(long long l, long long r) {
  return (l + r) * (r - l + 1) / 2;
}

/**
 * @brief 数学的に正しい余り計算 (非負の剰余: 常に 0 <= res < d)
 */
template <typename T>
constexpr T modulo(T n, T d) {
  return (n % d + d) % d;
}

/**
 * @brief 最小の除外数 (Minimum Excluded: 配列に含まれない最小の非負整数)
 * @param a 判定対象のコレクション
 * @note 負の値は無視されます
 */
template <class T>
long long mex(const T& a) {
  int n = a.size();
  vector<int> cnt(n + 1, 0);
  for (auto x : a) {
    if (x < 0 || x > n) continue;
    cnt[x]++;
  }
  int res = 0;
  while (cnt[res]) res++;
  return res;
}