#include <bits/stdc++.h>

using namespace std::string_literals;

#include <atcoder/modint>
// ライブラリコードをここにインクルードまたは配置

#include "utility/util_functions.hpp"

void test_vector_pair_operators() {
  // vector インクリメント/デクリメント
  std::vector<int> v = {1, 2, 3};
  ++v;
  assert((v == std::vector<int>{2, 3, 4}));
  v++;
  assert((v == std::vector<int>{3, 4, 5}));
  --v;
  assert((v == std::vector<int>{2, 3, 4}));
  v--;
  assert((v == std::vector<int>{1, 2, 3}));

  // pair 演算
  std::pair<int, int> p1 = {1, 2}, p2 = {3, 4};
  assert((p1 + p2 == std::pair<int, int>{4, 6}));
  assert((p2 - p1 == std::pair<int, int>{2, 2}));
  p1 += p2;
  assert((p1 == std::pair<int, int>{4, 6}));
  p1 -= p2;
  assert((p1 == std::pair<int, int>{1, 2}));

  std::cout << "vector pair operators tests passed successfully!" << std::endl;
}

void test_graph_and_counting() {
  // read_edges テスト (cin のダミー入力)
  std::stringstream ss("1 2\n2 3\n");
  std::streambuf* cin_buf = std::cin.rdbuf(ss.rdbuf());

  std::vector<std::vector<long long>> g(3);
  read_edges(g, 2, true);
  std::cin.rdbuf(cin_buf);  // cin を元に戻す

  assert((g[0] == std::vector<long long>{1}));
  assert((g[1] == std::vector<long long>{0, 2}));

  // count_freq & count_map
  std::vector<long long> nums = {1, 3, 1, 3, 3};
  auto freq = count_freq(nums);
  assert(freq[1] == 2 && freq[3] == 3);

  auto cmap = count_map(nums);
  assert(cmap[1] == 2 && cmap[3] == 3);

  // make_iota
  assert((make_iota(4, 2) == std::vector<long long>{2, 3, 4, 5}));

  std::cout << "graph and counting tests passed successfully!" << std::endl;
}

void test_min_max_utils() {
  std::vector<long long> v = {3, 1, 4, 1, 5, 9, 2};
  assert(maxv(v) == 9);
  assert(minv(v) == 1);
  assert(argmax(v) == 5);
  assert(argmin(v) == 1);

  // max/min オーバーロード (int と long long)
  int a = 5;
  long long b = 10LL;
  assert(max(a, b) == 10LL);
  assert(min(b, a) == 5LL);

  // chmax / chmin
  int val = 5;
  assert(chmax(val, 10));
  assert(val == 10);
  assert(!chmax(val, 8));
  assert(chmin(val, 3));
  assert(val == 3);

  std::cout << "min max utils tests passed successfully!" << std::endl;
}

// sumv, maxv, minvが様々なコンテナ、要素の型で動作するかの追加テスト
void test_sumv_maxv_minv() {
  using mint = atcoder::modint998244353;
  // 1. vector × long long
  {
    std::vector<long long> v = {3, 1, 4, 1, 5, 9, 2, 6};
    assert(sumv(v) == 31LL);
    assert(maxv(v) == 9LL);
    assert(minv(v) == 1LL);
  }

  // 2. std::array × int
  {
    std::array<int, 5> a = {10, -20, 30, 0, 5};
    assert(sumv(a) == 25);
    assert(maxv(a) == 30);
    assert(minv(a) == -20);
  }

  // 3. std::deque × mint (atcoder::modint998244353)
  {
    std::deque<mint> d = {100, 200,
                          998244300};  // 100 + 200 + 998244300 = 998244600
    assert(sumv(d) == mint(247));      // 998244600 % 998244353 = 247
  }

  // 4. std::vector × std::pair<long long, long long>
  {
    std::vector<std::pair<long long, long long>> vp = {
        {1, 10}, {3, 20}, {2, 30}};
    assert(sumv(vp) == (std::pair<long long, long long>{6, 60}));
    assert(maxv(vp) == (std::pair<long long, long long>{3, 20}));
    assert(minv(vp) == (std::pair<long long, long long>{1, 10}));
  }

  // 5. std::set × std::string (辞書順比較)
  {
    std::set<std::string> s = {"apple", "banana", "cherry"};
    assert(sumv(s) == "applebananacherry");
    assert(maxv(s) == "cherry");
    assert(minv(s) == "apple");
  }

  // 6. std::map × std::pair (key-valueペアの集計)
  {
    std::map<int, long long> m = {{1, 100}, {2, 200}, {3, -50}};
    assert(sumv(m) == 250LL);
    assert(minv(m) == -50LL);
    assert(maxv(m) == 200LL);
  }

  // 3. std::deque × pair<mint, mint> (atcoder::modint)
  {
    using dmint = atcoder::modint;
    dmint::set_mod(1000000007);
    std::deque<std::pair<dmint, dmint>> d = {
        {100, 200}, {200, 300}, {1000000000, 1000000000}};
    auto res = sumv(d);
    assert(res.first == dmint(293));
    assert(res.second == dmint(493));
  }

  // 7. 空コンテナ（sumv の値初期化挙動）
  {
    std::vector<long long> empty_v;
    assert(sumv(empty_v) == 0LL);

    std::vector<mint> empty_mint;
    assert(sumv(empty_mint) == mint(0));

    std::vector<std::pair<int, int>> empty_vp;
    assert(sumv(empty_vp) == (std::pair<int, int>{0, 0}));
  }

  std::cout << "sumv maxv minv tests passed successfully!" << std::endl;
}

