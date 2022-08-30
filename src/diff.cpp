#include "diff.hpp"  // for File, Line, Hunk, Parse, Line::Add, Line::Delete, Line::Keep

#include <assert.h>  // for assert
#include <stdlib.h>  // for EXIT_SUCCESS
#include <iostream>  // for operator<<, stringstream, endl, basic_ios, basic_istream, basic_ostream, cout, ostream
#include <memory>  // for allocator_traits<>::value_type, shared_ptr, __shared_ptr_access
#include <regex>  // for regex_match, match_results, match_results<>::_Base_type, sub_match, regex, smatch
#include <sstream>  // IWYU pragma: keep
#include <string>  // for string, allocator, to_string, basic_string, operator+, char_traits, stoi, getline
#include <utility>  // for move
#include <vector>   // for vector

#include "ftxui/component/component.hpp"  // for Renderer, Button, CatchEvent, Checkbox, Horizontal, Menu, ResizableSplitLeft, Vertical
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/component_options.hpp"   // for ButtonOption
#include "ftxui/component/event.hpp"               // for Event
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/deprecated.hpp"                // for text
#include "ftxui/dom/elements.hpp"  // for operator|, text, separator, vbox, Element, Elements, bgcolor, size, xflex, color, filler, hbox, dim, EQUAL, WIDTH, flex, yflex
#include "ftxui/screen/color.hpp"  // for Color, Color::Black, Color::White
#include "scroller.hpp"            // for Scroller
#include "simple_button_options.hpp"
#include "subprocess/ProcessBuilder.hpp"  // for RunBuilder, run
#include "subprocess/basic_types.hpp"  // for PipeOption, PipeOption::pipe, CompletedProcess, PipeOption::close

#ifdef RGB
// Workaround for wingdi.h (via Windows.h) defining macros that break things.
// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-rgb
#undef RGB
#endif

using namespace ftxui;

namespace gittui::diff {

std::vector<File> Parse(std::string input) {
  std::stringstream ss(input);
  std::string current;
  auto eat = [&]() -> bool { return !!std::getline(ss, current); };
  auto get = [&] { return current; };
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
      files.back().left_file = get().substr(3);
      continue;
    }

    if (start_with("+++") && parse_header) {
      assert(files.size() != 0);
      files.back().right_file = get().substr(6);
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
      files.back().hunks.back().lines.back().content = get().substr(1);
      continue;
    }

    if (start_with("+")) {
      files.back().hunks.back().lines.emplace_back();
      files.back().hunks.back().lines.back().type = Line::Add;
      files.back().hunks.back().lines.back().content = get().substr(1);
      continue;
    }

    if (start_with("-")) {
      files.back().hunks.back().lines.emplace_back();
      files.back().hunks.back().lines.back().type = Line::Delete;
      files.back().hunks.back().lines.back().content = get().substr(1);
      continue;
    }
  }
  return files;
}

Element RenderSplit(const Hunk& hunk) {
  Elements left_line_numbers;
  Elements left_lines;
  Elements right_line_numbers;
  Elements right_lines;
  int left_line_number = hunk.left_start;
  int right_line_number = hunk.right_start;
  auto stabilize = [&] {
    while (left_lines.size() < right_lines.size()) {
      left_lines.push_back(text(L""));
      left_line_numbers.push_back(text(L"~") | dim);
    }
    while (left_lines.size() > right_lines.size()) {
      right_lines.push_back(text(L""));
      right_line_numbers.push_back(text(L"~") | dim);
    }
  };
  for (const Line& line : hunk.lines) {
    switch (line.type) {
      case Line::Keep:
        stabilize();
        left_line_numbers.push_back(text(std::to_string(left_line_number++)));
        right_line_numbers.push_back(text(std::to_string(right_line_number++)));
        left_lines.push_back(text(line.content));
        right_lines.push_back(text(line.content));
        break;

      case Line::Delete:
        left_line_numbers.push_back(text(std::to_string(left_line_number++)));
        left_lines.push_back(text(line.content) |
                             color(Color::RGB(255, 200, 200)) |
                             bgcolor(Color::RGB(128, 0, 0)));
        break;
      case Line::Add:
        right_line_numbers.push_back(text(std::to_string(right_line_number++)));
        right_lines.push_back(text(line.content) |
                              color(Color::RGB(200, 255, 200)) |
                              bgcolor(Color::RGB(0, 128, 0)));
        break;
    }
  }
  stabilize();
  return hbox({
             vbox(std::move(left_line_numbers)),
             separator(),
             vbox(std::move(left_lines)) | size(WIDTH, EQUAL, 800) | xflex,
             separator(),
             vbox(std::move(right_line_numbers)),
             separator(),
             vbox(std::move(right_lines)) | size(WIDTH, EQUAL, 800) | xflex,
         }) |
         xflex;
}

