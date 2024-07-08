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

template <typename E> constexpr std::string variant_type_to_string(E value) {
  std::string out = "";
  [:util::expand(template_arguments_of(^E)):] >> [&]<auto e> {
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
  [:util::expand(std::meta::enumerators_of(^E)):] >> [&]<auto e> {
    if (value == [:e:]) {
      result = util::name_of(e);
    }
  };
  return result;
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
  [:util::expand(members_of(refl)):] >> [&]<auto mem> {
    pool.emplace_back(std::thread([&]() { [:mem:](); }));
  };

  for (auto &t : pool) {
    t.join();
  }
}

template <auto refl> void run_seq() {
  [:util::expand(members_of(refl)):] >> [&]<auto mem> { [:mem:](); };
}

template <auto refl> void run_tests() {
  std::vector<std::thread> pool;
  [:util::expand(members_of(refl)
                 ):] >> [&]<auto mem>
                       requires std::invocable<typename[:type_of(mem):]>
  {
    pool.emplace_back(std::thread([&]() {
      std::string out;
      if constexpr (std::is_same_v<
                        std::invoke_result_t<typename[:type_of(mem):]>, bool>) {
        if ([:mem:]())
          std::format_to(std::back_inserter(out), "{}", "\N{FIRE}");
        else
          std::format_to(std::back_inserter(out), "{}", "\N{PILE OF POO}");
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

template <typename T> constexpr void print_members() {
  std::println("Members of: {}", util::name_of(^T));
  [:util::expand(nonstatic_data_members_of(^T)):] >> [&]<auto mem> {
    std::println("{}", util::name_of(mem));
  };
}

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
