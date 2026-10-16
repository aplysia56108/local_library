#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

constexpr long long kNAndQMax = 500000;
constexpr long long kMod = 998244353;

class Random {
 private:
  std::mt19937_64 engine_;

 public:
  explicit Random(long long seed) : engine_(seed) {}

  long long uniform(long long l, long long r) {
    std::uniform_int_distribution<long long> dist(l, r);
    return dist(engine_);
  }

  std::pair<long long, long long> uniform_pair(long long l, long long r) {
    // l <= first < second <= r
    long long a = uniform(l, r);
    long long b = uniform(l, r - 1);

    if (b >= a) ++b;
    if (a > b) std::swap(a, b);

    return {a, b};
  }
};

int main(int argc, char** argv) {
  const long long seed = argc >= 2 ? std::atoll(argv[1]) : 0;
  Random gen(seed);

  const long long n = gen.uniform(kNAndQMax - 1000, kNAndQMax);
  const long long q = gen.uniform(kNAndQMax - 1000, kNAndQMax);

  std::cout << n << ' ' << q << '\n';

  std::vector<long long> a(n);
  for (auto& x : a) {
    x = gen.uniform(0, kMod - 1);
  }

  for (long long i = 0; i < n; ++i) {
    if (i) std::cout << ' ';
    std::cout << a[i];
  }
  std::cout << '\n';

  long long len = n;

  for (long long query = 0; query < q; ++query) {
    long long t = 0;

    if (len > 0) {
      t = gen.uniform(0, 4);
    }

    if (t == 0) {
      const long long i = gen.uniform(0, len);
      const long long x = gen.uniform(0, kMod - 1);

      std::cout << t << ' ' << i << ' ' << x << '\n';
      ++len;
    } else if (t == 1) {
      const long long i = gen.uniform(0, len - 1);

      std::cout << t << ' ' << i << '\n';
      --len;
    } else {
      auto [l, r] = gen.uniform_pair(0, len);

      if (gen.uniform(0, 9) == 0) l = 0;
      if (gen.uniform(0, 9) == 0) r = len;

      if (t == 3) {
        const long long b = gen.uniform(0, kMod - 1);
        const long long c = gen.uniform(0, kMod - 1);

        std::cout << t << ' ' << l << ' ' << r << ' ' << b << ' ' << c << '\n';
      } else {
        std::cout << t << ' ' << l << ' ' << r << '\n';
      }
    }
  }
}