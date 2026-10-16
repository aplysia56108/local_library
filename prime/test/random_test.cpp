#include <bits/stdc++.h>
#define int long long
#include "prime/prime.hpp"
using namespace std;
signed main() {
  mt19937_64 rng(123456);
  Prime p;
  for (long long t = 0; t < 3000; t++) {
    long long n = rng() % 1000000000000LL + 1;
    auto pf = p.factorize(n);
    __int128 v = 1;
    for (auto [q, e] : pf) {
      assert(MillerRabin::is_prime(q));
      for (long long j = 0; j < e; j++) v *= q;
    }
    assert(v == n);
    assert(p.is_prime(n) ==
           (pf.size() == 1 && pf[0].first == n && pf[0].second == 1));
  }
  for (long long t = 0; t < 400; t++) {
    long long a = rng() % 1000000000LL + 1, b = rng() % 1000000000LL + 1;
    auto pf = p.factorize(a * b);
    __int128 v = 1;
    for (auto [q, e] : pf) {
      assert(MillerRabin::is_prime(q));
      for (long long j = 0; j < e; j++) v *= q;
    }
    assert(v == (__int128)a * b);
  }
  cerr << "RANDOM PASSED\n";
}
