#include "test.h"

int main() {
  form::run_tests<^form::tests>();
  form::run_tests<^form::tests>();
  form::run_tests<^form::round_trip_tests>();
  form::run_tests<^form::examples>();
  std::println("{}", Z());
}
