#include "log.hpp"

#include <stdlib.h>   // for EXIT_SUCCESS
#include <algorithm>  // for copy
#include <ftxui/component/component.hpp>  // for Horizontal, Renderer, Button, Menu, CatchEvent, Checkbox, Vertical
#include <ftxui/component/screen_interactive.hpp>  // for ScreenInteractive
#include <ftxui/dom/elements.hpp>  // for operator|, text, separator, Element, size, xflex, vbox, color, yframe, hbox, bold, EQUAL, WIDTH, filler, Elements, nothing, yflex, bgcolor
#include <ftxui/screen/string.hpp>  // for to_wstring, to_string
#include <functional>               // for function
#include <iostream>                 // for basic_istream, char_traits
#include <iterator>                 // for istreambuf_iterator, operator!=
#include <map>                      // for map, map<>::mapped_type
#include <memory>  // for allocator, shared_ptr, __shared_ptr_access, unique_ptr, make_unique
#include <queue>  // for queue
#include <sstream>
#include <string>  // for wstring, basic_string, string, getline, operator+, operator<, to_string
#include <utility>  // for move
#include <vector>   // for vector

#include "diff.hpp"                            // for File, Parse, Render
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/event.hpp"           // for Event
#include "ftxui/screen/color.hpp"  // for Color, Color::Green, Color::Red, Color::Black, Color::White
#include "process.hpp"   // for process
#include "scroller.hpp"  // for Scroller

