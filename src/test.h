// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <algorithm>
#include <complex>
#include <form/form.h>
#include <print>
#include <thread>

namespace log {

template <class... Args>
inline void println(std::format_string<Args...> fmt, Args &&...args) {
#if 0
  std::println(fmt, std::forward<Args>(args)...);
#endif
}

} // namespace log

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

using ColumnCount = int;
using LineCount = int;

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

template <typename T> void serialize(T const &val) {
   log::println("================================== [Serialized] YAML");
   auto data_yaml = form::format_yaml(val);
   log::println("{}", data_yaml);

   // log::println("================================== [Serialized] JSON");
   // auto data_json = form::format_json(val);
   // log::println("{}", data_json);
}

namespace form::tests {

bool compareS() {
   S s;
   s.i = 1;
   s.j = 2;
   S s2;
   return form::compare(s, s2) == false;
}

bool compareX() {
   X x;
   X x2;
   return form::compare(x, x2) == true;
}

bool compareA() {
   A a;
   a.a = 1;
   a.b = 2;
   A a2;
   return form::compare(a, a2) == false;
}

bool compareAA() {
   AA aa;
   aa.a.a = 123;
   aa.a.b = 321;
   AA aa2;
   return form::compare(aa, aa2) == false;
}

} // namespace form::tests

namespace list {
struct CancelSelection {};
struct ClearHistoryAndReset {};
} // namespace list

using list_variant = [:form::create_variant(^^list):];

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

namespace for_invoke {

std::string foo(std::string_view a, std::string_view b) {
   return std::format("{} {}", a, b);
}

int bar(int a, std::string_view b) { return a + b.size(); }

} // namespace for_invoke

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
   auto transform = [](auto data) {
      std::string out;
      out += std::tolower(data[0]);
      for (auto const c : data.substr(1, data.size())) {
         if (std::isupper(c))
            out += "-";
         out += std::tolower(c);
      }
      return out;
   };
   res &=
       (form::enum_to_string(Decorator::Underline, transform) == "underline");
   res &= (form::enum_to_string(Decorator::DottedUnderline, transform) ==
           "dotted-underline");
   return res;
}

bool StringToEnum() {
   bool res = true;
   res &= form::string_to_enum<Color>("red").value() == Color::red;
   res &= form::string_to_enum<Color>("green").value() == Color::green;
   res &= form::string_to_enum<Color>("blue").value() == Color::blue;
   return res;
}

bool PaddingCheck() {
   static_assert(!form::no_padding<struct_with_padding>);
   static_assert(form::no_padding<struct_no_padding>);
   return form::no_padding<struct_no_padding> &&
          (!form::no_padding<struct_with_padding>);
}

bool CheckInvoke() {
   bool res = form::invoke<^^for_invoke::foo>() == std::string_view("a b");
   res &= form::invoke<^^for_invoke::bar>() == 1;
   return res;
}

} // namespace form::examples