void test_in_range() {
  // 半開区間 [3, 8)
  assert(in_range(3, 3, 8) == true);   // 最小境界値 (含む)
  assert(in_range(3, 5, 8) == true);   // 範囲内
  assert(in_range(3, 7, 8) == true);   // 最大値直前
  assert(in_range(3, 8, 8) == false);  // 最大境界値 (含まない)
  assert(in_range(3, 2, 8) == false);  // 下限外
  assert(in_range(3, 9, 8) == false);  // 上限外

  // 負の数を含むケース [-5, 5)
  assert(in_range(-5, -5, 5) == true);
  assert(in_range(-5, 0, 5) == true);
  assert(in_range(-5, 5, 5) == false);

  // 閉区間 [3, 8] のテスト
  assert(in_range_inclusive(3, 8, 8) == true);   // 8 を含む
  assert(in_range_inclusive(3, 9, 8) == false);  // 9 は含まない

  std::cout << "in_range tests passed successfully!" << std::endl;
}

void test_conversions() {
  assert(ctoi('7') == 7);
  assert(atoi('c') == 2);

  std::string s_num = "123";
  assert((ctoi(s_num) == std::vector<long long>{1, 2, 3}));

  std::string s_alpha = "abc";
  assert((atoi(s_alpha) == std::vector<long long>{0, 1, 2}));

  std::vector<long long> v_num = {1, 2, 3};
  assert(itoc(v_num) == "123");
  assert(itoa(v_num) == "bcd");
  {
    std::vector<std::string> s1 = {"ab", "cd"};
    std::vector<std::string> s2 = {"01", "23"};
    std::vector<std::vector<long long>> grid = {{0, 1}, {2, 3}};
    assert(atoi(s1) == grid);
    assert(ctoi(s2) == grid);
  }
  std::vector<std::vector<long long>> grid = {{0, 1}, {2, 3}};
  std::vector<std::string> expected = {"ab", "cd"};
  assert(itoa(grid) == expected);

  std::cout << "conversions tests passed successfully!" << std::endl;
}

void test_math_and_bit() {
  // 128bit abs
  __int128_t x = -1;
  for (int i = 0; i < 100; i++) x *= 2;
  assert(abs(x) == -x);
  int y = -100;
  assert(abs(y) == 100);

  // ispow2 & countDigits
  assert(ispow2(8));
  assert(!ispow2(6));
  assert(countDigits(12345) == 5);
  assert(countDigits(-123) == 3);

  // lower_idx & upper_idx
  std::vector<int> sorted_v = {10, 20, 20, 30, 40};
  assert(lower_idx(sorted_v, 20) == 1);
  assert(upper_idx(sorted_v, 20) == 3);

  // ビット演算
  long long msk = 0;
  msk = set_bit(1, msk);  // 2 (0b0010)
  msk = set_bit(3, msk);  // 10 (0b1010)
  assert(msk == 10);
  assert(get_bit(msk, 3) == 1);
  assert(get_bit(msk, 2) == 0);

  msk = unset_bit(1, msk);  // 8 (0b1000)
  assert(msk == 8);

  msk = flip_bit(0, msk);  // 9 (0b1001)
  assert(msk == 9);

  assert(pop_count(9) == 2);
  assert(top_bit(9) == 3);     // MSB at index 3
  assert(bottom_bit(9) == 0);  // LSB at index 0
  assert(mask(3) == 7);        // 0b0111

  std::cout << "math and bit tests passed successfully!" << std::endl;
}

void test_tuple_at() {
  std::tuple<int, std::string, double> t = {100, "hello", 3.14};

  auto val0 = tuple_at(t, 0);
  assert(std::get<int>(val0) == 100);

  auto val1 = tuple_at(t, 1);
  assert(std::get<std::string>(val1) == "hello");

  auto val2 = tuple_at(t, 2);
  assert(std::get<double>(val2) == 3.14);

  std::cout << "tuple_at tests passed successfully!" << std::endl;
}

