#include <stdlib.h>  // for EXIT_SUCCESS
#include <iostream>  // for operator<<, endl, basic_ostream, cout, ostream
#include <string>    // for operator==, allocator, basic_string, string

#include "diff.hpp"                            // for diff
#include "log.hpp"                            // for diff
#include "environment.h"                       // for project_version
#include "ftxui/component/captured_mouse.hpp"  // for ftxui

using namespace ftxui;

namespace help {
int main(int argc, const char** argv) {
  (void)argc;
  (void)argv;
  std::cout << "Usage: " << std::endl;
  std::cout << " - git tui diff [args]*" << std::endl;
  std::cout << "" << std::endl;
  return EXIT_SUCCESS;
}
}  // namespace help

namespace version {
int main(int argc, const char** argv) {
  (void)argc;
  (void)argv;
  std::cout << project_version << std::endl;
  return EXIT_SUCCESS;
}
}  // namespace version

int main(int argc, const char** argv) {
  if (argc == 0)
    return help::main(argc, argv);

  // Eat the first argument.
  argc--;
  argv++;

  if (argc == 0)
    return help::main(argc, argv);

  std::string command = argv[0];

  // Eat the second argument.
  argc--;
  argv++;

  if (command == "diff")
    return diff::main(argc, argv);

  if (command == "log")
    return log::main(argc, argv);

  if (command == "version")
    return version::main(argc, argv);

  return help::main(argc, argv);
}

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
