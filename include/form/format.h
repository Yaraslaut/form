#pragma once
#include <form/util.h>
#include <format>
#include <type_traits>
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
auto withLevelKnown() { return withLevel("{}: {}"); }
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
auto withLevelNested() { return "\"{}\": {}"; }
void increase_level() {}
void decrease_level() {}

} // namespace json

namespace universal {

constexpr auto delimiter() { return ","; }
constexpr auto start() { return "{"; }
constexpr auto end() { return "}"; }
auto withLevelKnown() { return ".{}={}"; }
auto withLevelNested() { return ".{}={}"; }
void increase_level() {}
void decrease_level() {}

} // namespace universal

namespace yaml {

struct Node {

  std::string_view _context;

  Node(std::string_view c) : _context{c} {
  }

  template <typename T> T as() {

    try {
      return as_impl<T>();
    } catch (std::exception &e) {
      std::string error = e.what();
      std::string_view error_view = error;
      std::println("Error while parsing value: {} with context {}", error_view,
                   _context);
      return T{};
    }
  }

  template <typename T> T as_impl() {
    if constexpr (std::is_same_v<T, std::string>) {
      return std::string(_context);
    } else if constexpr (std::is_same_v<T, std::string_view>) {
      return _context;
    } else if constexpr (std::is_same_v<T, int>) {
      return std::stoi(std::string(_context));
    } else if constexpr (std::is_same_v<T, unsigned int>) {
      return std::stoul(std::string(_context));
    } else if constexpr (std::is_same_v<T, double>) {
      return std::stod(std::string(_context));
    } else if constexpr (std::is_same_v<T, bool>) {
      return std::string(_context) == "true";
    } else {
      throw std::runtime_error(
          std::format("Unsupported type {}", util::name_of(^T)));
    }
  }

  Node operator[](std::string_view name_raw) const {
    using namespace std::literals;

    std::string name = std::string{name_raw} + ":";

    auto lines = std::views::split(_context, '\n');

    auto root_lines = lines | std::views::filter([](auto line) {
                        return !std::string_view{line}.starts_with(' ');
                      });

    auto found_in_root = std::ranges::find_if(root_lines, [&](auto line) {
      return std::string_view{line}.starts_with(name);
    });

    auto found_value = util::get_before(
        util::get_after(std::string_view{*found_in_root}, ":"sv), "\n"sv);

    // primitive type and entry is "name: value"
    if (found_value.size() > 0) {
      return Node{found_value};
    }

    const auto found_element_in_all_lines =
        std::ranges::find_if(lines, [&](auto line) {
          return std::string_view{line}.starts_with(name);
        });
    const auto next_element_in_all_lines =
        std::ranges::find_if(lines, [&](auto line) {
          return std::string_view{line}.starts_with(
              std::string_view{*(std::next(found_in_root))});
        });

    // get lines between found and next
    const auto found_index =
        std::distance(lines.begin(), found_element_in_all_lines);
    auto next_index = std::distance(lines.begin(), next_element_in_all_lines);

    // if next element is not found, it is the last element in roots
    if (next_index == 0)
      next_index = std::distance(lines.begin(), lines.end());

    auto found_element = lines | std::views::drop(found_index + 1) |
                         std::views::take(next_index - found_index - 1) |
                         std::views::transform([](auto line_view) {
                           auto line = std::string_view{line_view};
                           if (line.size() > 2)
                             return line.substr(2);
                           return line;
                         });

    std::string folded;
    for (const auto line : found_element) {
      folded += std::string{std::string_view{line}} + "\n";
    }
    return Node{folded};
  }
};

} // namespace yaml

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

    if constexpr (std::is_arithmetic_v<[:type_of(mem):]>) {
      std::format_to(std::back_inserter(out),
                     std::runtime_format([:refl:] ::withLevelKnown()), name,
                     t.[:mem:]);
    } else if constexpr (std::same_as<[:type_of(mem):], std::string>) {
      std::format_to(std::back_inserter(out),
                     std::runtime_format([:refl:] ::withLevelKnown()), name,
                     std::format("\"{}\"", t.[:mem:]));
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

template <typename T> void from_node(auto const &node, T &t) {

  util::for_range<0, util::number_of_members<T>()>([&]<auto I>() {
    constexpr auto mem = util::member_info<T>(I);
    auto name = std::string{util::name_of(mem)};
    // std::println("get value of: {}", name_of(mem));
    if constexpr (std::is_arithmetic_v<[:type_of(mem):]> ||
                  std::same_as<[:type_of(mem):], std::string>) {
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
      from_node(node[name], t.[:mem:]);
    }
  });

  util::for_range<0, util::number_of_base<T>()>([&]<auto I>() {
    constexpr auto base = util::base_info<T>(I);
    if constexpr (is_accessible(base))
      util::for_range<0, util::number_of_members<[:type_of(base):]>()>(
          [&]<auto II>() {
            constexpr auto mem = util::member_info<[:type_of(base):]>(II);
            if constexpr (std::is_arithmetic_v<[:type_of(mem):]> ||
                          std::same_as<[:type_of(mem):], std::string>) {
              auto name = std::string{util::name_of(mem)};
              t.[:mem:] = node[name].template as<[:type_of(mem):]>();
            } else if constexpr (std::formattable<[:type_of(mem):], char>) {
            } else {
              from_node(node[util::name_of(mem)], t.[:mem:]);
            }
          });
  });
}

template <typename T> std::string format_yaml(T const &t) {
  return format<^yaml>(t);
}

template <typename T> std::string format_json(T const &t) {
  return format<^json>(t);
}

template <typename T> std::string format_universal(T const &t) {
  return format<^universal>(t);
}

template <typename T> T from_yaml(auto input) {
  auto node = yaml::Node(input);
  T t;
  from_node(node, t);
  return t;
}

} // namespace form
