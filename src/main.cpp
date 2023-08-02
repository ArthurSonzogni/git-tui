#include <string>  // for operator==, allocator, basic_string, string

#include "diff.hpp"     // for main
#include "help.hpp"     // for main
#include "log.hpp"      // for main
#include "version.hpp"  // for main
#include <cstdlib>      // for main

// redirect all non implemented git commands to git
void go_through_git(int argc, const char** argv, std::string command){
  std::string full_git_cmd = "git ";
  full_git_cmd.append(command);
  
  for (int i = 0; i <  argc; i++){
    full_git_cmd.append(" ");
    full_git_cmd.append(argv[i]);
  }
  std::system(full_git_cmd.c_str());
}


int main(int argc, const char** argv) {
  if (argc == 0)
    return gittui::help::main(argc, argv);

  // Eat the first argument.
  argc--;
  argv++;

  if (argc == 0)
    return gittui::help::main(argc, argv);

  std::string command = argv[0];

  // Eat the second argument.
  argc--;
  argv++;

  if (command == "diff")
    return gittui::diff::main(argc, argv);

  if (command == "log")
    return gittui::log::main(argc, argv);

  if (command == "version")
    return gittui::version::main(argc, argv);

  go_through_git(argc, argv, command);
  
}

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
