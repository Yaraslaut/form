# form

> Collection of static reflection usage examples 

Collection utilize existing c++26 reflection support from [clang-p2996](https://github.com/bloomberg/clang-p2996/tree/p2996) 

To test it you can use provided Dockerfile to get compiler and build project 

``` sh
docker build . --progress=plain
```

Some additional examples can be found in [proposal](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r3.html)

## Create variant of all types inside namespace

``` c++
namespace list {
struct CancelSelection {};
struct ClearHistoryAndReset {};
} // namespace list
using list_variant = [:form::util::create_variant(^list):];
```

## Enum/Variant to string 

``` c++

enum class Color { red, green, blue };

void VariantToString() {
  list_variant v{list::CancelSelection{}};
  std::println("{}", form::variant_type_to_string(v)); // CancelSelection
}

void EnumToString() { 
  std::println("{}", form::enum_to_string(Color::red));  // red
}

```

## Universal formatter



``` c++

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


template <typename T>
  requires form::util::is_one_of<T, Z>
struct std::formatter<T> : std::formatter<std::string> {
  auto format(const T &val, std::format_context &ctx) const {
    return std::format_to(ctx.out(), "{}",
                          form::universal_formatter::format(val));
  }
};


int main() {
  std::println("{}", Z()); // Z{X{.m1=1}, Y{.m2=2}, .m3=3, .m4=4}
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
}

/*
===== JSON =====
{"live":false,"v":90,"b":90,page_size: {lines: {"value":10},columns: {"value":10}}}
===== YAML =====
live: "false"
v: "90"
b: "90"
page_size:
  lines:
    value: "10"
  columns:
    value: "10"
*/
```


## Deserialization into different formats

Only YAML supported at the moment

``` c++

int main() {
  Config c;

  auto yaml_input = form::format_yaml(c);

  auto from_yaml = form::from_yaml<Config>(yaml_input);

  return form::compare(c,from_yaml);
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

void runTests() { form::run_tests<^for_tests>(); }

/*
 🔥 test_true
 💩 test_false
 🍀 test_void
*/

```

`
