#include "test.h"

int main() {
  form::util::detail::run_tests<^^form::examples>();
  form::util::detail::run_tests<^^form::tests>();
  return 0;
}
