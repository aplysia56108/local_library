#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

// Adjust to the actual location of the library header.
#include "data-structure/other/interval-set.hpp"

using Set = IntervalSet<long long>;
using State = Set::IntervalState;
using Point = Set::Point;

bool same_point(Point a, Point b) {
  return a.x == b.x && a.plus_eps == b.plus_eps;
}

bool same_state(const State& a, const State& b) {
  return same_point(a.l_end, b.l_end) && same_point(a.r_end, b.r_end) &&
         a.color == b.color;
}

bool same_states(const std::vector<State>& a, const std::vector<State>& b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (!same_state(a[i], b[i])) return false;
  }
  return true;
}

// Samples are integer and half-integer points represented by twice their value.
bool contains(long long s2, long long l, long long r, bool l_open,
              bool r_open) {
  return (2 * l < s2 || (2 * l == s2 && !l_open)) &&
         (s2 < 2 * r || (s2 == 2 * r && !r_open));
}

bool contains_state(long long s2, const State& st) {
  return contains(s2, st.l_end.x, st.r_end.x, st.l_end.plus_eps,
                  !st.r_end.plus_eps);
}

void fixed_tests() {
  Set st;
  assert(!st.get(0));
  assert(st.get_intervals(2, 2).empty());  // [2,2) is empty
  assert(st.overwrite(true, 2, 2).empty());

  st.overwrite(true, 2, 2, false, false);  // [2,2]
  assert(st.get(2));
  assert(!st.get(1) && !st.get(3));
  auto center = st.get_interval_state(2);
  assert(same_point(center.l_end, {2, false}));
  assert(same_point(center.r_end, {2, true}));
  assert(center.color);
  assert(center.to_int_interval() == std::pair(2LL, 2LL));

  st.flip(2, 2, false, false);
  assert(!st.get(2));
  st.flip(2, 2, false, false);
  assert(st.get(2));
  auto changed = st.overwrite(false, 2, 2, false, false);
  assert(changed.size() == 1 && changed[0].color);
  assert(!st.get(2));
  assert(st.overwrite(false, 2, 2, false, false).empty());

  st.overwrite(true, 1, 5);  // [1,5)
  assert(st.get(1) && st.get(4) && !st.get(5));
  st.overwrite(true, 5, 7);  // joins [1,5) and [5,7)
  auto merged = st.get_interval_state(5);
  assert(same_point(merged.l_end, {1, false}));
  assert(same_point(merged.r_end, {7, false}));
  st.overwrite(false, 3, 4);  // make a white hole
  assert(st.get(2) && !st.get(3) && st.get(4));
  st.flip(3, 4);  // fill the hole
  assert(st.get(3));

  State closed{{2, false}, {5, true}, true};
  State open{{2, true}, {5, false}, true};
  State left_open{{2, true}, {5, true}, true};
  State right_open{{2, false}, {5, false}, true};
  assert(closed.to_int_interval() == std::pair(2LL, 5LL));
  assert(open.to_int_interval() == std::pair(3LL, 4LL));
  assert(left_open.to_int_interval() == std::pair(3LL, 5LL));
  assert(right_open.to_int_interval() == std::pair(2LL, 4LL));
  assert((State{{2, true}, {3, false}, false}.to_int_interval() ==
          std::pair(3LL, 2LL)));  // (2,3) contains no integer

  // A point interval and its boundary behavior.
  Set point;
  point.overwrite(true, 4, 4, false, false);
  assert(same_states(point.get_intervals(4, 4, false, false),
                     {State{{4, false}, {4, true}, true}}));
  assert(same_point(point.get_interval_state(4).l_end, {4, false}));
  assert(same_point(point.get_interval_state(4).r_end, {4, true}));

  // Open endpoints are not included.
  Set open_set;
  open_set.overwrite(true, 2, 5, true, true);
  assert(!open_set.get(2) && open_set.get(3) && open_set.get(4));
  assert(!open_set.get(5));
  assert(same_point(open_set.get_interval_state(3).l_end, {2, true}));
  assert(same_point(open_set.get_interval_state(3).r_end, {5, false}));
}

