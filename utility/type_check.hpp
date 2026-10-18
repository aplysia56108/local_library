#pragma once

#include <atcoder/all>
#include <concepts>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <array>
#include <cstdint>
#include <istream>

namespace type_check {

template <typename T>
concept CanOstream = requires(std::ostream& os, const T& val) {
  { os << val } -> std::same_as<std::ostream&>;
};

template <class T>
concept IsString = std::same_as<std::decay_t<T>, std::string> ||
                   std::same_as<std::decay_t<T>, std::string_view> ||
                   std::same_as<std::decay_t<T>, const char*> ||
                   std::same_as<std::decay_t<T>, char*>;

template <typename T>
struct is_tuple : std::false_type {};
template <typename... Args>
struct is_tuple<std::tuple<Args...>> : std::true_type {};
template <typename T>
concept IsTuple = is_tuple<std::decay_t<T>>::value;

template <typename T>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};
template <typename T>
concept IsOptional = is_optional<std::decay_t<T>>::value;

template <typename T>
struct is_variant : std::false_type {};
template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {};
template <typename T>
concept IsVariant = is_variant<std::decay_t<T>>::value;

// 1次元 vector 判定
template <class T>
struct is_vector : std::false_type {};

template <class T, class Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <class T>
concept IsVector = is_vector<std::remove_cvref_t<T>>::value;

// 2次元 vector 判定
template <typename T>
struct is_vector_2d : std::false_type {};
template <typename U, typename Alloc1, typename Alloc2>
struct is_vector_2d<std::vector<std::vector<U, Alloc1>, Alloc2>>
    : std::true_type {};
template <typename T>
concept IsVector2D = is_vector_2d<std::decay_t<T>>::value;

// pair判定
template <class T>
concept IsPairLike = requires(const T& x) {
  x.first;
  x.second;
};

// map / unordered_map 判定 (mapped_type と key_type を持つコンテナ)
template <typename T>
concept IsMapLike = requires {
  typename std::decay_t<T>::key_type;
  typename std::decay_t<T>::mapped_type;
  typename std::decay_t<T>::value_type;
};

// queue, priority_queue, stack などのアダプター系コンテナ判定
template <class T>
concept IsContainerAdapter =
    requires(std::remove_cvref_t<T> a) {
      { a.empty() } -> std::convertible_to<bool>;
      a.pop();
    } &&
    (requires(std::remove_cvref_t<T> a) { a.top(); } ||
     requires(std::remove_cvref_t<T> a) { a.front(); });

// AtCoder Library 判定用コンセプト
template <typename T>
concept IsAtCoderDSU = std::same_as<std::decay_t<T>, atcoder::dsu>;

template <typename T>
struct is_fenwick_tree : std::false_type {};
template <typename U>
struct is_fenwick_tree<atcoder::fenwick_tree<U>> : std::true_type {};
template <typename T>
concept IsAtCoderFenwickTree = is_fenwick_tree<std::decay_t<T>>::value;

template <typename T>
struct is_segtree : std::false_type {};
template <class S, S (*op)(S, S), S (*e)()>
struct is_segtree<atcoder::segtree<S, op, e>> : std::true_type {};
template <typename T>
concept IsAtCoderSegtree = is_segtree<std::decay_t<T>>::value;

template <typename T>
struct is_lazy_segtree : std::false_type {};
template <class S, S (*op)(S, S), S (*e)(), class F, S (*mapping)(F, S),
          F (*composition)(F, F), F (*id)()>
struct is_lazy_segtree<
    atcoder::lazy_segtree<S, op, e, F, mapping, composition, id>>
    : std::true_type {};
template <typename T>
concept IsAtCoderLazySegtree = is_lazy_segtree<std::decay_t<T>>::value;

template <class T>
concept IsPrintableRange =
    std::ranges::input_range<const std::remove_cvref_t<T>> && !IsString<T>;

template <class T>
concept IsInt128 = std::is_same_v<std::remove_cvref_t<T>, __int128_t> ||
                   std::is_same_v<std::remove_cvref_t<T>, __uint128_t>;

// --- ACL modint 判定 ---
template <class T>
struct is_modint : std::false_type {};

template <std::int32_t m>
struct is_modint<atcoder::static_modint<m>> : std::true_type {};

template <std::int32_t id>
struct is_modint<atcoder::dynamic_modint<id>> : std::true_type {};

template <class T>
concept IsModint = is_modint<std::remove_cvref_t<T>>::value;

// --- istream 入力可能判定 ---
template <class T>
concept CanIstream = requires(std::istream& is, T& val) {
  { is >> val } -> std::same_as<std::istream&>;
};

// --- std::array 判定 ---
template <class T>
struct is_array : std::false_type {};

template <class T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type {};

template <class T>
concept IsArray = is_array<std::remove_cvref_t<T>>::value;

// io_via_val が　タグとして存在する場合、val()を経由して出力する。
template <class T>
concept HasIOByValTag = requires(const std::remove_cvref_t<T>& x) {
  typename std::remove_cvref_t<T>::io_via_val;
  x.val();
};

template <class T>
concept IOByVal = (HasIOByValTag<T> || IsModint<T>);

}  // namespace type_check