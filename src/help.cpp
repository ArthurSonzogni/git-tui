#include "help.hpp"

#include <stdlib.h>  // for EXIT_SUCCESS
#include <iostream>  // for operator<<, endl, basic_ostream, cout, ostream

namespace help {
int main(int argc, const char** argv) {
  (void)argc;
  (void)argv;
  std::cout << "Usage: " << std::endl;
  std::cout << " - git tui diff [args]*" << std::endl;
  std::cout << " - git tui log [args]*" << std::endl;
  std::cout << "" << std::endl;
  return EXIT_SUCCESS;
}
}  // namespace help

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
