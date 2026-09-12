#pragma once

#include <array>
#include <atcoder/all>
#include <cctype>
#include <deque>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utility/base_exchange.hpp"

// ==========================================
// 前方宣言 (Forward Declarations)
// ==========================================

// --- 基本型 / ACL ---
template <int m>
std::istream& operator>>(std::istream& is, atcoder::static_modint<m>& a);
template <int m>
std::ostream& operator<<(std::ostream& os, const atcoder::static_modint<m>& a);

inline std::ostream& operator<<(std::ostream& s, const __int128_t& p);
inline std::istream& operator>>(std::istream& is, __int128_t& v);

// --- pair ---
template <class T, class U>
std::istream& operator>>(std::istream& is, std::pair<T, U>& p);
template <class T, class U>
std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& p);

// --- 標準コンテナ ---
template <class T>
std::ostream& operator<<(std::ostream& s, const std::set<T>& P);
template <class T1, class T2>
std::ostream& operator<<(std::ostream& s, const std::map<T1, T2>& P);
template <class T>
std::ostream& operator<<(std::ostream& s, const std::multiset<T>& P);
template <class T>
std::ostream& operator<<(std::ostream& s, const std::unordered_set<T>& P);
template <class T1, class T2>
std::ostream& operator<<(std::ostream& s, const std::unordered_map<T1, T2>& P);

template <class T>
std::istream& operator>>(std::istream& is, std::vector<T>& v);
template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v);
template <class T>
std::ostream& operator<<(std::ostream& os,
                         const std::vector<std::vector<T>>& v);

template <class T, size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& a);

template <class... Args>
std::ostream& operator<<(std::ostream& os, const std::tuple<Args...>& t);

// --- アダプター系コンテナ ---
template <class T>
std::ostream& operator<<(std::ostream& os, std::queue<T> q);
template <class T>
std::ostream& operator<<(std::ostream& os, std::stack<T> q);
template <class T>
std::ostream& operator<<(std::ostream& os, const std::deque<T>& q);
template <class T>
std::ostream& operator<<(std::ostream& os, std::priority_queue<T> q);
template <class T>
std::ostream& operator<<(
    std::ostream& os,
    std::priority_queue<T, std::vector<T>, std::greater<T>> q);

// --- 出力ユーティリティ関数 ---
inline void out();
template <class T>
inline void out(const T& a);
template <class T, class... Ts>
inline void out(const T& a, const Ts&... b);

// ==========================================
// 実装 (Implementations)
// ==========================================

// --- AtCoder Modint 入出力 ---
template <int m>
inline std::istream& operator>>(std::istream& is,
                                atcoder::static_modint<m>& a) {
  long long v;
  is >> v;
  a = v;
  return is;
}

template <int m>
inline std::ostream& operator<<(std::ostream& os,
                                const atcoder::static_modint<m>& a) {
  return os << a.val();
}

// --- __int128_t 入出力 ---
inline std::ostream& operator<<(std::ostream& s, const __int128_t& p) {
  s << to_baseB(p);
  return s;
}

inline std::istream& operator>>(std::istream& is, __int128_t& v) {
  std::string s;
  is >> s;
  v = 0;
  bool neg = false;
  int idx = 0;
  if (!s.empty() && s[0] == '-') {
    neg = true;
    idx = 1;
  }
  for (; idx < (int)s.size(); idx++) {
    if (std::isdigit(s[idx])) {
      v = v * 10 + (s[idx] - '0');
    }
  }
  if (neg) v = -v;
  return is;
}

// --- pair 入出力 ---
template <class T, class U>
inline std::istream& operator>>(std::istream& is, std::pair<T, U>& p) {
  is >> p.first >> p.second;
  return is;
}

template <class T, class U>
inline std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& p) {
  os << p.first << " " << p.second;
  return os;
}

// --- 各種コンテナの出力 ---
template <class T>
inline std::ostream& operator<<(std::ostream& s, const std::set<T>& P) {
  for (const auto& it : P) s << it << " ";
  return s;
}

