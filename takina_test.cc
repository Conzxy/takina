#define _TAKINA_DEBUG_
#include "takina.h"
#include <iostream>
#include <string.h>

enum TestEnum {
  TE_A = 10,
  TE_B,
};

struct TakinaOption {
  std::string stropt;
  std::vector<std::string> stropts;
  std::vector<int> iopts;
  std::vector<double> dopts;
  std::string strfopts[2];
  int ifopts[2];
  double ffopts[2];

  bool version;
  int iopt;
  double fopt;
  TestEnum e;
};

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> const& params) {
  for (auto const& param : params) {
    os << param << " ";
  }
  return os;
}

// template<typename T, size_t N>
// std::ostream& operator<<(std::ostream& os, T const(&params)[N]) {
//   for (auto const& param : params) {
//     os << param << " ";
//   }
//   return os;
// }

template<typename T, size_t N>
void PrintFixedParams(std::ostream& os, T const(&params)[N]) {
  for (auto const& param : params) {
    os << param << " ";
  }
}
static inline void PrintOption(TakinaOption const& option) {
  printf("stropt: %s\n", option.stropt.c_str());
  printf("version: %d\n", (int)option.version);
  printf("iopt: %d\n", option.iopt);
  printf("dopt: %lf\n", option.fopt);
  std::cout << "stropts: " << option.stropts << "\n"
    << "iopts: " << option.iopts << "\n"
    << "dopts: " << option.dopts << "\n";

  std::cout << "strfopts: ";
  PrintFixedParams(std::cout, option.strfopts);
  std::cout << "\nifopts: ";
  PrintFixedParams(std::cout, option.ifopts);
  std::cout << "\nffopts: ";
  PrintFixedParams(std::cout, option.ffopts);
  std::cout << "\nenum: " << option.e;
  std::cout << std::endl;
}

int main(int argc, char** argv) {
  std::string message;
  TakinaOption opt;
  takina::AddUsage("./takina_test [options]");
  takina::AddDescription("The program is just a test");
  takina::AddSection("Information");
  takina::AddOption({"", "version", "Display the version information"}, &opt.version);
  takina::AddSection("Parameter set test");
  takina::AddOption({"s", "string", "Set string argument", "STR"}, &opt.stropt);
  takina::AddOption({"i", "int", "Set int argument", "INT"}, &opt.iopt);
  takina::AddOption({"f", "float", "Set floating-point number", "FLOAT_NUMBER"}, &opt.fopt);
  takina::AddSection("Multiple Parameters set test");
  takina::AddOption({"ms", "multi_string", "Set multiple-string arguments", "STRINGS"}, &opt.stropts);
  takina::AddOption({"mi", "multi_int", "Set multiple-int arguments", "INTS"}, &opt.iopts);
  takina::AddOption({"mf", "multi_float", "Set multiple-floating-point-number arguments", "FLOAT_NUMBERS"}, &opt.dopts);
  // Duplicate option will output error message to stderr
  takina::AddOption({"mf", "multi_float", "Set multiple-floating-point-number arguments"}, &opt.dopts);
  takina::AddOption({"mi", "multi_int", "Set multiple-int arguments"}, &opt.iopts);
  takina::AddOption({"ms", "multi_string", "Set Multiple-string arguments"}, &opt.stropts);

  takina::AddSection("Fixed parameters set test");
  takina::AddOption({"fs", "fixed_string", "Set Fixed-string arguments"}, opt.strfopts, 2);
  takina::AddOption({"fi", "fixed_int", "Set Fixed-int arguments"}, opt.ifopts, 2);
  takina::AddOption({"ff", "fixed_float", "Set Fixed-floating-point-number arguments"}, opt.ffopts, 2);
  takina::AddOption({"e", "enum", "Set enum argument", "ENUM"}, [&opt](char const *arg) {
    if (!strcmp(arg, "A")) {
      opt.e = TE_A;
    } else if (!strcmp(arg, "B")) {
      opt.e = TE_B;
    } else {
      return false;
    }
    return true;
  });

  // takina::DebugPrint();
  auto success = takina::Parse(argc, argv, &message);

  if (success) {
    takina::Teardown();
    PrintOption(opt);
  } else {
    ::puts(message.c_str());
  }
}
