// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <experimental/meta>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <vector>

namespace form::util {

consteval auto name_of(auto refl) {
  if (has_identifier(refl)) {
    return identifier_of(refl);
  } else {
    return display_string_of(refl);
  }
}

template <typename... Ts, typename F> constexpr void enumerate_types(F &&f) {
  [&f]<auto... Is>(std::index_sequence<Is...>) {
    (f.template operator()<Ts, Is>(), ...);
  }(std::index_sequence_for<Ts...>{});
}

template <auto... Xs, typename F> constexpr void for_values(F &&f) {
  (f.template operator()<Xs>(), ...);
}

template <auto B, auto E, typename F> constexpr void for_range(F &&f) {
  using t = std::common_type_t<decltype(B), decltype(E)>;

  [&f]<auto... Xs>(std::integer_sequence<t, Xs...>) {
    for_values<(B + Xs)...>(f);
  }(std::make_integer_sequence<t, E - B>{});
}

template <typename T> consteval auto member_info(int n) {
  return nonstatic_data_members_of(^T)[n];
}

template <typename T> consteval auto base_info(int n) {
  return bases_of(^T)[n];
}

template <typename T> consteval auto template_arguments(int n) {
  return template_arguments_of(^T)[n];
}

template <typename T> consteval auto number_of_members() {
  return nonstatic_data_members_of(^T).size();
}

template <typename T> consteval auto number_of_base() {
  return bases_of(^T).size();
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
    args.push_back(reflect_value(r));
  }
  return substitute(^__impl::replicator, args);
}

template <typename T, typename F> T construct_from(F f) { return T{f}; }

/**
 *
 * reflection of namespace
 * using variant_type = [:create_variant(^namespace):];
 *
 **/
consteval auto create_variant(auto reflection) {

  return substitute(^std::variant, std::vector{
                                       members_of(reflection)});
}

template <typename Check, typename... T>
concept is_one_of = std::disjunction_v<std::is_same<Check, T>...>;

std::string_view get_after(std::string_view str, std::string_view delim) {
  auto pos = str.find(delim);
  if (pos == std::string_view::npos) {
    return str;
  }
  return str.substr(pos + delim.size());
}

std::string_view get_before(std::string_view str, std::string_view delim) {
  auto pos = str.find(delim);
  if (pos == std::string_view::npos) {
    return str;
  }
  return str.substr(0, pos);
}

} // namespace form::util
