#pragma once

#include <atcoder/all>
#include <concepts>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "utility/type_check.hpp"
#include "utility/base_exchange.hpp"

namespace io_impl {

using namespace type_check;

template <class T>
inline constexpr bool dependent_false = false;

// --- デバッグ出力クラス ---
class Printer {
 public:
  std::ostream& os;

  Printer(std::ostream& os_ = std::cout) : os(os_) {}

  template <typename T>
  void print(const T& val) {
    if constexpr (IOByVal<T>) {
      print(val.val());
    } else if constexpr (IsString<T>) {
      if constexpr (CanOstream<T>)
        os << val;
      else {
        static_assert(dependent_false<T>,
                      "io_impl::Printer: This type is not printable.");
      }
    } else if constexpr (IsAtCoderDSU<T>) {
      print_dsu(val);
    } else if constexpr (IsAtCoderFenwickTree<T>) {
      print_fenwick_tree(val);
    } else if constexpr (IsAtCoderSegtree<T>) {
      print_segtree(val);
    } else if constexpr (IsAtCoderLazySegtree<T>) {
      print_lazy_segtree(val);
    } else if constexpr (IsVector2D<T>) {
      print_vector2d(val);
    } else if constexpr (IsTuple<T>) {
      print_tuple(val);
    } else if constexpr (IsMapLike<T>) {
      print_map(val);
    } else if constexpr (IsPairLike<T>) {
      print_pair(val);
    } else if constexpr (IsContainerAdapter<T>) {
      print_adapter(val);
    } else if constexpr (IsOptional<T>) {
      print_optional(val);
    } else if constexpr (IsVariant<T>) {
      print_variant(val);
    } else if constexpr (IsPrintableRange<T>) {
      print_range(val);
    } else if constexpr (IsInt128<T>) {
      print_int128(val);
    } else if constexpr (CanOstream<T>) {
      os << val;
    } else {
      static_assert(dependent_false<T>,
                    "io_impl::Printer: This type is not printable.");
    }
  }

 private:
  void print_int128(const __int128_t& p) {
    os << to_baseB(p);
  }
  // --- AtCoder Library 出力ロジック ---
  void print_dsu(const atcoder::dsu& di) {
    atcoder::dsu d = di;
    auto groups = d.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      if (i) os << "\n";
      for (size_t j = 0; j < groups[i].size(); ++j) {
        if (j) os << " ";
        print(groups[i][j]);
      }
    }
  }

  template <typename T>
  void print_fenwick_tree(T bit) {
    for (std::size_t i = 0; i < bit._n; ++i) {
      if (i) os << " ";
      print(bit.sum(i, i + 1));
    }
  }

  template <typename T>
  void print_segtree(T seg) {
    for (std::size_t i = 0; i < seg._n; ++i) {
      if (i) os << " ";
      print(seg.get(i));
    }
  }

  template <typename T>
  void print_lazy_segtree(T seg) {
    for (std::size_t i = 0; i < seg._n; ++i) {
      if (i) os << " ";
      print(seg.get(i));
    }
  }

  // --- 標準データ構造の出力ロジック ---
  template <typename P>
  void print_pair(const P& p) {
    print(p.first);
    os << " ";
    print(p.second);
  }

  template <typename T>
  void print_tuple(const T& tup) {
    std::apply(
        [this](const auto&... args) {
          size_t n = 0;
          ((print(args), os << (++n == sizeof...(args) ? "" : " ")), ...);
        },
        tup);
  }

  template <typename T>
  void print_optional(const T& opt) {
    if (opt.has_value()) {
      print(*opt);
    } else {
      print("nullopt");
    }
  }

  template <typename T>
  void print_variant(const T& var) {
    if (var.valueless_by_exception()) {
      print("valueless");
    } else {
      std::visit(
          [this](const auto& val) {
            print(val);
          },
          var);
    }
  }

  template <typename T>
  void print_vector2d(const T& grid) {
    for (size_t i = 0; i < grid.size(); ++i) {
      if (i) os << '\n';
      for (size_t j = 0; j < grid[i].size(); ++j) {
        if (j) os << " ";
        print(grid[i][j]);
      }
    }
  }

  template <typename T>
  void print_adapter(const T& adapter_in) {
    using Container = std::decay_t<T>;
    Container adapter = adapter_in;

    bool first = true;
    while (!adapter.empty()) {
      if (!first) os << " ";
      if constexpr (requires { adapter.top(); }) {
        print(adapter.top());
        adapter.pop();
      } else if constexpr (requires { adapter.front(); }) {
        print(adapter.front());
        adapter.pop();
      }
      first = false;
    }
  }

  template <typename T>
  void print_map(const T& mp) {
    bool first = true;
    for (const auto& [k, v] : mp) {
      if (!first) os << "\n";
      print(k);
      os << " ";
      print(v);
      first = false;
    }
  }

