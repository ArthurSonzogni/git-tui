#include "version.hpp"

#include <stdlib.h>  // for EXIT_SUCCESS
#include <iostream>  // for operator<<, endl, basic_ostream, cout, ostream

#include "environment.h"  // for project_version

namespace gittui::version {
int main(int argc, const char** argv) {
  (void)argc;
  (void)argv;
  std::cout << project_version << std::endl;
  return EXIT_SUCCESS;
}
}  // namespace gittui::version

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
