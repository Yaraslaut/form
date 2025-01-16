# form

> Collection of static reflection usage examples 

- [Create variant of all types inside the namespace](https://github.com/Yaraslaut/form#create-variant-of-all-types-inside-the-namespace)
- [Enum to string](https://github.com/Yaraslaut/form#enum-to-string)
- Getting Class Layout [P2996 paper](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r3.html#getting-class-layout)
- List of Types to List of Sizes [P2996 paper](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r3.html#list-of-types-to-list-of-sizes)
- Named Tuple [P2996 paper](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r3.html#named-tuple) 
- [Padding check at compile time](https://github.com/Yaraslaut/form#padding-check-at-compile-time)
- Parsing Command-Line options [P2996 paper](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r3.html#parsing-command-line-options-ii)
- [Run all function from namespace in serial or parallel](https://github.com/Yaraslaut/form#run-all-function-from-namespace-in-serial-or-parallel)
- [same_as concept for templates](https://github.com/Yaraslaut/form?tab=readme-ov-file#same_as-concept-for-templates)
- [Serialization/Deserialization into different formats](https://github.com/Yaraslaut/form#serialization-into-different-formats) 
- [String to enum](https://github.com/Yaraslaut/form#string-to-enum)
- Struct to struct of Arrays [P2996 paper](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r3.html#struct-to-struct-of-arrays)
- [Variant type to string](https://github.com/Yaraslaut/form#variant-type-to-string)

Collection utilize existing c++26 reflection support from [clang-p2996](https://github.com/bloomberg/clang-p2996/tree/p2996) 

To test it you can use provided Dockerfile to get compiler and build project 

``` sh
docker build . --progress=plain
```



## Enum to string 

Transform enum directly to std::string

``` c++

enum class Color { red, green, blue };

void EnumToString() { 
  std::println("{}", form::enum_to_string(Color::red));  // red
}

```

Use lambda to format enum values following your pattern.

``` c++
void EnumToString() { 
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
  form::enum_to_string(Decorator::Underline, transform) //  underline
  form::enum_to_string(Decorator::DottedUnderline, transform) // dotted-underline
}
```


## String to enum
Transform string directly to enum

``` c++

enum class Color { red, green, blue };

void EnumToString() {
  auto color = form::string_to_enum<Color>("red").value();
}

```


## Padding check at compile time

Calculate padding at compile time and enforce zero padding via concepts

``` c++
struct struct_with_padding {
  char a;
  double c;
};

struct struct_no_padding {
  double c;
  char a;
};

template <form::no_padding T> void foo(T t) { 
  std::println("Without padding"); 
}

template <typename T> void foo(T t) {
  std::println("With padding: {}", form::get_padding<T>());
}


void PaddingCheck() {
  foo(struct_with_padding{}); // With padding: 7
  foo(struct_no_padding{});   // Without padding
  static_assert(form::no_padding<struct_no_padding>);
}

```

## Create variant of all types inside the namespace

``` c++
namespace list {
struct CancelSelection {};
struct ClearHistoryAndReset {};
} // namespace list
using list_variant = [:form::util::create_variant(^list):];
```

## Variant type to string

``` c++

void VariantToString() {
  list_variant v{list::CancelSelection{}};
  std::println("{}", form::variant_type_to_string(v)); // CancelSelection
}
```

## Serialization into different formats 

```c++

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


void SerializationIntoDifferentFormats() {
  Config c;
  std::println("===== JSON =====");
  std::println("{}", form::format_json(c));
  std::println("===== YAML =====");
  std::println("{}", form::format_yaml(c));
  std::println("===== UNIVERSAL =====");
  std::println("{}", form::format_universal(c));
}

/*
===== JSON =====
{"live":false,"v":90,"b":90,page_size: {lines: {"value":10},columns: {"value":10}}}
===== YAML =====
live: false
v: 90
b: 90
page_size:
  lines:
    value: 10
  columns:
    value: 10
===== UNIVERSAL =====
{.live=false,.v=90,.b=90,.page_size={.lines={.value=10},.columns={.value=10}}}
*/
```


## Deserialization from different formats

Only YAML supported at the moment

``` c++

int fromYAML() {
  Config c;

  auto yaml_input = form::format_yaml(c);

  auto from_yaml = form::from_yaml<Config>(yaml_input);
}

```

## Run all function from namespace in serial or parallel

To run function in serial use `form::run_seq<^namespace>()` for parallel `form::run_par<^namespace>()`

Example usage of similar technique for running tests
``` c++

namespace for_tests {
bool test_true() { return true; }
bool test_false() { return false; }
void test_void() {};
} // namespace for_tests

void runTests() { form::run_tests<^^for_tests>(); }

/*
 🔥 test_true
 💩 test_false
 🍀 test_void
*/

```

## same_as concept for templates

concept `form::same_as` checks if type represent specific template 

``` c++

template <typename T> int foo(T) { return -1; }

template <typename T>
  requires form::same_as<T, ^std::complex>
int foo(T) {
  return 1;
}

foo(std::complex<float>{1.0f,1.0f}); //1

```
