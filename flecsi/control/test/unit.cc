#include <flecsi/control/ftest.h>

#include <iostream>

int unit_initialization(int argc, char ** argv) {
  std::cout << "Executing initialization action" << std::endl;
  return 0;
}

int unit_driver(int argc, char ** argv) {
  std::cout << "Executing driver action" << std::endl;
  return 0;
}
