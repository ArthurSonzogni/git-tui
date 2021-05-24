#include <string>  // for operator==, allocator, basic_string, string

#include "diff.hpp"     // for main
#include "help.hpp"     // for main
#include "log.hpp"      // for main
#include "version.hpp"  // for main

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