template <class T1, class T2>
inline std::ostream& operator<<(std::ostream& s, const std::map<T1, T2>& P) {
  s << '[';
  bool is_first = 1;
  for (const auto& [x, y] : P) {
    if (!is_first) s << ", ";
    s << "{" << x << "->" << y << "}";
    is_first = 0;
  }
  s << ']';
  return s;
}

template <class T>
inline std::ostream& operator<<(std::ostream& s, const std::multiset<T>& P) {
  for (const auto& it : P) s << it << " ";
  return s;
}

template <class T>
inline std::ostream& operator<<(std::ostream& s,
                                const std::unordered_set<T>& P) {
  for (const auto& it : P) s << it << " ";
  return s;
}

template <class T1, class T2>
inline std::ostream& operator<<(std::ostream& s,
                                const std::unordered_map<T1, T2>& P) {
  s << '[';
  bool is_first = 1;
  for (const auto& [x, y] : P) {
    if (!is_first) s << ", ";
    s << "{" << x << "->" << y << "}";
    is_first = 0;
  }
  s << ']';
  return s;
}

template <class T>
inline std::istream& operator>>(std::istream& is, std::vector<T>& v) {
  for (auto& e : v) is >> e;
  return is;
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
  for (size_t i = 0; i < v.size(); ++i) {
    os << v[i] << (i + 1 == v.size() ? "" : " ");
  }
  return os;
}

template <class T>
inline std::ostream& operator<<(std::ostream& os,
                                const std::vector<std::vector<T>>& v) {
  for (size_t i = 0; i < v.size(); ++i) {
    os << v[i] << (i + 1 == v.size() ? "" : "\n");
  }
  return os;
}

template <class T, size_t N>
inline std::ostream& operator<<(std::ostream& os, const std::array<T, N>& a) {
  for (size_t i = 0; i < N; ++i) {
    os << a[i] << (i + 1 == N ? "" : " ");
  }
  return os;
}

// --- tuple 出力 ---
template <size_t I = 0, class... Args>
inline typename std::enable_if<I == sizeof...(Args), void>::type print_tuple(
    std::ostream&, const std::tuple<Args...>&) {}

template <size_t I = 0, class... Args>
    inline typename std::enable_if <
    I<sizeof...(Args), void>::type print_tuple(std::ostream& os,
                                               const std::tuple<Args...>& t) {
  if (I != 0) os << " ";
  os << std::get<I>(t);
  print_tuple<I + 1, Args...>(os, t);
}

template <class... Args>
inline std::ostream& operator<<(std::ostream& os,
                                const std::tuple<Args...>& t) {
  print_tuple(os, t);
  return os;
}

// --- アダプター系コンテナ ---
template <class T>
inline std::ostream& operator<<(std::ostream& os, std::queue<T> q) {
  while (!q.empty()) {
    os << q.front() << ' ';
    q.pop();
  }
  return os;
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, std::stack<T> q) {
  while (!q.empty()) {
    os << q.top() << ' ';
    q.pop();
  }
  return os;
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const std::deque<T>& q) {
  for (const auto& e : q) os << e << ' ';
  return os;
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, std::priority_queue<T> q) {
  while (!q.empty()) {
    os << q.top() << ' ';
    q.pop();
  }
  return os;
}

template <class T>
inline std::ostream& operator<<(
    std::ostream& os,
    std::priority_queue<T, std::vector<T>, std::greater<T>> q) {
  while (!q.empty()) {
    os << q.top() << ' ';
    q.pop();
  }
  return os;
}

// --- 出力ユーティリティ関数 ---
inline void out() {
  std::cout << std::endl;
}

template <class T>
inline void out(const T& a) {
  std::cout << a << std::endl;
}

template <class T, class... Ts>
inline void out(const T& a, const Ts&... b) {
  std::cout << a;
  (..., (std::cout << ' ' << b));
  std::cout << std::endl;
}