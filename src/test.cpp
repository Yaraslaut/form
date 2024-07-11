#include "test.h"

int main() {
  form::run_tests<^form::tests>();
  form::run_tests<^form::examples>();
}
