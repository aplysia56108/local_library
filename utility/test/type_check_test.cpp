#include <array>
#include <cassert>
#include <deque>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "utility/type_check.hpp"
#include "data-structure/btree/bplustree-lazysegtree.hpp"
#include "data-structure/btree/bplustree-segtree.hpp"

using namespace type_check;

using Mint = atcoder::modint998244353;

// ACL Segtree 用
inline long long op(long long a, long long b) {
  return a + b;
}
inline long long e() {
  return 0;
}

inline long long mapping(long long f, long long x) {
  return f + x;
}
inline long long composition(long long f, long long g) {
  return f + g;
}
inline long long id() {
  return 0;
}

using Seg = atcoder::segtree<long long, op, e>;
using LazySeg = atcoder::lazy_segtree<long long, op, e, long long, mapping,
                                      composition, id>;
using Fenwick = atcoder::fenwick_tree<long long>;

struct CustomPair {
  long long first;
  long long second;
};

struct CustomRange {
  std::vector<long long> values;

  auto begin() const {
    return values.begin();
  }
  auto end() const {
    return values.end();
  }
};

struct NonConstRange {
  std::vector<long long> values;

  auto begin() {
    return values.begin();
  }
  auto end() {
    return values.end();
  }
};

// ==========================================
// 19種類の型判定テスト
// ==========================================