Element RenderJoin(const Hunk& hunk) {
  Elements left_line_numbers;
  Elements right_line_numbers;
  Elements lines;
  int left_line_number = hunk.left_start;
  int right_line_number = hunk.right_start;
  for (const Line& line : hunk.lines) {
    switch (line.type) {
      case Line::Keep:
        left_line_numbers.push_back(text(std::to_string(left_line_number++)));
        right_line_numbers.push_back(text(std::to_string(right_line_number++)));
        lines.push_back(text(line.content));
        break;

      case Line::Delete:
        left_line_numbers.push_back(text(std::to_string(left_line_number++)));
        right_line_numbers.push_back(text(L"~") | dim);
        lines.push_back(text(line.content) | color(Color::RGB(255, 200, 200)) |
                        bgcolor(Color::RGB(128, 0, 0)));
        break;
      case Line::Add:
        left_line_numbers.push_back(text(L"~") | dim);
        right_line_numbers.push_back(text(std::to_string(right_line_number++)));
        lines.push_back(text(line.content) | color(Color::RGB(200, 255, 200)) |
                        bgcolor(Color::RGB(0, 128, 0)));
        break;
    }
  }
  return hbox({
             vbox(std::move(left_line_numbers)),
             separator(),
             vbox(std::move(right_line_numbers)),
             separator(),
             vbox(std::move(lines)) | size(WIDTH, EQUAL, 800) | xflex,
         }) |
         xflex;
}

Element Render(const File& file, bool split) {
  Elements hunks;
  bool is_first = true;
  for (const Hunk& hunk : file.hunks) {
    if (!is_first)
      hunks.push_back(separator());
    is_first = false;
    if (split)
      hunks.push_back(RenderSplit(hunk));
    else
      hunks.push_back(RenderJoin(hunk));
  }
  return vbox(std::move(hunks));
}

int main(int argc, const char** argv) {
  using namespace ftxui;

  std::string args;
  for (int i = 0; i < argc; ++i) {
    args += argv[i];
    args += " ";
  }

  int hunk_size = 3;

  std::vector<File> files;
  std::vector<std::string> file_menu_entries;

  auto refresh_data = [&] {
    files.clear();
    file_menu_entries.clear();

    std::vector<std::string> args = {"git", "diff",
                                     "-U" + std::to_string(hunk_size)};
    for (int i = 0; i < argc; ++i)
      args.push_back(argv[i]);

    auto process = subprocess::run(std::move(args),
                                   subprocess::RunBuilder()                 //
                                       .cerr(subprocess::PipeOption::pipe)  //
                                       .cout(subprocess::PipeOption::pipe)  //
                                       .cin(subprocess::PipeOption::close)  //
    );

    files = Parse(process.cout);
    for (const auto& file : files)
      file_menu_entries.push_back(file.right_file);
  };
  refresh_data();

  if (files.size() == 0) {
    std::cout << "No difference..." << std::endl;
    return EXIT_SUCCESS;
  }

  auto increase_hunk = [&] {
    hunk_size++;
    refresh_data();
  };
  auto decrease_hunk = [&] {
    if (hunk_size != 0)
      hunk_size--;
    refresh_data();
  };
  auto button_option = SimpleButtonOption();
  auto button_increase_hunk = Button("[+1]", increase_hunk, button_option);
  auto button_decrease_hunk = Button("[-1]", decrease_hunk, button_option);

  // File menu.
  int file_menu_selected = 0;
  auto file_menu = Menu(&file_menu_entries, &file_menu_selected);

  auto screen = ScreenInteractive::Fullscreen();
  bool split = true;
  auto split_checkbox = Checkbox("[S]plit", &split);
  auto button_quit = Button("[Q]uit", screen.ExitLoopClosure(), button_option);

  auto scroller = Scroller(
      Renderer([&] { return Render(files[file_menu_selected], split); }));

  auto file_menu_renderer = Renderer(file_menu, [&] {
    return vbox({
        text(L" Files "),
        separator(),
        file_menu->Render(),
    });
  });

  auto file_renderer = Renderer(scroller, [&, file_menu] {
    const File& file = files[file_menu_selected];
    return vbox({
               text(L" Difference "),
               separator(),
               text(file.left_file + " -> " + file.right_file),
               separator(),
               scroller->Render(),
           }) |
           flex;
  });

  int file_menu_width = 30;
  auto layout =
      ResizableSplitLeft(file_menu_renderer, file_renderer, &file_menu_width);

  auto layout_renderer = Renderer(layout, [&] {
    return layout->Render() | yflex;
  });

  auto options = Container::Horizontal({
      split_checkbox,
      button_decrease_hunk,
      button_increase_hunk,
      button_quit
  });

  auto option_renderer = Renderer(options, [&] {
    return hbox({
               text(L"[git tui diff]"),
               filler(),
               split_checkbox->Render(),
               text(L"   Context:"),
               button_decrease_hunk->Render(),
               text(std::to_string(hunk_size)),
               button_increase_hunk->Render(),
               filler(),
               button_quit->Render(),
           }) |
           bgcolor(Color::White) | color(Color::Black);
  });

  auto main_container = Container::Vertical({
      option_renderer,
      layout_renderer,
  });

  auto final_container = CatchEvent(main_container, [&](Event event) {
    if (event == Event::Character('s')) {
      split = !split;
      return true;
    }

    if (event == Event::Character('-')) {
      decrease_hunk();
      return true;
    }

    if (event == Event::Character('+')) {
      increase_hunk();
      return true;
    }

    if (event == Event::Character('q') || event == Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }

    return false;
  });

  file_menu->TakeFocus();

  screen.Loop(final_container);

  return EXIT_SUCCESS;
}

}  // namespace gittui::diff

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
