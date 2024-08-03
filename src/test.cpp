#include "test.h"

int main() {
  form::run_tests<^form::examples>();
  form::run_tests<^form::tests>();
  return 0;
}