void test_type_check() {
  // 1. IsAtCoderDSU
  static_assert(IsAtCoderDSU<atcoder::dsu>);
  static_assert(!IsAtCoderDSU<Seg>);
  static_assert(!IsAtCoderDSU<std::vector<long long>>);

  // 2. IsAtCoderSegtree
  static_assert(IsAtCoderSegtree<Seg>);
  static_assert(!IsAtCoderSegtree<LazySeg>);
  static_assert(!IsAtCoderSegtree<Fenwick>);

  // 3. IsAtCoderLazySegtree
  static_assert(IsAtCoderLazySegtree<LazySeg>);
  static_assert(!IsAtCoderLazySegtree<Seg>);
  static_assert(!IsAtCoderLazySegtree<Fenwick>);

  // 4. IsAtCoderFenwickTree
  static_assert(IsAtCoderFenwickTree<Fenwick>);
  static_assert(!IsAtCoderFenwickTree<Seg>);
  static_assert(!IsAtCoderFenwickTree<LazySeg>);

  // 5. IsString
  static_assert(IsString<std::string>);
  static_assert(IsString<std::string_view>);
  static_assert(IsString<const char*>);
  static_assert(IsString<char*>);
  static_assert(IsString<decltype("abc")>);
  static_assert(!IsString<std::vector<char>>);
  static_assert(!IsString<std::array<char, 3>>);

  // 6. IsVector
  static_assert(IsVector<std::vector<long long>>);
  static_assert(IsVector<std::vector<std::vector<long long>>>);
  static_assert(IsVector<std::vector<bool>>);
  static_assert(!IsVector<std::array<long long, 3>>);
  static_assert(!IsVector<std::deque<long long>>);
  static_assert(!IsVector<std::string>);

  // 7. IsVector2D
  static_assert(IsVector2D<std::vector<std::vector<long long>>>);
  static_assert(IsVector2D<std::vector<std::vector<std::string>>>);
  static_assert(!IsVector2D<std::vector<long long>>);
  static_assert(!IsVector2D<std::array<std::array<long long, 3>, 3>>);

  // 8. IsMapLike
  static_assert(IsMapLike<std::map<long long, long long>>);
  static_assert(IsMapLike<std::unordered_map<long long, long long>>);
  static_assert(IsMapLike<std::multimap<long long, long long>>);
  static_assert(!IsMapLike<std::set<long long>>);
  static_assert(!IsMapLike<std::vector<long long>>);

  // 9. IsPairLike
  static_assert(IsPairLike<std::pair<long long, long long>>);
  static_assert(IsPairLike<CustomPair>);
  static_assert(!IsPairLike<std::tuple<long long, long long>>);
  static_assert(!IsPairLike<std::array<long long, 2>>);

  // 10. IsTuple
  static_assert(IsTuple<std::tuple<>>);
  static_assert(IsTuple<std::tuple<long long, char>>);
  static_assert(!IsTuple<std::pair<long long, char>>);
  static_assert(!IsTuple<std::array<long long, 2>>);

  // 11. IsPrintableRange
  static_assert(IsPrintableRange<std::vector<long long>>);
  static_assert(IsPrintableRange<std::array<long long, 3>>);
  static_assert(IsPrintableRange<std::deque<long long>>);
  static_assert(IsPrintableRange<std::span<const long long>>);
  static_assert(IsPrintableRange<CustomRange>);
  static_assert(IsPrintableRange<SumSegtree>);
  static_assert(IsPrintableRange<AddSumLazySegtree>);
  static_assert(!IsPrintableRange<NonConstRange>);
  static_assert(!IsPrintableRange<std::string>);
  static_assert(!IsPrintableRange<std::string_view>);
  static_assert(!IsPrintableRange<long long>);

  // 12. IsContainerAdapter
  static_assert(IsContainerAdapter<std::queue<long long>>);
  static_assert(IsContainerAdapter<std::stack<long long>>);
  static_assert(IsContainerAdapter<std::priority_queue<long long>>);
  static_assert(!IsContainerAdapter<std::vector<long long>>);
  static_assert(!IsContainerAdapter<std::deque<long long>>);

  // 13. IsVariant
  static_assert(IsVariant<std::variant<long long, std::string>>);
  static_assert(IsVariant<std::variant<std::monostate, long long>>);
  static_assert(!IsVariant<std::optional<long long>>);
  static_assert(!IsVariant<std::tuple<long long, char>>);

  // 14. IsOptional
  static_assert(IsOptional<std::optional<long long>>);
  static_assert(IsOptional<std::optional<std::string>>);
  static_assert(!IsOptional<std::variant<long long, char>>);
  static_assert(!IsOptional<std::vector<long long>>);

  // 15. IOByVal
  static_assert(IOByVal<IdentityValue<long long>>);
  static_assert(IOByVal<Mint>);
  static_assert(!IOByVal<long long>);
  static_assert(!IOByVal<std::vector<long long>>);

  // 16. CanOstream
  static_assert(CanOstream<long long>);
  static_assert(CanOstream<std::string>);
  static_assert(CanOstream<char>);
  static_assert(!CanOstream<std::vector<long long>>);

  // 17. IsModint
  static_assert(IsModint<atcoder::modint998244353>);
  static_assert(IsModint<atcoder::dynamic_modint<0>>);
  static_assert(!IsModint<long long>);
  static_assert(!IsModint<std::vector<long long>>);

  // 18. CanIstream
  static_assert(CanIstream<long long>);
  static_assert(CanIstream<double>);
  static_assert(CanIstream<char>);
  static_assert(CanIstream<std::string>);
  static_assert(!CanIstream<std::vector<long long>>);

  // 19. IsArray
  static_assert(IsArray<std::array<long long, 3>>);
  static_assert(IsArray<const std::array<long long, 3>&>);
  static_assert(!IsArray<std::vector<long long>>);
  static_assert(!IsArray<long long>);

  std::cout << "test type check is passed!" << std::endl;
}

template <class T, template <class> class Check>
concept CvrefInvariant = (Check<T>::value == Check<const T>::value) &&
                         (Check<T>::value == Check<T&>::value) &&
                         (Check<T>::value == Check<const T&>::value) &&
                         (Check<T>::value == Check<T&&>::value);

template <class T>
using CheckIsVector = std::bool_constant<type_check::IsVector<T>>;
template <class T>
using CheckIsString = std::bool_constant<type_check::IsString<T>>;
template <class T>
using CheckIsVector2D = std::bool_constant<type_check::IsVector2D<T>>;
template <class T>
using CheckIsPrintableRange =
    std::bool_constant<type_check::IsPrintableRange<T>>;
