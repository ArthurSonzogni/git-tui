#ifndef GIT_DIFF_TUI_DIFF_HPP
#define GIT_DIFF_TUI_DIFF_HPP

#include <string>
#include <vector>

struct Line {
  enum Type {
    Keep,
    Add,
    Delete,
  };
  Type type;
  std::wstring content;
};

struct Hunk {
  int left_start;
  int right_start;
  std::vector<Line> lines;
};

struct File {
  std::wstring left_file;
  std::wstring right_file;
  std::vector<Hunk> hunks;
};

std::vector<File> Parse(std::string input);

#endif /* end of include guard: GIT_DIFF_TUI_DIFF_HPP */

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
