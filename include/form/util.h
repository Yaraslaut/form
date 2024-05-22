// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <experimental/meta>
#include <print>
#include <ranges>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

namespace form::util {

template <auto... Xs, typename F> constexpr void for_values(F &&f) {
  (f.template operator()<Xs>(), ...);
}

template <auto B, auto E, typename F> constexpr void for_range(F &&f) {
  using t = std::common_type_t<decltype(B), decltype(E)>;

  [&f]<auto... Xs>(std::integer_sequence<t, Xs...>) {
    for_values<(B + Xs)...>(f);
  }(std::make_integer_sequence<t, E - B>{});
}

template <typename T> consteval auto member_info(size_t n) {
  return nonstatic_data_members_of(^^T,
                                   std::meta::access_context::current())[n];
}

template <typename T> consteval auto base_info(size_t n) {
  return bases_of(^^T, std::meta::access_context::current())[n];
}

template <typename T> consteval auto template_arguments(size_t n) {
  return template_arguments_of(^^T)[n];
}

template <typename T> consteval auto number_of_members() {
  return nonstatic_data_members_of(^^T, std::meta::access_context::current())
      .size();
}

template <typename T> consteval auto number_of_base() {
  return bases_of(^^T, std::meta::access_context::current()).size();
}

namespace __impl {
template <auto... vals> struct replicator_type {
  template <typename F> constexpr void operator>>(F body) const {
    (body.template operator()<vals>(), ...);
  }
};

template <auto... vals> replicator_type<vals...> replicator = {};
} // namespace __impl

template <typename R> consteval auto expand(R range) {
  std::vector<std::meta::info> args;
  for (auto r : range) {
    args.push_back(reflect_constant(r));
  }
  return substitute(^^__impl::replicator, args);
}

namespace detail {

template <auto refl> void run_tests() {
  std::vector<std::thread> pool;
  // clang-format off
  [:util::expand(members_of(refl, std::meta::access_context::current())):] >> [&]<auto mem>
  // clang-format on
          requires std::invocable<typename[:type_of(mem):]>
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
      std::format_to(std::back_inserter(out), "{}",
                     std::meta::identifier_of(mem));
      std::println(" {}", out);
    }));
  };

  for (auto &t : pool) {
    t.join();
  }
}

} // namespace detail

} // namespace form::util
