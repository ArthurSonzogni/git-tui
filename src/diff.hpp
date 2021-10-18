#ifndef GIT_DIFF_TUI_DIFF_HPP
#define GIT_DIFF_TUI_DIFF_HPP

#include <ftxui/dom/elements.hpp>  // for Element
#include <string>                  // for string, string
#include <vector>                  // for vector

#include "ftxui/screen/box.hpp"  // for ftxui

namespace gittui::diff {
using namespace ftxui;

struct Line {
  enum Type {
    Keep,
    Add,
    Delete,
  };
  Type type;
  std::string content;
};

struct Hunk {
  int left_start;
  int right_start;
  std::vector<Line> lines;
};

struct File {
  std::string left_file;
  std::string right_file;
  std::vector<Hunk> hunks;
};

std::vector<File> Parse(std::string input);
Element RenderSplit(const Hunk& hunk);
Element RenderJoin(const Hunk& hunk);
Element Render(const File& file, bool split);
int main(int argc, const char** argv);

}  // namespace gittui::diff

#endif /* end of include guard: GIT_DIFF_TUI_DIFF_HPP */

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
