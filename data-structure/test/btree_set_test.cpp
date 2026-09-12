#include <concepts>
#include <type_traits>

#include "data-structure/set/btree-set.hpp"

int main() {
  using Set = Btree_Set<int>;

  static_assert(std::default_initializable<Set>);
  static_assert(std::same_as<Set::key_type, int>);
  static_assert(std::same_as<Set::value_type, int>);
  static_assert(std::same_as<Set::size_type, std::size_t>);
  static_assert(std::same_as<Set::iterator, Btree_Set<int>::iterator>);
  static_assert(
      std::same_as<Set::const_iterator, Btree_Set<int>::const_iterator>);
}
