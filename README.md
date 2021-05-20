<p align="center">
  <br><img src="logo.png" alt="git diff-tui" /><br>
</p>

<h1 align="center">git diff-tui</h1>

<h3 align="center">A human friendly git diff, running in the terminal.</h3>



<p align="center">
  <a href="https://github.com/ArthurSonzogni/git-diff-tui/actions?workflow=linux-clang.yaml">
    <img src="https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/linux-clang.yaml/badge.svg?branch=master" alt="GitHub action linux clang">
  </a>
  <a href="https://github.com/ArthurSonzogni/git-diff-tui/actions?workflow=linux-gcc.yaml">
    <img src="https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/linux-gcc.yaml/badge.svg?branch=master" alt="GitHub action linux gcc">
  </a>
  <a href="https://github.com/ArthurSonzogni/git-diff-tui/actions?workflow=mac-clang.yaml">
    <img src="https://github.com/ArthurSonzogni/git-diff-tui/actions/workflows/mac-clang.yaml/badge.svg?branch=master" alt="GitHub action mac clang">
  </a>
  <a href="https://github.com/ArthurSonzogni/git-diff-tui/blob/master/LICENSE.md">
    <img src="https://img.shields.io/github/license/ArthurSonzogni/git-diff-tui" alt="LICENSE">
  </a>
</p>

<p align="center">
  <a href="#demo"><b>Demo</b></a>&nbsp;&bull;
  <a href="#install"><b>Installation</b></a>&nbsp;&bull;
  <a href="#usage"><b>Usage</b></a>&nbsp;&bull;
  <a href="#contribute"><b>Contribute</b></a>
</p>

# Demo
![Demo](./demo.gif)

# Install

### Snap:
[![git-diff-tui](https://snapcraft.io/git-diff-tui/badge.svg)](https://snapcraft.io/git-diff-tui)
```bash
sudo snap install git-diff-tui
```
### From binaries:

10 different packages (deb, rpm, sh, zip, tar.gz, etc...): [Latest](https://github.com/ArthurSonzogni/git-diff-tui/releases/latest).

### From source:
```bash
git clone https://github.com/ArthurSonzogni/git-diff-tui;
cd git-diff-tui;
mkdir build;
cd build;
cmake ..;
sudo make install -j;
```
# Usage

The commands `git diff` and `git diff-tui` are interchangeable. They both accept the same options.

# Contribute

If you like `git diff-tui` and you find it useful, there are ways for you to contribute. You can suggest improvement or propose your own pull requests.
