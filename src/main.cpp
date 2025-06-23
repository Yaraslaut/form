// SPDX-License-Identifier: Apache-2.0
#include <boxed-cpp/boxed.hpp>
#include <complex>
#include <cstdio>
#include <form/form.h>
#include <print>

using ColumnCount = boxed::boxed<int>;
using LineCount = boxed::boxed<int>;

struct PageSize {
  LineCount lines;
  ColumnCount columns;
};

struct Config {
  bool live{false};
  int v{90};
  double b{90.0};
  PageSize page_size{LineCount{10}, ColumnCount{10}};
};

namespace list {
struct CancelSelection {};
struct ClearHistoryAndReset {};
} // namespace list
using list_variant = [:form::util::create_variant(^^list):];

enum class Color { red, green, blue };

namespace for_tests {
bool test_true() { return true; }
bool test_false() { return false; }
void test_void() {};
} // namespace for_tests

namespace run {

void SerializationIntoDifferentFormats() {
  Config c;
  std::println("===== JSON =====");
  std::println("{}", form::format_json(c));
  std::println("===== YAML =====");
  std::println("{}", form::format_yaml(c));
  std::println("===== UNIVERSAL =====");
  std::println("{}", form::format_universal(c));
}

void VariantToString() {
  list_variant v{list::CancelSelection{}};
  std::println("{}", form::variant_type_to_string(v));
}

void EnumToString() { std::println("{}", form::enum_to_string(Color::red)); }

void runTests() { form::run_tests<^^for_tests>(); }

} // namespace run

int main() {
  form::run_tests<^^run>();
  return 0;
}