  template <typename T>
  void print_range(const T& container) {
    bool first = true;
    for (const auto& elem : container) {
      if (!first) os << " ";
      print(elem);
      first = false;
    }
  }
};

class Reader {
 public:
  std::istream& is;

  Reader(std::istream& is_ = std::cin) : is(is_) {}

  template <class T>
  void read(T& val) {
    if constexpr (IsInt128<T>) {
      read_int128(val);
    } else if constexpr (IsModint<T>) {
      long long x;
      is >> x;
      val = x;
    } else if constexpr (IsPairLike<T>) {
      read(val.first);
      read(val.second);
    } else if constexpr (IsTuple<T>) {
      std::apply(
          [this](auto&... xs) {
            (read(xs), ...);
          },
          val);
    } else if constexpr (std::same_as<T, std::vector<bool>>) {
      for (long long i = 0; i < (long long)val.size(); ++i) {
        long long x;
        read(x);
        val[i] = (x != 0);
      }
    } else if constexpr (IsVector<T> || IsArray<T>) {
      for (auto& x : val) read(x);
    } else if constexpr (CanIstream<T>) {
      is >> val;
    } else {
      static_assert(dependent_false<T>,
                    "cin_impl::Reader: This type is not readable.");
    }
  }

 private:
  template <typename T>
  void read_int128(T& val) {
    using U = __uint128_t;

    std::string s;
    if (!(is >> s)) return;

    bool neg = s[0] == '-';
    std::size_t idx = (s[0] == '-' || s[0] == '+') ? 1 : 0;

    constexpr U umax = ~U(0);
    constexpr U smax = umax >> 1;

    U limit = umax;
    if constexpr (std::is_same_v<T, __int128_t>) {
      limit = neg ? smax + 1 : smax;
    } else if (neg) {
      is.setstate(std::ios::failbit);
      return;
    }

    if (idx == s.size()) {
      is.setstate(std::ios::failbit);
      return;
    }

    U x = 0;
    for (; idx < s.size(); ++idx) {
      if (s[idx] < '0' || s[idx] > '9') {
        is.setstate(std::ios::failbit);
        return;
      }

      U digit = s[idx] - '0';
      if (x > (limit - digit) / 10) {
        is.setstate(std::ios::failbit);
        return;
      }
      x = x * 10 + digit;
    }

    if constexpr (std::is_same_v<T, __int128_t>) {
      if (neg && x != 0) {
        val = -static_cast<__int128_t>(x - 1) - 1;
      } else {
        val = static_cast<__int128_t>(x);
      }
    } else {
      val = x;
    }
  }
};

}  // namespace io_impl

template <class T>
  requires(io_impl::IOByVal<T> || io_impl::IsContainerAdapter<T> ||
           io_impl::IsMapLike<T> || io_impl::IsTuple<T> ||
           io_impl::IsPairLike<T> || io_impl::IsPrintableRange<T> ||
           io_impl::IsOptional<T> || io_impl::IsVariant<T> ||
           io_impl::IsAtCoderDSU<T> || io_impl::IsAtCoderFenwickTree<T> ||
           io_impl::IsAtCoderLazySegtree<T> || io_impl::IsAtCoderSegtree<T> ||
           io_impl::IsInt128<T>)
inline std::ostream& operator<<(std::ostream& os, T& val) {
  io_impl::Printer printer(os);
  printer.print(val);
  return os;
}

template <class T>
  requires(io_impl::IsInt128<T> || io_impl::IsVector<T> ||
           io_impl::IsTuple<T> || io_impl::IsPairLike<T> ||
           io_impl::IsArray<T> || io_impl::IsModint<T>)
inline std::istream& operator>>(std::istream& is, T& val) {
  io_impl::Reader reader(is);
  reader.read(val);
  return is;
}

// --- 出力ユーティリティ関数 ---
inline void out() {
#ifdef LOCAL
  std::cerr << "[out]" << std::endl;
#endif
  std::cout << std::endl;
}

template <class T>
inline void out(const T& a) {
#ifdef LOCAL
  std::cerr << "[out] " << a << std::endl;
#endif
  std::cout << a << std::endl;
}

template <class T, class... Ts>
inline void out(const T& a, const Ts&... b) {
#ifdef LOCAL
  std::cerr << "[out] " << a;
  (..., (std::cerr << ' ' << b));
  std::cerr << std::endl;
#endif

  std::cout << a;
  (..., (std::cout << ' ' << b));
  std::cout << std::endl;
}

template <class T>
inline void out_without_endl(const T& a) {
#ifdef LOCAL
  std::cerr << "[out] " << a;
#endif
  std::cout << a;
}

template <class T, class... Ts>
inline void out_without_endl(const T& a, const Ts&... b) {
#ifdef LOCAL
  std::cerr << "[out] " << a;
  (..., (std::cerr << ' ' << b));
#endif

  std::cout << a;
  (..., (std::cout << ' ' << b));
}
