git diff-tui
------------

A human friendly version of "git diff". Running with a terminal UI.

![Demo](./demo.gif)

# Install

## Snap:
[![git-diff-tui](https://snapcraft.io/git-diff-tui/badge.svg)](https://snapcraft.io/git-diff-tui)
```bash
sudo snap install git-diff-tui
```
## From binaries:

10 different packages (deb, rpm, sh, zip, tar.gz, etc...): [Latest](https://github.com/ArthurSonzogni/git-diff-tui/releases/latest).

## From source:
```bash
git clone https://github.com/ArthurSonzogni/git-diff-tui;
cd git-diff-tui;
mkdir build;
cd build;
cmake ..;
sudo make install -j;
```

## Operating systems
- [![linux-gcc][badge.linux-gcc]][link.linux-gcc]
  [![linux-clang][badge.linux-clang]][link.linux-clang]
- [![mac-clang][badge.mac-clang]][link.mac-clang]

[badge.linux-gcc]: https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/linux-gcc.yaml/badge.svg?branch=master
[badge.linux-clang]: https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/linux-clang.yaml/badge.svg?branch=master
[badge.mac-clang]: https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/mac-clang.yaml/badge.svg?branch=master

[link.linux-gcc]: https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/linux-gcc.yaml
[link.linux-clang]: https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/linux-clang.yaml
[link.mac-clang]: https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/mac-clang.yaml
