#pragma once
#include <form/util.h>
#include <format>
#include <yaml-cpp/yaml.h>

namespace form {

namespace yaml {

int yaml_indent = 0;
auto withLevel(std::string const in, int indent) {
  auto out = std::string(indent * 2, ' ');
  out += in;
  return out;
}
auto withLevel(std::string const in) { return withLevel(in, yaml_indent); }
auto withLevelKnown() { return withLevel("{}: \"{}\""); }
auto withLevelNested() { return withLevel("{}:\n{}"); }
void increase_level() { yaml_indent += 1; }
void decrease_level() { yaml_indent -= 1; }
constexpr auto delimiter() { return "\n"; }
constexpr auto start() { return ""; }
constexpr auto end() { return ""; }

} // namespace yaml

namespace json {

constexpr auto delimiter() { return ","; }
constexpr auto start() { return "{"; }
constexpr auto end() { return "}"; }
auto withLevelKnown() { return "\"{}\":{}"; }
auto withLevelNested() { return "{}: {}"; }
void increase_level() {}
void decrease_level() {}

} // namespace json

namespace universal_formatter {

template <typename T> std::string format(T const &t) {
  std::string formatted;
  auto out = std::back_inserter(formatted);
  std::format_to(out, "{}{{", name_of<std::string_view>(^T));

  auto delim = [first = true, &out]() mutable {
    if (!first) {
      *out++ = ',';
      *out++ = ' ';
    }
    first = false;
  };

  [:util::expand(bases_of(^T)):] >> [&]<auto base> {
    delim();
    std::format_to(out, "{}", format((typename[:type_of(base):] const &)(t)));
  };

  [:util::expand(nonstatic_data_members_of(^T)):] >> [&]<auto mem> {
    delim();
    std::format_to(out, ".{}={}", name_of<std::string_view>(mem), t.[:mem:]);
  };

  *out++ = '}';
  return formatted;
}

}; // namespace universal_formatter

template <auto refl, typename T> std::string format(T const &t) {
  std::string out;

  auto delim = [&, first = true]() mutable {
    if (!first) {
      std::format_to(std::back_inserter(out), [:refl:] ::delimiter());
    }
    first = false;
  };

  std::format_to(std::back_inserter(out), "{}", [:refl:] ::start());
  util::for_range<0, util::number_of_base<T>()>([&]<auto I>() {
    constexpr auto base = util::base_info<T>(I);
    if constexpr (is_accessible(base)) {
      delim();
      std::format_to(std::back_inserter(out), std::runtime_format("{}"),
                     format<refl>(static_cast<[:type_of(base):] const &>(t)));
    }
  });

  util::for_range<0, util::number_of_members<T>()>([&]<auto I>() {
    constexpr auto mem = util::member_info<T>(I);
    constexpr auto name = util::name_of(mem);
    constexpr auto type = type_of(mem);
    delim();

    // std::println("Handling member: {} with type: {}", name,
    //              name_of(type_of(mem)));
    if constexpr (util::is_trivial_type<[:type_of(mem):]>) {
      std::format_to(std::back_inserter(out),
                     std::runtime_format([:refl:] ::withLevelKnown()), name,
                     t.[:mem:]);
    } else if constexpr (std::is_constructible_v<
                             std::formatter<[:type_of(mem):]>>) {
      std::format_to(std::back_inserter(out),
                     std::runtime_format([:refl:] ::withLevelKnown()), name,
                     std::format("{}", t.[:mem:]));
    } else {
      std::format_to(std::back_inserter(out),
                     std::runtime_format([:refl:] ::withLevelNested()), name,
                     [&]() {
                       [:refl:] ::increase_level();
                       return format<refl>(t.[:mem:]);
                     }());
      [:refl:] ::decrease_level();
    }
  });
  std::format_to(std::back_inserter(out), "{}", [:refl:] ::end());

  return out;
}

template <typename T> void from_yaml_node(YAML::Node const &node, T &t) {

  util::for_range<0, util::number_of_members<T>()>([&]<auto I>() {
    constexpr auto mem = util::member_info<T>(I);
    auto name = std::string{util::name_of(mem)};
    // std::println("get value of: {}", name_of(mem));
    if constexpr (util::is_trivial_type<[:type_of(mem):]>) {
      t.[:mem:] = node[name].template as<[:type_of(mem):]>();
    } else if constexpr (std::is_constructible_v<
                             std::formatter<[:type_of(mem):]>>) {
      // std::println("get value from std::format {}", name);
      constexpr auto type_of_mem =
          util::template_arguments<[:type_of(mem):]>(0);
      t.[:mem:] = util::construct_from<[:type_of(mem):]>(
                    node[name].template as<[:type_of_mem:]>());
    } else {
      // std::println("diving into: {}", name);
      from_yaml_node(node[name], t.[:mem:]);
    }
  });

  util::for_range<0, util::number_of_base<T>()>([&]<auto I>() {
    constexpr auto base = util::base_info<T>(I);
    if constexpr (is_accessible(base))
      util::for_range<0, util::number_of_members<[:type_of(base):]>()>(
          [&]<auto II>() {
            constexpr auto mem = util::member_info<[:type_of(base):]>(II);
            if constexpr (util::is_trivial_type<[:type_of(mem):]>) {
              auto name = std::string{util::name_of(mem)};
              t.[:mem:] = node[name].template as<[:type_of(mem):]>();
            } else if constexpr (std::formattable<[:type_of(mem):], char>) {
            } else {
              from_yaml_node(node[util::name_of(mem)], t.[:mem:]);
            }
          });
  });
}

} // namespace form
