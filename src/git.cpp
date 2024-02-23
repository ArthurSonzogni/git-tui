#include "git.hpp"

#include <string>   // for string, allocator, basic_string
#include <utility>  // for move
#include <vector>   // for vector

#include "subprocess/ProcessBuilder.hpp"  // for run
#include "subprocess/basic_types.hpp"     // for CompletedProcess

namespace gittui::git {
int main(int argc, const char** argv) {
  std::vector<std::string> args = {
      "git",
  };

  for (int i = 0; i < argc; ++i)
    args.push_back(argv[i]);

  return subprocess::run(std::move(args)).returncode;
}
}  // namespace gittui::git

// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
