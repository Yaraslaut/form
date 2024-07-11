// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <algorithm>
#include <boxed-cpp/boxed.hpp>
#include <complex>
#include <form/form.h>
#include <print>
#include <thread>

struct S {
  unsigned i : 2, j : 6;
};

struct X {
  int m1 = 1;
};

struct Y {
  int m2 = 2;
};

class Z : public X, private Y {
  int m3 = 3;
  int m4 = 4;
};

struct A {
  int a;
  int b;
};

struct AA {
  A a;
  A b;
};

/// ColumnCount simply represents a number of columns.
using ColumnCount = boxed::boxed<int>;

/// LineCount represents a number of lines.
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

namespace documentation {

template <std::size_t N> struct StringLiteral {
  constexpr StringLiteral(const char (&str)[N]) : value{} {
    std::copy_n(str, N, value);
  }

  char value[N]; // NOLINT
};

constexpr StringLiteral Dummy{"{comment} Dummy config entry \n"};

} // namespace documentation

template <typename T, documentation::StringLiteral doc> struct ConfigEntry {
  using value_type = T;

  std::string documentation = doc.value;
  constexpr ConfigEntry() : _value{} {}
  //clang-format off
  constexpr explicit ConfigEntry(T &&in) : _value{std::forward<T>(in)} {}

  template <typename F>
  constexpr explicit ConfigEntry(F &&in) : _value{std::forward<F>(in)} {}
  //clang-format on

  [[nodiscard]] constexpr T const &value() const { return _value; }
  [[nodiscard]] constexpr T &value() { return _value; }

  constexpr ConfigEntry &operator=(T const &value) {
    _value = value;
    return *this;
  }

  constexpr ConfigEntry &operator=(T &&value) noexcept {
    _value = std::move(value);
    return *this;
  }

  constexpr ConfigEntry(ConfigEntry const &) = default;
  constexpr ConfigEntry &operator=(ConfigEntry const &) = default;
  constexpr ConfigEntry(ConfigEntry &&) noexcept = default;
  constexpr ConfigEntry &operator=(ConfigEntry &&) noexcept = default;
  ~ConfigEntry() = default;

private:
  T _value;
};

template <typename A, documentation::StringLiteral B>
struct std::formatter<ConfigEntry<A, B>> {
  using Entry = ConfigEntry<A, B>;
  template <class ParseContext>
  constexpr ParseContext::iterator parse(ParseContext &ctx) {
    return ctx.begin();
  }

  template <class FmtContext>
  FmtContext::iterator format(Entry s, FmtContext &ctx) const {
    return format_to(ctx.out(), "{}", s.value());
  }
};

template <typename A, typename B>
struct std::formatter<boxed::detail::boxed<A, B>> {
  using Boxed = boxed::detail::boxed<A, B>;
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  auto format(const Boxed &obj, format_context &ctx) const {
    return std::format_to(ctx.out(), "{}", obj.value);
  }
};

template <typename T> void serialize(T const &val) {
  std::println("================================== [Serialized] YAML");
  auto data_yaml = form::format_yaml(val);
  std::println("{}", data_yaml);

  std::println("================================== [Serialized] JSON");
  auto data_json = form::format_json(val);
  std::println("{}", data_json);
}

template <typename T> void deserialize(T const &val) {
  std::println("================================== [Deserialized]");
  try {
    auto data = form::format_yaml(val);
    auto deserialized = form::from_yaml<T>(data);
    std::println("{}", form::format_yaml(deserialized));
  } catch (const std::exception &e) {
    std::println("Failed");
  }
}

template <typename T> bool round_trip(T const &val) {
  const auto data = form::format_yaml(val);
  const auto deserialized = form::from_yaml<T>(data);
  return form::compare(val, deserialized);
}

using StrongType = boxed::boxed<int>;

struct ConfigEntries {
  ConfigEntry<int, documentation::Dummy> int_entry;
  ConfigEntry<double, documentation::Dummy> double_entry;
  // ConfigEntry<StrongType, documentation::Dummy> strong_entry;
};

namespace detail {

template <typename T, typename Tag = decltype([] {})> struct CreateUniqueT;

// clang-format off
template <typename T> constexpr auto CreateClass() {
  return define_class(^T, {
    data_member_spec(^int,{.name = "i"}),
    data_member_spec(^int, {.name = "j"})
  });
}
// clang-format on

} // namespace detail

namespace form_same_as {
template <typename T> int foo(T) { return -1; }

template <typename T>
  requires form::same_as<T, ^std::complex>
int foo(T) {
  return 1;
}

} // namespace form_same_as

