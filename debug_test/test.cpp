#include <bits/stdc++.h>

#include <array>
#include <debug.hpp>
#include <deque>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>

// 1. << 非対応の自作構造体 (Unprintable)
struct UnprintableStruct {
  int id;
  std::string name;
};

// 2. << 対応の自作構造体
struct PrintableStruct {
  int x, y;
  friend std::ostream& operator<<(std::ostream& os, const PrintableStruct& p) {
    return os << "(" << p.x << ", " << p.y << ")";
  }
};

// lazy_segtree 用の定義例 (Range Add Range Sum)
struct S {
  long long value;
  int size;
};
std::ostream& operator<<(std::ostream& os, const S& x) {
  return os << "{" << x.value << ", " << x.size << "}";
}
using F = long long;
S op(S a, S b) {
  return {a.value + b.value, a.size + b.size};
}
S e() {
  return {0, 0};
}
S mapping(F f, S x) {
  return {x.value + f * x.size, x.size};
}
F composition(F f, F g) {
  return f + g;
}
F id() {
  return 0;
}

void test_segtree() {
  std::vector<S> v(5, {1, 1});
  atcoder::segtree<S, op, e> seg(v);
  seg.set(2, {3, 1});
  seg.set(4, {5, 1});

  debug(seg);
}

void test_lazy_segtree() {
  std::vector<S> v(5, {1, 1});
  atcoder::lazy_segtree<S, op, e, F, mapping, composition, id> seg(v);
  seg.apply(1, 3, 5);

  debug(seg);
}

void test_fenwick_tree() {
  atcoder::fenwick_tree<long long> bit(5);
  bit.add(0, 10);
  bit.add(2, 20);

  debug(bit);
}

void test_dsu() {
  atcoder::dsu uf(5);
  uf.merge(0, 1);
  uf.merge(2, 3);
  uf.merge(3, 4);

  debug(uf);
}

int main() {
  std::cout << "=== 1. 基本型・特殊数値・文字列 ===" << std::endl;
  int a = 42;
  double b = 3.14159;
  std::string s = "Hello C++23";
  const char* cstr = "C-style string";
  __int128_t huge = 1234567890123456789LL;
  debug(a);
  debug(a, b, s, cstr, huge);

  std::cout
      << "\n=== 2. 標準シーケンスコンテナ (vector, deque, array, C配列) ==="
      << std::endl;
  std::vector<int> vec = {1, 2, 3, 4, 5};
  std::deque<int> deq(vec.begin(), vec.end());
  std::array<int, 4> arr = {10, 20, 30, 40};
  int raw_arr[3] = {7, 8, 9};
  debug(vec);
  debug(deq);
  debug(arr);
  debug(raw_arr);

  std::cout << "\n=== 3. 順序付き・順序無し集合 & マップ ===" << std::endl;
  std::set<std::string> st = {"apple", "banana"};
  std::multiset<int> mst = {3, 1, 3, 2};
  std::unordered_set<int> ust = {100, 200, 300};
  std::map<std::string, int> mp = {{"one", 1}, {"two", 2}};
  std::unordered_map<int, std::string> ump = {{10, "ten"}, {20, "twenty"}};
  debug(st, mst);
  debug(ust);
  debug(mp);
  debug(ump);

  std::cout
      << "\n=== 4. アダプター系コンテナ (queue, priority_queue, stack) ==="
      << std::endl;
  std::queue<int> q;
  q.push(10);
  q.push(20);
  q.push(30);

  std::stack<std::string> sk;
  sk.push("bottom");
  sk.push("middle");
  sk.push("top");

  // 降順 priority_queue (デフォルト: 大きい順)
  std::priority_queue<int> pq_desc;
  pq_desc.push(30);
  pq_desc.push(10);
  pq_desc.push(50);

  // 昇順 priority_queue (小さい順)
  std::priority_queue<int, std::vector<int>, std::greater<int>> pq_asc;
  pq_asc.push(30);
  pq_asc.push(10);
  pq_asc.push(50);

  debug(q);
  debug(sk);
  debug(pq_desc);
  debug(pq_asc);

  std::cout << "\n=== 5. pair と tuple ===" << std::endl;
  std::pair<int, std::string> p = {10, "orbit"};
  std::tuple<int, double, std::string> tup = {100, 2.718, "tuple_test"};
  debug(p);
  debug(tup);

  std::cout << "\n=== 6. optional と variant ===" << std::endl;
  std::optional<int> opt_has = 777;
  std::optional<int> opt_empty = std::nullopt;
  std::variant<int, std::string, std::vector<int>> var1 = "variant_string";
  std::variant<int, std::string, std::vector<int>> var2 =
      std::vector<int>{10, 20};
  debug(opt_has, opt_empty);
  debug(var1, var2);

  std::cout << "\n=== 7. ネストされた複雑なデータ構造 & DPテーブル ==="
            << std::endl;
  std::map<std::string, std::vector<std::pair<int, int>>> complex_map = {
      {"path1", {{0, 0}, {1, 2}}}, {"path2", {{3, 4}}}};
  std::vector<std::optional<std::tuple<int, std::string>>> complex_vec = {
      std::make_tuple(1, "A"), std::nullopt, std::make_tuple(2, "B")};
  std::vector<std::vector<int>> dp_int = {
      {0, 3, 500, -1}, {10, 15000, 0, 99}, {7, 8, 12, 1}};
  std::vector<std::vector<double>> dp_double = {{1.00, 0.50, 0.25},
                                                {0.00, 0.50, 0.75}};

  debug(complex_map);
  debug(complex_vec);
  debug(dp_int);
  debug(dp_double);

  std::cout << "\n=== 8. 出力対応 / 非対応の構造体 ===" << std::endl;
  UnprintableStruct unprintable{101, "Test"};
  PrintableStruct printable{5, 9};
  std::vector<UnprintableStruct> vec_unprintable = {{1, "A"}, {2, "B"}};

  debug(unprintable, printable);
  debug(vec_unprintable);

  std::cout << "\n=== 9. AtCoder Library (ACL) データ構造 ===" << std::endl;
  test_segtree();
  test_lazy_segtree();
  test_fenwick_tree();
  test_dsu();

  return 0;
}