template <class T>
using CheckIsPairLike = std::bool_constant<type_check::IsPairLike<T>>;
template <class T>
using CheckIsMapLike = std::bool_constant<type_check::IsMapLike<T>>;
template <class T>
using CheckIsTuple = std::bool_constant<type_check::IsTuple<T>>;
template <class T>
using CheckIsAtCoderDSU = std::bool_constant<type_check::IsAtCoderDSU<T>>;
template <class T>
using CheckIsAtCoderFenwickTree =
    std::bool_constant<type_check::IsAtCoderFenwickTree<T>>;
template <class T>
using CheckIsAtCoderSegtree =
    std::bool_constant<type_check::IsAtCoderSegtree<T>>;
template <class T>
using CheckIsAtCoderLazySegtree =
    std::bool_constant<type_check::IsAtCoderLazySegtree<T>>;
template <class T>
using CheckIsContainerAdapter =
    std::bool_constant<type_check::IsContainerAdapter<T>>;
template <class T>
using CheckIsOptional = std::bool_constant<type_check::IsOptional<T>>;
template <class T>
using CheckIsVariant = std::bool_constant<type_check::IsVariant<T>>;
template <class T>
using CheckIOByVal = std::bool_constant<type_check::IOByVal<T>>;
template <class T>
using CheckIsModint = std::bool_constant<type_check::IsModint<T>>;
template <class T>
using CheckIsArray = std::bool_constant<type_check::IsArray<T>>;

template <class T>
void test() {
  static_assert(CvrefInvariant<T, CheckIsAtCoderDSU>);
  static_assert(CvrefInvariant<T, CheckIsAtCoderSegtree>);
  static_assert(CvrefInvariant<T, CheckIsAtCoderLazySegtree>);
  static_assert(CvrefInvariant<T, CheckIsAtCoderFenwickTree>);
  static_assert(CvrefInvariant<T, CheckIsString>);
  static_assert(CvrefInvariant<T, CheckIsVector>);
  static_assert(CvrefInvariant<T, CheckIsVector2D>);
  static_assert(CvrefInvariant<T, CheckIsMapLike>);
  static_assert(CvrefInvariant<T, CheckIsPairLike>);
  static_assert(CvrefInvariant<T, CheckIsTuple>);
  static_assert(CvrefInvariant<T, CheckIsPrintableRange>);
  static_assert(CvrefInvariant<T, CheckIsContainerAdapter>);
  static_assert(CvrefInvariant<T, CheckIsVariant>);
  static_assert(CvrefInvariant<T, CheckIsOptional>);
  static_assert(CvrefInvariant<T, CheckIOByVal>);
  static_assert(CvrefInvariant<T, CheckIsModint>);
  static_assert(CvrefInvariant<T, CheckIsArray>);
}

void test_cvref_all() {
  test<atcoder::dsu>();
  test<Seg>();
  test<LazySeg>();
  test<Fenwick>();

  test<std::string>();
  test<std::string_view>();
  test<const char*>();

  test<std::vector<long long>>();
  test<std::vector<std::vector<long long>>>();
  test<std::map<long long, long long>>();
  test<std::unordered_map<long long, long long>>();
  test<std::pair<long long, long long>>();
  test<CustomPair>();
  test<std::tuple<long long, char>>();

  test<std::array<long long, 3>>();
  test<std::span<const long long>>();
  test<CustomRange>();
  test<NonConstRange>();

  test<std::queue<long long>>();
  test<std::stack<long long>>();
  test<std::priority_queue<long long>>();

  test<std::variant<long long, char>>();
  test<std::optional<long long>>();

  test<long long>();

  std::cout << "test cvref all is passed!" << std::endl;
}
int main() {
  test_type_check();
  test_cvref_all();

  std::cout << "type check all passed!" << std::endl;
}