namespace form::tests {

void testZ() {
  Z z;
  z.m1 = -3;
  serialize(z);
  deserialize(z);
}

void testPrintMembers() { form::print_members<Z>(); }

void testCreateClass() {
  constexpr auto cls = detail::CreateClass<detail::CreateUniqueT<int>>();
  form::print_members<[:cls:]>();
}

void testAA() {
  AA aa;
  aa.a.a = 123;
  aa.a.b = 0;
  aa.b.a = 0;
  aa.b.b = 321;
  serialize(aa);
  deserialize(aa);
}

void testPageSize() {
  PageSize ps;
  ps.lines = LineCount{5};
  ps.columns = ColumnCount{7};
  serialize(ps);
  deserialize(ps);
}

void testConfig() {
  Config config;
  config.b = 3.1;
  config.page_size = PageSize{LineCount{5}, ColumnCount{7}};
  serialize(config);
  deserialize(config);
}

void testConfigEntry() {
  ConfigEntries entries;
  entries.int_entry = 1;
  entries.double_entry = 1.1;
  // entries.strong_entry = StrongType(3);
  serialize(entries);
  deserialize(entries);
}

void testSJson() {
  S s;
  s.i = 1;
  s.j = 2;
  std::println("{}", form::format_json(s));
}

bool CheckSameAs() { return form_same_as::foo(std::complex<float>{1, 1}) == 1; }

bool compareTrue() {
  return form::compare(3.0, 3.0) && form::compare(3, 3) &&
         form::compare("a", "a");
}

bool compareFalse() {
  bool res = form::compare(3.0, 3.1) ||
             form::compare(3, 4); //|| form::compare("a", "b");
  return res == false;
}

bool compareS() {
  S s;
  s.i = 1;
  s.j = 2;
  S s2;
  return compare(s, s2) == false;
}

bool compareX() {
  X x;
  X x2;
  return compare(x, x2) == true;
}

bool compareA() {
  A a;
  a.a = 1;
  a.b = 2;
  A a2;
  return compare(a, a2) == false;
}

bool compareAA() {
  AA aa;
  aa.a.a = 123;
  aa.a.b = 321;
  AA aa2;
  return compare(aa, aa2) == false;
}

bool SRoundTrip() {
  S s;
  s.i = 1;
  s.j = 2;
  return round_trip(s);
}

bool XRoundTrip() {
  X x;
  return round_trip(x);
}

bool YRoundTrip() {
  Y y;
  return round_trip(y);
}

bool ZRoundTrip() {
  Z z;
  z.m1 = -3;
  return round_trip(z);
}

bool ARoundTrip() {
  A a;
  a.a = 1;
  a.b = 2;
  return round_trip(a);
}

bool ConfigRoundTrip() {
  Config config;
  config.b = 3.1;
  config.page_size = PageSize{LineCount{5}, ColumnCount{7}};
  return round_trip(config);
}

} // namespace form::tests

namespace list {
struct CancelSelection {};
struct ClearHistoryAndReset {};
} // namespace list
using list_variant = [:form::util::create_variant(^list):];

enum class Color { red, green, blue };

enum class Decorator : uint8_t {
  Underline,
  DoubleUnderline,
  CurlyUnderline,
  DottedUnderline,
  DashedUnderline,
  Overline,
  CrossedOut,
  Framed,
  Encircle,
};

struct struct_with_padding {
  char a;
  double c;
};

struct struct_no_padding {
  double c;
  char a;
};

template <form::no_padding T> void foo(T t) { std::println("Without padding"); }

template <typename T> void foo(T t) {
  std::println("With padding: {}", form::get_padding<T>());
}
namespace form::examples {

bool VariantCreate() {
  list_variant v{list::CancelSelection{}};
  return std::holds_alternative<list::CancelSelection>(v);
}

bool VariantToString() {
  list_variant v{list::CancelSelection{}};
  auto variant_name = form::variant_type_to_string(v);
  return "CancelSelection" == variant_name;
}

bool EnumToString() { return form::enum_to_string(Color::red) == "red"; }

bool EnumToStringWithTransform() {
  bool res = true;
  auto transform = [](std::string data) {
    std::string out;
    out += std::tolower(data[0]);
    for (auto const c : data.substr(1, data.size())) {
      if (std::isupper(c))
        out += "-";
      out += std::tolower(c);
    }
    return out;
  };
  res &= (form::enum_to_string(Decorator::Underline, transform) == "underline");
  res &= (form::enum_to_string(Decorator::DottedUnderline, transform) ==
          "dotted-underline");
  return res;
}

void PaddingCheck() {

  foo(struct_with_padding{});
  foo(struct_no_padding{});
  static_assert(form::no_padding<struct_no_padding>);
}

} // namespace form::examples

void ExampleConfig() {
  std::println("{}", form::format_yaml([]() {
                 Config c;
                 c.b = 3.1;
                 c.v = 10;
                 c.page_size = PageSize(LineCount(3), ColumnCount(5));
                 return c;
               }()));
  /*
  live: false
  v: 10
  b: 3.1
  page_size:
    lines:
      value: 3
    columns:
      value: 5
  */
}
