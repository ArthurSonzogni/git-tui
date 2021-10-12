#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifndef WIN32
inline FILE* _popen(const char* command, const char* type) {
  return popen(command, type);
}
inline void _pclose(FILE* file) {
  pclose(file);
}
#endif

std::string ExecuteProcess(std::string cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"),
                                                 _pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), (int)buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}