void double_tests() {
  using DoubleSet = IntervalSet<double>;
  using DoubleState = DoubleSet::IntervalState;
  auto check = [](DoubleState s, long long l, long long r) {
    assert(s.to_int_interval() == std::make_pair(l, r));
  };
  check({{2.2, false}, {5.8, true}, true}, 3, 5);
  check({{2.0, true}, {5.0, false}, true}, 3, 4);
  check({{2.0, false}, {5.0, false}, true}, 2, 4);
  check({{-2.5, true}, {2.5, true}, true}, -2, 2);
  check({{-2.0, true}, {-1.0, false}, true}, -1, -2);

  DoubleSet st;
  st.overwrite(true, 1.5, 3.25, true, false);  // (1.5, 3.25]
  assert(!st.get(1.5) && st.get(2.0) && st.get(3.25));
  assert(!st.get(4.0));
  st.flip(2.0, 2.0, false, false);  // turn only x=2.0 white
  assert(!st.get(2.0) && st.get(2.5));
  check(st.get_interval_state(2.5), 3, 3);
}

void random_tests() {
  constexpr long long LOW = -4;
  constexpr long long HIGH = 4;
  constexpr long long SLOW = -12;  // -6.0, outside all updates
  constexpr long long SHIGH = 12;  // +6.0, outside all updates
  std::mt19937_64 rng(20261019);
  auto rnd = [&](long long lo, long long hi) {
    return std::uniform_int_distribution<long long>(lo, hi)(rng);
  };

  for (long long trial = 0; trial < 100; ++trial) {
    Set st;
    std::vector<bool> brute(SHIGH - SLOW + 1, false);
    auto old_color = [&](long long s2) -> bool {
      return brute[static_cast<std::size_t>(s2 - SLOW)];
    };

    for (long long iter = 0; iter < 500; ++iter) {
      long long l = rnd(LOW, HIGH);
      long long r = rnd(LOW, HIGH);
      if (l > r) std::swap(l, r);
      bool l_open = rnd(0, 1);
      bool r_open = rnd(0, 1);
      bool color = rnd(0, 1);
      long long op = rnd(0, 2);
      auto before = st.get_intervals(l, r, l_open, r_open);
      std::vector<State> changed_expected;
      for (const State& seg : before) {
        if (seg.color != color) changed_expected.push_back(seg);
      }

      if (op == 0) {
        auto changed = st.overwrite(color, l, r, l_open, r_open);
        assert(same_states(changed, changed_expected));
      } else if (op == 1) {
        st.flip(l, r, l_open, r_open);
      } else {
        // Read-only get_intervals operation
        auto actual = st.get_intervals(l, r, l_open, r_open);
        assert(same_states(actual, before));
      }

      for (long long s2 = SLOW; s2 <= SHIGH; ++s2) {
        if (contains(s2, l, r, l_open, r_open)) {
          auto idx = static_cast<std::size_t>(s2 - SLOW);
          if (op == 0) brute[idx] = color;
          if (op == 1) brute[idx] = !brute[idx];
        }
      }

      // Check colors at all integer points (including outside update domain).
      for (long long x = -6; x <= 6; ++x) {
        assert(st.get(x) == old_color(2 * x));
        assert(st[x] == old_color(2 * x));
        auto state = st.get_interval_state(x);
        assert(state.color == old_color(2 * x));
        assert(contains_state(2 * x, state));
      }

      // Check the exact get_intervals decomposition and its colors on a dense
      // grid.
      auto all = st.get_intervals(-6, 6, false, false);
      assert(!all.empty());
      assert(same_point(all.front().l_end, {-6, false}));
      assert(same_point(all.back().r_end, {6, true}));
      for (std::size_t j = 1; j < all.size(); ++j) {
        assert(same_point(all[j - 1].r_end, all[j].l_end));
        assert(all[j - 1].color != all[j].color);
      }
      for (long long s2 = SLOW; s2 <= SHIGH; ++s2) {
        long long hits = 0;
        for (const auto& seg : all) {
          if (contains_state(s2, seg)) {
            ++hits;
            assert(seg.color == old_color(s2));
          }
        }
        assert(hits == 1);
      }
    }
  }
}

int main() {
  fixed_tests();
  double_tests();
  random_tests();
  std::cout << "All IntervalSet tests passed\n";
}