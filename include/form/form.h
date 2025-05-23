// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <concepts>
#include <form/format.h>
#include <format>
#include <iterator>
#include <print>
#include <thread>
#include <type_traits>

namespace form {

namespace detail {
using namespace form;

template <typename Inner>
constexpr Inner string_to_enum(std::string_view value) {
  Inner result{};

  constexpr auto enum_type = []<typename Check>(this auto self, Check) {
    if constexpr (std::is_enum_v<Check>) {
      return ^^Check;
    } else {
      return self(typename Check::value_type{});
    }
  }(Inner{});

  [:util::expand(std::meta::enumerators_of(enum_type)):] >> [&]<auto e> {
    if (value == util::name_of(e)) {
      result = [:e:];
    }
  };
  return result;
}
} // namespace detail

template <typename E> constexpr std::string variant_type_to_string(E value) {
  std::string out = "";
  [:util::expand(template_arguments_of(^^E)):] >> [&]<auto e> {
    if (auto *pval = std::get_if<typename[:e:]>(&value)) {
      std::format_to(std::back_inserter(out), "{}", util::name_of(e));
    }
  };
  return out;
}

template <typename E>
  requires std::is_enum_v<E>
constexpr std::string enum_to_string(E value) {
  std::string result = "<unnamed>";
  [:util::expand(std::meta::enumerators_of(^^E)):] >> [&]<auto e> {
    if (value == [:e:]) {
      result = util::name_of(e);
    }
  };
  return result;
}

template <typename E>
  requires std::is_enum_v<E>
constexpr std::string enum_to_string(E value, auto transformation) {
  return transformation(enum_to_string(value));
}

template <typename E>
  requires std::is_enum_v<E>
constexpr std::optional<E> string_to_enum(std::string_view value) {
  return detail::string_to_enum<std::optional<E>>(value);
}

template <typename T> bool compare(T const &lhs, T const &rhs) {
  bool result = true;
  if constexpr (std::is_arithmetic_v<T>)
    return lhs == rhs;
  if constexpr (std::is_class_v<T>)
    util::for_range<0, util::number_of_members<T>()>([&]<auto I>() {
      constexpr auto mem = util::member_info<T>(I);
      if (!compare(lhs.[:mem:], rhs.[:mem:])) {
        result = result && false;
      }
    });
  return result;
}

template <auto refl> void run_par() {
  std::vector<std::thread> pool;
  [:util::expand(
        members_of(refl, std::meta::access_context::unchecked())):] >> [&]<auto mem> {
    pool.emplace_back(std::thread([&]() { [:mem:](); }));
  };

  for (auto &t : pool) {
    t.join();
  }
}

template <auto refl> void run_seq() {
  [:util::expand(members_of(
        refl, std::meta::access_context::unchecked())):] >> [&]<auto mem> { [:mem:](); };
}

template <auto refl> void run_tests() {
  std::vector<std::thread> pool;
  [:util::expand(members_of(
        refl, std::meta::access_context::unchecked())):] >> [&]<auto mem>
                                                requires std::invocable<
                                                    typename[:type_of(mem):]>
  {
    pool.emplace_back(std::thread([&]() {
      std::string out;
      if constexpr (std::is_same_v<
                        std::invoke_result_t<typename[:type_of(mem):]>, bool>) {
        if ([:mem:]())
          std::format_to(std::back_inserter(out), "{}", "\N{FIRE}");
        else {
          std::format_to(std::back_inserter(out), "{}", "\N{PILE OF POO}");
        }
      } else {
        [:mem:]();
        std::format_to(std::back_inserter(out), "{}", "\N{FOUR LEAF CLOVER}");
      }
      std::format_to(std::back_inserter(out), "{}", util::name_of(mem));
      std::println(" {}", out);
    }));
  };

  for (auto &t : pool) {
    t.join();
  }
}

template <typename S> consteval std::size_t get_padding() {

  std::size_t pointer{static_cast<size_t>(
      offset_of(nonstatic_data_members_of(^^S, std::meta::access_context::unchecked())[0])
          .bytes)};
  std::size_t padding{0};

  [:util::expand(nonstatic_data_members_of(
        ^^S, std::meta::access_context::unchecked())):] >> [&, i = 0]<auto e>() {
    if (pointer == offset_of(e).bytes)
      pointer += size_of(e);
    else {
      padding += offset_of(e).bytes - pointer;
      pointer = offset_of(e).bytes + size_of(e);
    }
  };
  return padding;
}

template <typename T>
concept no_padding =
    std::same_as<std::integral_constant<std::size_t, get_padding<T>()>,
                 std::integral_constant<std::size_t, 0>>;

template <typename T, auto refl>
concept same_as = template_of(^^T) == refl;

} // namespace form

template <> struct std::formatter<std::basic_string_view<char8_t>> {
  template <class ParseContext>
  constexpr ParseContext::iterator parse(ParseContext &ctx) {
    return ctx.begin();
  }
  template <class FmtContext>
  FmtContext::iterator format(std::basic_string_view<char8_t> s,
                              FmtContext &ctx) const {
    return format_to(ctx.out(), "{}", std::string(s.begin(), s.end()));
  }
};
