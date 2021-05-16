#include "diff.hpp"
#include <assert.h>
#include <ftxui/screen/string.hpp>
#include <iostream>
#include <regex>
#include <sstream>

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
