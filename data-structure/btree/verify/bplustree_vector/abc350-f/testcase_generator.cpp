#include <iostream>
#include <string>

int main() {
  constexpr int N = 500000;
  constexpr int Pairs = N / 4;

  std::string s;
  s.reserve(N);

  // (a(b(c(d(... の形で深くネスト
  for (int i = 0; i < Pairs; ++i) {
    s += '(';
    s += 'a' + i % 26;
  }

  // 残りの文字を最深部に入れる
  for (int i = 0; i < N - Pairs * 3; ++i) {
    s += 'a' + i % 26;
  }

  // 全部閉じる
  for (int i = 0; i < Pairs; ++i) {
    s += ')';
  }

  std::cout << s << '\n';
}