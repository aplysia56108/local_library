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

// debugでlibraryを参照する場合のルール
// debug と通常ライブラリの定義衝突を回避する。
// type_check を debug 専用の名前空間に変更して取り込む。
#define type_check debug_type_check
#include "utility/type_check.hpp"
#undef type_check

namespace debug_impl {

using namespace debug_type_check;

// --- デバッグ出力クラス ---
class DebugPrinter {
 public:
  std::ostream& os;

  DebugPrinter(std::ostream& os_ = std::cerr) : os(os_) {}

  template <typename T>
  void print(const T& val) {
    if constexpr (IOByVal<T>) {
      print(val.val());
    } else if constexpr (IsString<T>) {
      if constexpr (CanOstream<T>)
        os << val;
      else
        os << "<unprintable>";
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
    } else if constexpr (IsMapLike<T>) {
      print_map(val);
    } else if constexpr (IsContainerAdapter<T>) {
      print_adapter(val);
    } else if constexpr (IsTuple<T>) {
      print_tuple(val);
    } else if constexpr (IsOptional<T>) {
      print_optional(val);
    } else if constexpr (IsVariant<T>) {
      print_variant(val);
    } else if constexpr (IsPairLike<T>) {
      print_pair(val);
    } else if constexpr (std::ranges::range<T>) {
      print_range(val);
    } else if constexpr (CanOstream<T>) {
      os << val;
    } else {
      os << "<unprintable>";
    }
  }

 private:
  // --- AtCoder Library 出力ロジック ---
  void print_dsu(const atcoder::dsu& di) {
    os << "{";
    atcoder::dsu d = di;
    auto groups = d.groups();
    for (size_t i = 0; i < groups.size(); ++i) {
      if (i) os << ", ";
      os << "[";
      for (size_t j = 0; j < groups[i].size(); ++j) {
        if (j) os << " ";
        os << groups[i][j];
      }
      os << "]";
    }
    os << "}";
  }

  template <typename T>
  void print_fenwick_tree(T bit) {
    os << "[";
    for (int i = 0; i < bit._n; ++i) {
      if (i) os << ", ";
      print(bit.sum(i, i + 1));
    }
    os << "]";
  }

  template <typename T>
  void print_segtree(T seg) {
    os << "[";
    for (int i = 0; i < seg._n; ++i) {
      if (i) os << ", ";
      print(seg.get(i));
    }
    os << "]";
  }

  template <typename T>
  void print_lazy_segtree(T seg) {
    os << "[";
    for (int i = 0; i < seg._n; ++i) {
      if (i) os << ", ";
      print(seg.get(i));
    }
    os << "]";
  }

  // --- 標準データ構造の出力ロジック ---
  template <typename T1, typename T2>
  void print_pair(const std::pair<T1, T2>& p) {
    os << "{";
    print(p.first);
    os << ", ";
    print(p.second);
    os << "}";
  }

  template <typename T>
  void print_vector(const std::vector<T>& v) {
    os << "[";
    for (size_t i = 0; i < v.size(); ++i) {
      if (i > 0) os << ", ";
      print(v[i]);
    }
    os << "]";
  }

  template <typename T>
  void print_tuple(const T& tup) {
    os << "(";
    std::apply(
        [this](const auto&... args) {
          size_t n = 0;
          ((print(args), os << (++n == sizeof...(args) ? "" : ", ")), ...);
        },
        tup);
    os << ")";
  }

  template <typename T>
  void print_optional(const T& opt) {
    if (opt.has_value()) {
      print(*opt);
    } else {
      os << "nullopt";
    }
  }

