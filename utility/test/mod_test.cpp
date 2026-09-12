#include <atcoder/all>

using mint = atcoder::modint998244353;
using dmint = atcoder::modint;

#include "utility/mod.hpp"

Binomial<dmint> bm_dmint;
ModPowTable<dmint> pow10_dmint(10);

void test_binomial() {
  // 1. long long 版のテスト (mod = 998244353)
  long long MOD = 998244353;
  Binomial<long long> bm(MOD);

  assert(bm.C(5, 2) == 10);
  assert(bm.P(5, 2) == 20);
  assert(bm.H(5, 2) == 15);  // 6C2

  // 動的拡張のテスト (5 -> 20)
  assert(bm.C(20, 10) == 184756);
  assert(bm.C(100, 10) == 17310309456440 % MOD);

  // 範囲外の例外挙動テスト
  assert(bm.C(5, 10) == 0);
  assert(bm.C(-1, 2) == 0);

  // 2. atcoder::modint998244353 版のテスト
  Binomial<mint> bm_mint(998244353);
  assert(bm_mint.C(10, 3).val() == 120);
  assert(bm_mint.fact(5).val() == 120);
  assert(bm_mint.C(100, 10).val() == bm.C(100, 10));

  // 3. atcoder::modint998244353 版のテスト
  MOD = 1000000007;
  dmint::set_mod(MOD);  // mod を 10^9 + 7 に設定
  assert(bm_dmint.C(10, 3).val() == 120);
  assert(bm_dmint.fact(5).val() == 120);
  assert(bm_dmint.C(100, 10) == 17310309456440 % MOD);
}

void test_modpow() {
  // 2^n mod 998244353
  ModPowTable<long long> pow2(2, 998244353);

  assert(pow2[0] == 1);
  assert(pow2[1] == 2);
  assert(pow2[10] == 1024);
  assert(pow2[30] == 1073741824 % 998244353);

  // mint 版の累乗テーブル
  ModPowTable<mint> pow10(10);
  assert(pow10[3].val() == 1000);
}

void test_modpow_table_mint() {
  // 1. static_modint (mod = 998244353) のテスト
  ModPowTable<mint> pow2_mint(2);

  // 通常範囲のテスト [0, 10]
  assert(pow2_mint[0] == 1);
  assert(pow2_mint[1] == 2);
  assert(pow2_mint[10] == 1024);

  // 動的拡張のテスト
  assert(pow2_mint[20] == mint(2).pow(20));

  // 巨大な指数のテスト (i > 2e6 -> base.pow(i) が呼ばれる)
  long long big_exp = 3000000;
  assert(pow2_mint[big_exp] == mint(2).pow(big_exp));

  // 2. dynamic_modint (実行時に mod を指定する型) のテスト
  dmint::set_mod(1000000007);  // mod を 10^9 + 7 に設定

  // 通常範囲のテスト
  assert(pow10_dmint[0] == 1);
  assert(pow10_dmint[3] == 1000);
  assert(pow10_dmint[9] == 1000000000 % 1000000007);

  // 巨大な指数のテスト (i > 2e6)
  assert(pow10_dmint[big_exp] == dmint(10).pow(big_exp));

  std::cout << "ModPowTable (mint & dynamic_modint) tests passed successfully!"
            << std::endl;
}

int main() {
  test_binomial();
  test_modpow();
  test_modpow_table_mint();
  std::cout << "Mod functions library tests passed successfully!" << std::endl;
  return 0;
}