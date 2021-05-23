#include "diff.hpp"

#include <assert.h>                 // for assert
#include <algorithm>                // for max
#include <ftxui/screen/string.hpp>  // for to_wstring
#include <iostream>  // for stringstream, basic_ios, basic_istream
#include <memory>    // for allocator_traits<>::value_type
#include <regex>  // for regex_match, match_results, match_results<>::_Base_type, sub_match, regex, smatch

std::vector<File> Parse(std::string input) {
  std::stringstream ss(input);
  std::string current;
  auto eat = [&]() -> bool { return !!std::getline(ss, current); };
  auto get = [&] { return current; };
  auto getw = [&] { return ftxui::to_wstring(get()); };
  auto start_with = [&](const char* prefix) {
    return get().rfind(prefix, 0) == 0;
  };

  bool parse_header = true;
  std::vector<File> files;
  while (eat()) {
    if (start_with("diff")) {
      parse_header = true;
      files.emplace_back();
      continue;
    }

    if (start_with("index")) {
      assert(parse_header);
      continue;
    }

    if (start_with("---") && parse_header) {
      assert(files.size() != 0);
      files.back().left_file = getw().substr(3);
      continue;
    }

    if (start_with("+++") && parse_header) {
      assert(files.size() != 0);
      files.back().right_file = getw().substr(3);
      continue;
    }

    if (start_with("@@")) {
      assert(files.size() != 0);
      files.back().hunks.emplace_back();
      static std::regex regex(R"(@@ -(\d+)(?:,\d+)? \+(\d+)(?:,\d+)? @@.*)");
      std::smatch match;
      bool matched = std::regex_match(current, match, regex);
      assert(matched);
      (void)matched;
      files.back().hunks.back().left_start = std::stoi(match[1].str());
      files.back().hunks.back().right_start = std::stoi(match[2].str());
      parse_header = false;
      continue;
    }

    if (start_with(" ")) {
      files.back().hunks.back().lines.emplace_back();
      files.back().hunks.back().lines.back().type = Line::Keep;
      files.back().hunks.back().lines.back().content = getw().substr(1);
      continue;
    }

    if (start_with("+")) {
      files.back().hunks.back().lines.emplace_back();
      files.back().hunks.back().lines.back().type = Line::Add;
      files.back().hunks.back().lines.back().content = getw().substr(1);
      continue;
    }

    if (start_with("-")) {
      files.back().hunks.back().lines.emplace_back();
      files.back().hunks.back().lines.back().type = Line::Delete;
      files.back().hunks.back().lines.back().content = getw().substr(1);
      continue;
    }
  }
  return files;
}

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
