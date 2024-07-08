#include "test.h"

template <typename T>
  requires form::util::is_one_of<T, Z>
struct std::formatter<T> : std::formatter<std::string> {
  auto format(const T &val, std::format_context &ctx) const {
    return std::format_to(ctx.out(), "{}", form::format_univ(val));
  }
};

int main() {
  form::run_tests<^form::tests>();
  form::run_tests<^form::round_trip_tests>();
  form::run_tests<^form::examples>();
}
