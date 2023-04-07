#ifndef _TAKINA_TAKINA_H_
#define _TAKINA_TAKINA_H_

#include <string>
#include <vector>
#include <functional> // function

// I don't want to introduce std::max()
#define TAKINA_MAX(x, y) (((x) < (y)) ? (y) : (x))

namespace takina {

typedef std::function<bool(char const *arg)> OptionFunction;

// Use aggregation initialization to create a OptionDescription object
struct OptionDescption {
  // FIXME use char const *
  std::string sopt;         // short option
  std::string lopt;         // long option
  std::string desc;         // description of option
  std::string param_name{}; // name of parameters
};

using OptDesc = OptionDescption;

/** Add usage of process */
void AddUsage(std::string const &desc);

/** Add description of process */
void AddDescription(std::string const &desc);

/** Add section of options(or options group) */
void AddSection(std::string &&section);

/** Add Options */
void AddOption(OptDesc &&desc, bool *param);
void AddOption(OptDesc &&desc, std::string *param);
void AddOption(OptDesc &&desc, int *param);
void AddOption(OptDesc &&desc, double *param);
void AddOption(OptDesc &&desc, std::vector<std::string> *param);
void AddOption(OptDesc &&desc, std::vector<int> *param);
void AddOption(OptDesc &&desc, std::vector<double> *param);
void AddOption(OptDesc &&desc, std::string *param, unsigned int n);
void AddOption(OptDesc &&desc, int *param, unsigned int n);
void AddOption(OptDesc &&desc, double *param, unsigned int n);
void AddOption(OptDesc &&desc, OptionFunction fn, unsigned int n = 1);
#define MAX_OPTION_ARGS_NUM ((unsigned int)-1)

/** parse the command line arguments */
bool Parse(char **argv_begin, char **argv_end, std::string *errmsg);

inline bool Parse(int argc, char **argv, std::string *errmsg)
{
  return Parse(argv + 1, argv + argc, errmsg);
}

/** Free the resources used for parsing options */
void Teardown();

void DebugPrint();

void EnableIndependentNonOptionArgument(bool opt) noexcept;

std::vector<char const *> &GetNonOptionArguments();

} // namespace takina

#endif // _TAKINA_TAKINA_H_