  template <typename T>
  void print_variant(const T& var) {
    if (var.valueless_by_exception()) {
      os << "valueless";
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
    if (grid.empty()) {
      os << "[]";
      return;
    }

    size_t R = grid.size();
    size_t C = 0;
    for (const auto& row : grid) C = std::max(C, row.size());

    std::vector<std::vector<std::string>> str_grid(R);
    std::vector<size_t> col_widths(C, 0);

    for (size_t i = 0; i < R; ++i) {
      str_grid[i].resize(grid[i].size());
      for (size_t j = 0; j < grid[i].size(); ++j) {
        std::ostringstream oss;
        DebugPrinter sub_printer(oss);
        sub_printer.print(grid[i][j]);

        str_grid[i][j] = oss.str();
        col_widths[j] = std::max(col_widths[j], str_grid[i][j].length());
      }
    }

    os << "[\n";
    for (size_t i = 0; i < R; ++i) {
      os << "  [";
      for (size_t j = 0; j < str_grid[i].size(); ++j) {
        if (j > 0) os << ", ";
        os << std::setw(col_widths[j]) << str_grid[i][j];
      }
      os << "]";
      if (i + 1 < R) os << ",";
      os << "\n";
    }
    os << "]";
  }

  template <typename T>
  void print_adapter(const T& adapter_in) {
    using Container = std::decay_t<T>;
    Container adapter = adapter_in;

    os << "[";
    bool first = true;
    while (!adapter.empty()) {
      if (!first) os << ", ";
      if constexpr (requires { adapter.top(); }) {
        print(adapter.top());
        adapter.pop();
      } else if constexpr (requires { adapter.front(); }) {
        print(adapter.front());
        adapter.pop();
      }
      first = false;
    }
    os << "]";
  }

  template <typename T>
  void print_map(const T& mp) {
    os << "[";
    bool first = true;
    for (const auto& [k, v] : mp) {
      if (!first) os << ", ";
      os << "{";
      print(k);
      os << "->";
      print(v);
      os << "}";
      first = false;
    }
    os << "]";
  }

  template <typename T>
  void print_range(const T& container) {
    os << "[";
    bool first = true;
    for (const auto& elem : container) {
      if (!first) os << ", ";
      print(elem);
      first = false;
    }
    os << "]";
  }
};

// --- 引数処理インターフェース ---

inline const char* skip_quoted_literal(const char* p) {
  const char quote = *p++;
  while (*p) {
    if (*p == '\\') {
      if (p[1]) ++p;
    } else if (*p == quote) {
      return p + 1;
    }
    ++p;
  }
  return p;
}

inline const char* skip_raw_string_literal(const char* p) {
  const char* delimiter_begin = p + 2;
  const char* open_paren = delimiter_begin;
  while (*open_paren && *open_paren != '(') ++open_paren;
  if (*open_paren != '(') return p + 1;

  const std::size_t delimiter_size = open_paren - delimiter_begin;
  for (const char* q = open_paren + 1; *q; ++q) {
    if (*q != ')') continue;
    if (std::strncmp(q + 1, delimiter_begin, delimiter_size) == 0 &&
        q[delimiter_size + 1] == '"') {
      return q + delimiter_size + 2;
    }
  }
  return open_paren;
}

inline const char* find_top_level_comma(const char* names) {
  int paren_depth = 0;
  int bracket_depth = 0;
  int brace_depth = 0;

  for (const char* p = names; *p;) {
    if (*p == '\'' || *p == '"') {
      p = skip_quoted_literal(p);
      continue;
    }
    if (*p == 'R' && p[1] == '"') {
      p = skip_raw_string_literal(p);
      continue;
    }

    if (*p == '(')
      ++paren_depth;
    else if (*p == ')' && paren_depth > 0)
      --paren_depth;
    else if (*p == '[')
      ++bracket_depth;
    else if (*p == ']' && bracket_depth > 0)
      --bracket_depth;
    else if (*p == '{')
      ++brace_depth;
    else if (*p == '}' && brace_depth > 0)
      --brace_depth;
    else if (*p == ',' && paren_depth == 0 && bracket_depth == 0 &&
             brace_depth == 0) {
      return p;
    }
    ++p;
  }
  return nullptr;
}

// ベースケース：すべての引数を出力し終えた際（引数2つ）に呼ばれる安全弁
inline void debug_internal_impl(DebugPrinter&, const char*) {}

template <typename T, typename... Args>
inline void debug_internal_impl(DebugPrinter& printer, const char* names,
                                const T& val, const Args&... args) {
  while (*names == ' ') names++;
  const char* comma = find_top_level_comma(names);
  if (comma) {
    printer.os.write(names, comma - names) << ": ";
    printer.print(val);
    printer.os << ", ";
    debug_internal_impl(printer, comma + 1, args...);
  } else {
    printer.os << names << ": ";
    printer.print(val);
    printer.os << std::endl;  // 最後の引数の出力後に改行＆フラッシュ
  }
}

template <typename... Args>
inline void debug_launcher(const char* names, const Args&... args) {
  DebugPrinter printer(std::cerr);
  debug_internal_impl(printer, names, args...);
}

}  // namespace debug_impl

#define debug(...) debug_impl::debug_launcher(#__VA_ARGS__, __VA_ARGS__)
