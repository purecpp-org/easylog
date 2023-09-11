#include "easylog.hpp"
#include <iostream>

int main() {
  std::string filename = "easylog.txt";
  std::filesystem::remove(filename);
  easylog::init_log(easylog::Severity::DEBUG, filename, true, 5000, 1, true);

  ELOG_INFO << 42;
  ELOG_INFO << 42 << " " << 4.5 << 'a' << easylog::Severity::DEBUG;

  ELOGV(INFO, "test");
  ELOGV(INFO, "it is a long string test %d %s", 2, "ok");
}