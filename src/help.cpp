#include "help.hpp"

#include <stdlib.h>  // for EXIT_SUCCESS
#include <iostream>  // for operator<<, endl, basic_ostream, cout, ostream

namespace gittui::help {
int main(int argc, const char** argv) {
  (void)argc;
  (void)argv;
  std::cout << R"(
git tui is a terminal user interface for git.

Usage:
  - git tui <command> [args]*

Available TUI commands:
  - diff
  - log

Fallback to git:
  - git 
  - ... (fallback to git)

Other commands:
  - version
  - help

)" << std::endl;

  return EXIT_SUCCESS;
}
}  // namespace gittui::help

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
