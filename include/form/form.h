// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <algorithm>
#include <concepts>
#include <form/format.h>
#include <format>
#include <iterator>
#include <print>
#include <thread>
#include <type_traits>

// clang-format off

namespace form {

namespace detail {
using namespace form;

template <typename Inner>
constexpr Inner string_to_enum(std::string_view value) {
   Inner result{};
   constexpr auto enum_type = []<typename Check>(this auto self, Check)
   {
      if constexpr (std::is_enum_v<Check>) {
         return ^^Check;
      } else {
         return self(typename Check::value_type{});
      }
   }(Inner{});

   [:util::expand(std::meta::enumerators_of(enum_type)):] >> [&]<auto e> {
      if (value == std::meta::identifier_of(e)) {
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
      std::format_to(std::back_inserter(out), "{}", std::meta::identifier_of(e));
    }
  };
  return out;
}

template<typename E, bool Enumerable = std::meta::is_enumerable_type(^^E)>
  requires std::is_enum_v<E>
constexpr std::string_view enum_to_string(E value) {
  if constexpr (Enumerable)
    template for (constexpr auto e :
                  std::define_static_array(std::meta::enumerators_of(^^E)))
      if (value == [:e:])
        return std::meta::identifier_of(e);

  return "<unnamed>";
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
      template for(constexpr auto mem : std::define_static_array(nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())))
      {
         if (!compare(lhs.[:mem:], rhs.[:mem:]))
         {
            result = result && false;
         }
      }
   return result;
}

template <typename S> consteval std::size_t get_padding() {

   std::size_t pointer{static_cast<size_t>(offset_of(nonstatic_data_members_of(^^S, std::meta::access_context::unchecked())[0]).bytes)};
   std::size_t padding{0};

   template for(constexpr auto e: std::define_static_array(nonstatic_data_members_of(^^S, std::meta::access_context::unchecked())))
   {
      if (pointer == offset_of(e).bytes)
         pointer += size_of(e);
      else {
         padding += offset_of(e).bytes - pointer;
         pointer = offset_of(e).bytes + size_of(e);
      }
   }
   return padding;
}

template <typename T>
concept no_padding =
   std::same_as<std::integral_constant<std::size_t, get_padding<T>()>,
                std::integral_constant<std::size_t, 0>>;

consteval auto create_variant(auto reflection) {
   return substitute(
      ^^std::variant,
      std::vector{
         members_of(reflection, std::meta::access_context::current())});
}


template<auto F>
auto invoke() {
   static constexpr auto params = std::define_static_array(parameters_of(F));
   typename [:return_type_of(F):] result;
   [&result]<size_t...I>(std::index_sequence<I...>) {
      result = [:F:](
         [] consteval {
            constexpr auto type_reflection = std::meta::remove_cvref(std::meta::type_of(params[I]));
            if constexpr (std::meta::is_convertible_type(type_reflection, ^^std::string_view))
               return identifier_of(params[I]);
            else
               return typename [:type_reflection:]{};
         }()...
      );
   }(std::make_index_sequence<params.size()>{});
   return result;
}

} // namespace form
