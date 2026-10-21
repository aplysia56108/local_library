#include "data-structure/btree/components/bplustree-setter.hpp"
#include "data-structure/btree/components/bplustree-base.hpp"

#include <cassert>
#include <string>
#include <type_traits>

struct Action { std::string text; };
struct StringTree {
  using value_type = std::string;
  std::string value;
  Bplustree::Setter<StringTree> operator()(std::size_t i) { return {*this, i}; }
  const std::string& get(std::size_t) const { return value; }
  void set(std::size_t, const std::string& x) { value = x; }
  void prod_right(std::size_t, const std::string& x) { value += x; }
  void prod_left(std::size_t, const std::string& x) { value = x + value; }
  void apply(std::size_t, const Action& x) { value += x.text; }
  void apply_left(std::size_t, const Action& x) { value = x.text + value; }
};
template <class S>
concept HasMinus = requires(S s) { std::move(s) -= std::string{}; };
template <class S>
concept HasApply = requires(S s) { std::move(s).apply(Action{}); };
using StringSetter = Bplustree::Setter<StringTree>;
using Plain = Bplustree::Base<Bplustree::Traits<int>>;
static_assert(!std::is_copy_constructible_v<StringSetter>);
static_assert(!std::is_move_constructible_v<StringSetter>);
static_assert(!HasMinus<StringSetter> && HasApply<StringSetter>);
static_assert(!HasApply<typename Plain::setter_type>);

int main() {
  StringTree tree;
  tree(0) = "x";
  tree(0).prod_right("r"); tree(0).prod_left("l"); tree(0).prod("p");
  tree(0).apply(Action{"a"}); tree(0).apply_left(Action{"b"});
  tree(0) += "z";
  assert(tree(0).val() == "blxrpaz");
  Plain plain{20};
  plain(0) += 4; assert(plain[0] == 24);
  plain(0) -= 2; assert(plain[0] == 22);
  plain(0) *= 3; assert(plain[0] == 66);
  plain(0) /= 2; assert(plain[0] == 33);
  plain(0) %= 10; assert(plain[0] == 3);
  plain(0) |= 4; assert(plain[0] == 7);
  plain(0) &= 6; assert(plain[0] == 6);
  plain(0) ^= 3; assert(plain[0] == 5);
  plain(0) <<= 2; assert(plain[0] == 20);
  plain(0) >>= 1; assert(plain[0] == 10);
}
