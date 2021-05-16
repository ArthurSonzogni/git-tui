#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include "diff.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/screen.hpp"
#include "ftxui/screen/string.hpp"

using namespace ftxui;

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

Element RenderSplit(const Hunk& hunk) {
  Elements left_line_numbers;
  Elements left_lines;
  Elements right_line_numbers;
  Elements right_lines;
  int left_line_number = hunk.left_start;
  int right_line_number = hunk.right_start;
  auto stabilize = [&]{
    while(left_lines.size() < right_lines.size()) {
      left_lines.push_back(text(L""));
      left_line_numbers.push_back(text(L"~") | dim);
    }
    while(left_lines.size() > right_lines.size()) {
      right_lines.push_back(text(L""));
      right_line_numbers.push_back(text(L"~") | dim);
    }
  };
  for(const Line& line : hunk.lines) {
    switch(line.type) {
      case Line::Keep:
        stabilize();
        left_line_numbers.push_back(text(to_wstring(left_line_number++)));
        right_line_numbers.push_back(text(to_wstring(right_line_number++)));
        left_lines.push_back(text(line.content));
        right_lines.push_back(text(line.content));
        break;

      case Line::Delete:
        left_line_numbers.push_back(text(to_wstring(left_line_number++)));
        left_lines.push_back(text(line.content) |
                             color(Color::RGB(255, 200, 200)) |
                             bgcolor(Color::RGB(128, 0, 0)));
        break;
      case Line::Add:
        right_line_numbers.push_back(text(to_wstring(right_line_number++)));
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
  for(const Line& line : hunk.lines) {
    switch(line.type) {
      case Line::Keep:
        left_line_numbers.push_back(text(to_wstring(left_line_number++)));
        right_line_numbers.push_back(text(to_wstring(right_line_number++)));
        lines.push_back(text(line.content));
        break;

      case Line::Delete:
        left_line_numbers.push_back(text(to_wstring(left_line_number++)));
        right_line_numbers.push_back(text(L"~") | dim);
        lines.push_back(text(line.content) |
                             color(Color::RGB(255, 200, 200)) |
                             bgcolor(Color::RGB(128, 0, 0)));
        break;
      case Line::Add:
        left_line_numbers.push_back(text(L"~") | dim);
        right_line_numbers.push_back(text(to_wstring(right_line_number++)));
        lines.push_back(text(line.content) |
                              color(Color::RGB(200, 255, 200)) |
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

class ScrollerBase : public ComponentBase {
 public:
  ScrollerBase(Component child) { Add(child); }

 private:
  Element Render() final {
    auto focused = Focused() ? focus : ftxui::select;
    auto style = Focused() ? inverted : nothing;

    Element background = ComponentBase::Render();
    background->ComputeRequirement();
    size_ = background->requirement().min_y;
    return dbox({
               std::move(background),
               vbox({
                   text(L"") | size(HEIGHT, EQUAL, selected_),
                   text(L"") | style | focused,
               }),
           }) |
           yframe | yflex | reflect(box_);
  }

  bool OnEvent(Event event) final {
    if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
      TakeFocus();

    int selected_old = selected_;
    if (event == Event::ArrowUp || event == Event::Character('k') ||
        (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
      selected_--;
    }
    if ((event == Event::ArrowDown || event == Event::Character('j') ||
         (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
      selected_++;
    }

    selected_ = std::max(0, std::min(size_ - 1, selected_));
    return selected_old != selected_;
  }

  int selected_ = 0;
  int size_ = 0;
  Box box_;
};

Component Scroller(Component child) {
  return Make<ScrollerBase>(std::move(child));
}

int main(int argc, const char** argv) {
  using namespace ftxui;

  std::string args;
  for (int i = 1; i < argc; ++i) {
    args += argv[i];
    args += " ";
  }

  int hunk_size = 3;

  std::vector<File> files;
  std::vector<std::wstring> file_menu_entries;

  auto refresh_data = [&] {
    files.clear();
    file_menu_entries.clear();
    std::string command = "git diff -U" + std::to_string(hunk_size) + " " + args;
    std::string diff = exec(command.c_str());
    files = Parse(diff);
    for (const auto& file : files)
      file_menu_entries.push_back(file.right_file);
  };
  refresh_data();

  if (files.size() == 0) {
    std::cout << "No difference..." << std::endl;
    return EXIT_SUCCESS;
  }

  auto button_increase_hunk = Button("[+1]", [&] {
      hunk_size++;
      refresh_data();
  }, false);
  auto button_decrease_hunk = Button("[-1]", [&] {
      if (hunk_size != 0)
        hunk_size--;
      refresh_data();
  }, false);

  // File menu.
  int file_menu_selected = 0;
  auto file_menu = Menu(&file_menu_entries, &file_menu_selected);

  bool split = true;
  auto split_checkbox = Checkbox("[S]plit", &split);

  auto scroller = Scroller(
      Renderer([&] { return Render(files[file_menu_selected], split); }));

  auto container = Container::Horizontal({
      file_menu,
      scroller,
  });

  container = Renderer(container, [&] {
    const File& file = files[file_menu_selected];
    return hbox({
               window(text(L" Files "), file_menu->Render()) |
                   size(WIDTH, EQUAL, 30) | notflex,
               vbox({
                 window(text(L" Difference "), 
                   vbox({
                       text(file.left_file + L" -> " + file.right_file),
                       separator(),
                       scroller->Render()
                   })
                 ),
                 filler(),
               }) | xflex,
           }) |
           bgcolor(Color::RGB(30, 30, 30)) | yflex;
  });

  auto options = Container::Horizontal({
      split_checkbox,
      button_decrease_hunk,
      button_increase_hunk,
  });

  auto option_renderer = Renderer(options, [&] {
    return hbox({
        text(L"[git diff-tui]"),
        filler(),
        split_checkbox->Render(),
        text(L"   Context:"),
        button_decrease_hunk->Render(),
        text(to_wstring(hunk_size)),
        button_increase_hunk->Render(),
        filler(),
    }) | bgcolor(Color::White) | color(Color::Black);
  });

  container = Container::Vertical({
      option_renderer,
      container,
  });

  file_menu->TakeFocus();

  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(container);

  return EXIT_SUCCESS;
}
