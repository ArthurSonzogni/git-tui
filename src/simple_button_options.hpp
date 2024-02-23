#ifndef GIT_TUI_SIMPLE_BUTTOn_OPTION
#define GIT_TUI_SIMPLE_BUTTOn_OPTION

#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

namespace gittui {
using namespace ftxui;
/// when focused. This is the current default.
static ButtonOption SimpleButtonOption() {
  ButtonOption option;
  option.transform = [](const EntryState& s) {
    auto element = text(s.label);
    if (s.active) {
      element |= bold;
    }
    if (s.focused) {
      element |= inverted;
    }
    return element;
  };
  return option;
}

}  // namespace gittui

#endif  // GIT_TUI_SIMPLE_BUTTOn_OPTION

// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
