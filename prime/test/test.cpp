#include <bits/stdc++.h>
#define int long long
#include "prime/prime.hpp"
using namespace std;
signed main() {
  Prime p;
  vector<bool> isp(100001, true);
  isp[0] = isp[1] = false;
  for (long long i = 2; i * i <= 100000; i++)
    if (isp[i])
      for (long long j = i * i; j <= 100000; j += i) isp[j] = false;
  for (long long n = 1; n <= 5000; n++) {
    assert(p.is_prime(n) == isp[n]);
    auto pf = p.factorize(n);
    long long x = 1;
    for (auto [q, e] : pf) {
      assert(isp[q]);
      while (e--) x *= q;
    }
    assert(x == n);
    auto ds = p.divisors(n);
    sort(ds.begin(), ds.end());
    vector<long long> expected;
    for (long long d = 1; d <= n; d++)
      if (n % d == 0) expected.push_back(d);
    assert(ds == expected);
  }
  for (long long n : {1000000007LL, 1000000009LL, 1000000016000000063LL,
                      2305843009213693951LL, 9223372036854775807LL,
                      3215031751LL, 341550071728321LL}) {
    auto pf = p.factorize(n);
    __int128 x = 1;
    for (auto [q, e] : pf) {
      assert(MillerRabin::is_prime(q));
      while (e--) x *= q;
    }
    assert(x == n);
    cerr << n << ": ";
    for (auto [q, e] : pf) cerr << q << "^" << e << " ";
    cerr << "\n";
  }
  cerr << "ALL PASSED\n";
}