using namespace ftxui;
namespace gittui::log {

std::string ResolveHead() {
  return ExecuteProcess("git rev-list HEAD -1");
}

struct Commit {
  std::wstring hash;
  std::wstring title;
  std::wstring tree;
  std::vector<std::wstring> authors;
  std::vector<std::wstring> body;
  std::vector<std::wstring> committers;
  std::vector<std::wstring> parents;
};

Commit* GetCommit(std::wstring hash) {
  static std::map<std::wstring, std::unique_ptr<Commit>> g_commit;
  if (g_commit[hash])
    return g_commit[hash].get();

  g_commit[hash] = std::make_unique<Commit>();
  Commit* commit = g_commit[hash].get();
  commit->hash = hash;

  std::string data = ExecuteProcess("git cat-file commit " + to_string(hash));
  std::stringstream ss;
  ss << data;

  std::string line;
  while (std::getline(ss, line)) {
    if (line.find("tree ", 0) == 0) {
      commit->tree = to_wstring(line.substr(5));
      continue;
    }

    if (line.find("parent", 0) == 0) {
      commit->parents.push_back(to_wstring(line.substr(7)));
      continue;
    }

    if (line.find("author", 0) == 0) {
      commit->authors.push_back(to_wstring(line.substr(7)));
      continue;
    }

    if (line.find("committer", 0) == 0) {
      commit->committers.push_back(to_wstring(line.substr(10)));
      continue;
    }

    if (line.empty()) {
      int index = -1;
      while (std::getline(ss, line)) {
        ++index;
        if (index == 0)
          commit->title = to_wstring(line);
        if (index >= 2)
          commit->body.push_back(to_wstring(line));
      }
      break;
    }
  }
  return commit;
}

int main(int argc, const char** argv) {
  auto screen = ScreenInteractive::Fullscreen();

  (void)argc;
  (void)argv;
  std::vector<Commit*> commits;
  std::vector<std::wstring> menu_commit_entries;
  int menu_commit_index = 0;

  std::queue<std::wstring> todo;
  todo.push(to_wstring(ResolveHead()));

  auto refresh_commit_list = [&] {
    while (!todo.empty() && menu_commit_index > (int)commits.size() - 80) {
      std::wstring hash = todo.front();
      todo.pop();

      Commit* commit = GetCommit(hash);
      for (auto& parent : commit->parents)
        todo.push(parent);

      menu_commit_entries.push_back(commit->title);
      commits.push_back(commit);
    }
  };
  refresh_commit_list();

  int hunk_size = 3;
  bool split = true;
  std::vector<diff::File> files;
  std::vector<std::wstring> menu_files_entries;
  int menu_files_index = 0;
  auto refresh_files = [&] {
    Commit* commit = commits[menu_commit_index];
    files.clear();
    menu_files_entries.clear();

    std::string diff =
        ExecuteProcess("git diff -U" + std::to_string(hunk_size) + " " +  //
                       to_string(commit->hash) + "~..." +                 //
                       to_string(commit->hash));

    files = diff::Parse(diff);
    menu_files_entries.push_back(L"description");
    for (const auto& file : files)
      menu_files_entries.push_back(file.right_file);
  };
  refresh_files();

  auto menu_commit_option = MenuOption();
  menu_commit_option.on_change = [&] {
    refresh_files();
    menu_files_index = 0;
  };
  auto menu_commit =
      Menu(&menu_commit_entries, &menu_commit_index, menu_commit_option);
  auto menu_files = Menu(&menu_files_entries, &menu_files_index);

  auto commit_renderer = Renderer([&] {
    if (menu_files_index != 0 && menu_files_index - 1 < (int)files.size())
      return diff::Render(files[menu_files_index - 1], split);

    Commit* commit = commits[menu_commit_index];
    Elements elements;

    for (const auto& committer : commit->committers) {
      elements.push_back(hbox({
          text(L"committer:") | bold | color(Color::Green),
          text(committer) | xflex,
      }));
    }

    for (const auto& author : commit->authors) {
      elements.push_back(hbox({
          text(L"   author:") | bold | color(Color::Green),
          text(author) | xflex,
      }));
    }

    elements.push_back(hbox({
        text(L"     hash:") | bold | color(Color::Green),
        text(commit->hash) | xflex,
    }));

    for (const auto& parent : commit->parents) {
      elements.push_back(hbox({
          text(L"   parent:") | bold | color(Color::Green),
          text(parent) | xflex,
      }));
    }

    elements.push_back(hbox({
        text(L"     tree:") | bold | color(Color::Green),
        text(commit->tree) | xflex,
    }));

    elements.push_back(hbox({
        text(L"    title:") | bold | color(Color::Red),
        text(commit->title) | xflex,
    }));

    elements.push_back(separator());
    if (commit->body.size() != 0) {
      for (auto& it : commit->body)
        elements.push_back(text(it));
    }

    auto content = vbox(std::move(elements));
    return content;
  });

  auto scroller = Scroller(commit_renderer);

  auto increase_hunk = [&] {
    hunk_size++;
    refresh_files();
  };
  auto decrease_hunk = [&] {
    if (hunk_size != 0)
      hunk_size--;
    refresh_files();
  };
  auto split_checkbox = Checkbox("[S]plit", &split);
  auto button_option = ButtonOption();
  button_option.border = false;
  auto button_increase_hunk = Button("[+1]", increase_hunk, button_option);
  auto button_decrease_hunk = Button("[-1]", decrease_hunk, button_option);
  auto button_quit = Button("[Q]uit", screen.ExitLoopClosure(), button_option);

  auto options = Container::Horizontal({
      split_checkbox,
      button_decrease_hunk,
      button_increase_hunk,
      button_quit,
  });

  auto option_renderer = Renderer(options, [&] {
    return hbox({
               text(L"[git tui log]"),
               filler(),
               split_checkbox->Render(),
               text(L"   Context:"),
               button_decrease_hunk->Render(),
               text(to_wstring(hunk_size)),
               button_increase_hunk->Render(),
               filler(),
               button_quit->Render(),
           }) |
           bgcolor(Color::White) | color(Color::Black);
  });

  menu_files = Renderer(menu_files, [menu_files] {
    return vbox({
        text(L"Commit"),
        separator(),
        menu_files->Render() | vscroll_indicator | yframe | yflex,
    });
  });

  menu_commit = Renderer(menu_commit, [menu_commit] {
    return vbox({
        text(L"Files"),
        separator(),
        menu_commit->Render() | vscroll_indicator | yframe | yflex,
    });
  });

  scroller = Renderer(scroller, [scroller] {
    return vbox({
        text(L"Content"),
        separator(),
        scroller->Render() | flex,
    });
  });

  int menu_commit_width = 25;
  int menu_files_width = 25;

  auto container = scroller;
  container = ResizableSplitLeft(menu_files, container, &menu_files_width);
  container = ResizableSplitLeft(menu_commit, container, &menu_commit_width);
  container =
      Renderer(container, [container] { return container->Render() | flex; });
  container = Container::Vertical({
      option_renderer,
      container,
  });

  container = CatchEvent(container, [&](Event event) {
    refresh_commit_list();

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

  menu_commit->TakeFocus();

  screen.Loop(container);
  return EXIT_SUCCESS;
}

}  // namespace gittui::log

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
