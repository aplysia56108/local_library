#include "utility/cc.hpp"

#include <bits/stdc++.h>

void test_coordinate_compression() {
  // 1. 基本的な動作テスト (重複あり・ランダム順)
  std::vector<long long> a = {100, 10, 50, 10, 100, 0};
  CC<long long> cc(a);  // コンストラクタで自走 init()

  // 圧縮後ユニーク要素: {0, 10, 50, 100} -> size = 4
  assert(cc.size() == 4);

  // 値 -> インデックス
  assert(cc(0) == 0);
  assert(cc(10) == 1);
  assert(cc(50) == 2);
  assert(cc(100) == 3);

  // インデックス -> 元の値
  assert(cc[0] == 0);
  assert(cc[1] == 10);
  assert(cc[2] == 50);
  assert(cc[3] == 100);

  // 2. 逐次追加 (add -> init) のテスト
  CC<int> cc2;
  cc2.add(30);
  cc2.add(10);
  cc2.add({20, 30});
  cc2.init();  // 明示的初期化

  assert(cc2.size() == 3);  // {10, 20, 30}
  assert(cc2(10) == 0);
  assert(cc2(20) == 1);
  assert(cc2(30) == 2);

  // 3. const 参照経由の呼び出しテスト
  const auto& const_cc = cc2;
  assert(const_cc(20) == 1);
  assert(const_cc[1] == 20);

  std::cout << "CC library tests passed successfully!" << std::endl;
}

int main() {
  test_coordinate_compression();
}