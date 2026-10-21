#include <utility>
#include <compare>
#include <vector>
#include <cassert>
#include <cmath>
#include <limits>
#include <type_traits>

#include "common/common.hpp"
#include "data-structure/btree/bplustree-set.hpp"

constexpr long long Interval_Inf = library_inf;

/**
 * @brief 実数直線上の区間を白・黒の2色で管理する集合。
 * @details
 * 初期状態はすべて白 (false)。
 * 色が切り替わる境界だけを B+ tree で管理する。
 *
 * Point{x, false} は x の直前、Point{x, true} は
 * x の直後を表す。
 *
 * 各区間は左境界以上・右境界未満として扱う。
 * 管理する座標は [INTERVAL_MIN, INTERVAL_MAX] 内とする。
 *
 * @tparam T 座標の型
 * @tparam INTERVAL_MIN 管理範囲の下限
 * @tparam INTERVAL_MAX 管理範囲の上限
 */
template <class T = long long, T INTERVAL_MIN = static_cast<T>(-Interval_Inf),
          T INTERVAL_MAX = static_cast<T>(Interval_Inf)>
class IntervalSet {
 public:
  /**
   * @brief 区間の境界を表す。
   * @param x 境界の座標
   * @param plus_eps false なら x の直前、true なら直後
   */
  struct Point {
    T x;
    bool plus_eps;

    auto operator<=>(const Point&) const = default;
  };
  struct IntervalState;

 private:
  Bplustree_Set<Point> _st;

 public:
  /**
   * @brief 単色区間の状態を表す。
   * @details
   * 区間は [l_end, r_end) として表現される。
   * color は false が白、true が黒。
   */
  struct IntervalState {
    Point l_end, r_end;
    bool color;

    /**
     * @brief 区間に含まれる整数の範囲を開区間 [l, r) として返す。
     * @details
     * 開閉端点を考慮し、整数点を含まない場合は l == r を返す。
     * 変換結果が long long の範囲内に収まることを要求する。
     *
     * ### 計算量
     * O(1)
     */
    std::pair<long long, long long> to_int_interval() const {
      long long l, r;

      if constexpr (std::is_integral_v<T>) {
        l = l_end.x + l_end.plus_eps;
        r = r_end.x + r_end.plus_eps;
      } else {
        auto lx = static_cast<long double>(l_end.x);
        auto rx = static_cast<long double>(r_end.x);

        long double ll = l_end.plus_eps ? std::floor(lx) + 1 : std::ceil(lx);
        long double rr = r_end.plus_eps ? std::floor(rx) + 1 : std::ceil(rx);

        l = static_cast<long long>(ll);
        r = static_cast<long long>(rr);
      }

      return {l, r};
    }

    /**
     * @brief 実数上の区間の長さを返す。
     * @details 端点の開閉は長さに影響しない。
     *
     * ### 計算量
     * O(1)
     */
    T length() const {
      return r_end.x - l_end.x;
    };
  };

  IntervalSet() = default;

  /**
   * @brief 指定区間を指定色で上書きする。
   * @details
   * 実際に色が変化した区間について、更新前の状態を返す。
   * 戻り値の区間は左から順に並ぶ。
   *
   * @param color 上書き後の色
   * @param l 左端の座標
   * @param r 右端の座標
   * @param l_open 左端を含まない場合 true
   * @param r_open 右端を含まない場合 true
   * @return 色が変化した区間の更新前の状態
   *
   * ### 計算量
   * 1回あたり償却 O(log(Q+1))。
   * Q はこれまでの overwrite / flip の操作回数。
   * 1回の最悪計算量は O((K+1) log N)。
   */
  std::vector<IntervalState> overwrite(bool color, T l, T r,
                                       bool l_open = false,
                                       bool r_open = true) {
    std::vector<IntervalState> res;
    Point l_end{l, l_open};
    Point r_end{r, !r_open};
    if (!(l_end < r_end)) return res;
    Point prev = l_end;
    auto l_it = _st.upper_bound(l_end);
    auto r_it = _st.lower_bound(r_end);

    bool current_color = (l_it - _st.begin()) % 2;
    while (l_it != r_it) {
      if (current_color ^ color) res.push_back({prev, *l_it, current_color});
      prev = *l_it;
      ++l_it;
      current_color ^= true;
    }
    if (current_color ^ color) res.push_back({prev, r_end, current_color});

    if (res.empty()) return res;

    bool left_color = (_st.lower_bound(l_end) - _st.begin()) % 2;
    bool right_color = (_st.upper_bound(r_end) - _st.begin()) % 2;

    auto erase_it = _st.lower_bound(l_end);

    while (erase_it != _st.end() && *erase_it <= r_end)
      erase_it = _st.erase(erase_it);

    if (left_color != color) _st.insert(l_end);
    if (right_color != color) _st.insert(r_end);
    return res;
  };

  /**
   * @brief 指定区間内の色を反転する。
   * @details
   * 内部の境界は変更せず、両端の境界だけを反転する。
   *
   * ### 計算量
   * O(log N)
   */
  void flip(T l, T r, bool l_open = false, bool r_open = true) {
    Point l_end{l, l_open};
    Point r_end{r, !r_open};
    if (!(l_end < r_end)) return;

    if (_st.contains(l_end))
      _st.erase(l_end);
    else
      _st.insert(l_end);
    if (_st.contains(r_end))
      _st.erase(r_end);
    else
      _st.insert(r_end);
  };

  /**
   * @brief 点 x の色を取得する。
   * @return 黒なら true、白なら false
   *
   * ### 計算量
   * O(log N)
   */
  bool get(const T& x) const {
    return (_st.lower_bound({x, true}) - _st.begin()) % 2;
  };
  bool operator[](const T& x) const {
    return get(x);
  };
  /**
   * @brief 点 x を含む最大の単色区間を取得する。
   * @details
   * 区間の端点には管理範囲の番兵を使用する場合がある。
   *
   * ### 計算量
   * O(log N)
   */
  IntervalState get_interval_state(T x) const {
    Point l_end, r_end;
    auto r_it = _st.lower_bound({x, true});
    if (r_it == _st.begin())
      l_end = {INTERVAL_MIN, false};
    else
      l_end = *(std::prev(r_it));

    if (r_it == _st.end())
      r_end = {INTERVAL_MAX, true};
    else
      r_end = *r_it;

    return {l_end, r_end, get(x)};
  };

  /**
   * @brief 指定区間を単色区間に分割して取得する。
   * @details
   * 区間を左から順に列挙する。
   * 各区間は指定範囲内に切り詰められる。
   *
   * @param l 左端の座標
   * @param r 右端の座標
   * @param l_open 左端を含まない場合 true
   * @param r_open 右端を含まない場合 true
   * @return 単色区間の一覧
   *
   * ### 計算量
   * O((K+1) log N)
   */
  std::vector<IntervalState> get_intervals(T l, T r, bool l_open = false,
                                           bool r_open = true) const {
    std::vector<IntervalState> res;
    Point l_end{l, l_open};
    Point r_end{r, !r_open};
    if (!(l_end < r_end)) return res;
    Point prev = l_end;
    auto l_it = _st.upper_bound(l_end);
    auto r_it = _st.lower_bound(r_end);

    bool current_color = (l_it - _st.begin()) % 2;
    while (l_it != r_it) {
      res.push_back({prev, *l_it, current_color});
      prev = *l_it;
      ++l_it;
      current_color ^= true;
    }
    res.push_back({prev, r_end, current_color});
    return res;
  };
};