void test_make_vector() {
  // 1. 基本的な2次元 vector (サイズ 3 x 4, 初期値 -1)
  auto grid2d = make_vector(3, 4, -1);
  assert(grid2d.size() == 3);
  assert(grid2d[0].size() == 4);
  assert(grid2d[2][3] == -1);

  // 2. 初期値省略時のデフォルト値テスト (0 初期化)
  auto default_grid = make_vector(2, 5, signed(0));
  assert(default_grid[1][4] == signed(0));

  {
#define int long long
    // 1. 初期値省略時のデフォルト値テスト (0 初期化)
    auto default_grid = make_vector(2, 5, 0);
    assert(default_grid[1][4] == 0LL);
    // 2. 型のテスト (vector<vector<long long>> になっているか)
    static_assert(std::is_same_v<decltype(default_grid),
                                 std::vector<std::vector<long long>>>);
#undef int
  }

  // 3. 3次元以上の多次元 vector (サイズ 2 x 3 x 4, 初期値 1.5)
  auto grid3d = make_vector(2, 3, 4, 1.5);
  assert(grid3d.size() == 2);
  assert(grid3d[0].size() == 3);
  assert(grid3d[0][0].size() == 4);
  assert(grid3d[1][2][3] == 1.5);

  // 4. 文字列型 (std::string) の多次元 vector
  auto string_grid = make_vector(2, 2, "unvisited");
  assert(string_grid[0][1] == "unvisited");

  // 5. make_vec エイリアス（ショートカット）の動作検証
  auto short_vec = makev<int>(4, 2, 99);
  assert(short_vec.size() == 4);
  assert(short_vec[0].size() == 2);
  assert(short_vec[3][1] == 99);

  std::cout << "make_vector tests passed successfully!" << std::endl;
}

void test_sz() {
  // 1. std::vector
  std::vector<int> vec = {1, 2, 3, 4, 5};
  assert(sz(vec) == 5LL);
  static_assert(std::is_same_v<decltype(sz(vec)), long long>,
                "Return type must be long long");

  // 空のコンテナ
  std::vector<int> empty_vec;
  assert(sz(empty_vec) == 0LL);

  // 2. std::string
  std::string str = "hello";
  assert(sz(str) == 5LL);

  // 3. Cスタイル生配列
  int arr[] = {10, 20, 30, 40};
  assert(sz(arr) == 4LL);

  // 4. std::map などの他のSTLコンテナ
  std::map<int, int> mp = {{1, 10}, {2, 20}};
  assert(sz(mp) == 2LL);

  // 5. initializer_list
  assert(sz({1, 2, 3, 4, 5, 6}) == 6LL);

  std::cout << "sz() tests passed successfully!" << std::endl;
}

void test_read_column() {
  // 1. read_column2 および 異型(int, string)のテスト
  {
    std::string input_data = "1 apple\n2 banana\n3 cherry\n";
    std::stringstream ss(input_data);
    auto old_cin =
        std::cin.rdbuf(ss.rdbuf());  // std::cin の入力元を ss に切り替え

    auto [id, name] = read_column2<int, std::string>(3);

    assert(id.size() == 3);
    assert(name.size() == 3);
    assert(id[1] == 2);
    assert(name[1] == "banana");

    std::cin.rdbuf(old_cin);  // 元に戻す
  }

  // 2. read_column3 (デフォルト型 long long) のテスト
  {
    std::string input_data = "10 20 30\n40 50 60\n";
    std::stringstream ss(input_data);
    auto old_cin = std::cin.rdbuf(ss.rdbuf());

    auto [a, b, c] = read_column3(2);

    static_assert(std::is_same_v<decltype(a), std::vector<long long>>);
    assert(a[0] == 10 && b[0] == 20 && c[0] == 30);
    assert(a[1] == 40 && b[1] == 50 && c[1] == 60);

    std::cin.rdbuf(old_cin);
  }

  // 3. read_column4 のテスト
  {
    std::string input_data = "1 2 3 4\n5 6 7 8\n";
    std::stringstream ss(input_data);
    auto old_cin = std::cin.rdbuf(ss.rdbuf());

    auto [u, v, cost, id] = read_column4(2);

    assert(u.size() == 2);
    assert(u[1] == 5);
    assert(v[1] == 6);
    assert(cost[1] == 7);
    assert(id[1] == 8);

    std::cin.rdbuf(old_cin);
  }

  std::cout << "read column tests passed successfully!" << std::endl;
}

int main() {
  test_vector_pair_operators();
  test_graph_and_counting();
  test_min_max_utils();
  test_in_range();
  test_conversions();
  test_math_and_bit();
  test_tuple_at();
  test_make_vector();
  test_sz();
  test_sumv_maxv_minv();
  test_read_column();

  std::cout << "All tests passed successfully!" << std::endl;
  return 0;
}