#include "takina.h"

#include <assert.h>
#include <stddef.h> // size_t
#include <stdio.h>  // snprintf()
#include <stdlib.h> // exit()
#include <string.h> // strlen()
#include <unordered_map>
#include <utility> // move()
#include <vector>

namespace takina {
enum OptType : uint8_t {
  OT_STR = 0,
  OT_FSTR, // fixed string
  OT_MSTR, // Multiple string
  OT_INT,
  OT_FINT,
  OT_MINT,
  OT_DOUBLE,
  OT_FDOUBLE,
  OT_MDOUBLE,
  OT_VOID, // No argument, use a boolean variable to indicates the option is set
  OT_USR,  // user-defined
};

struct OptionParameter {
  /* Because options will be parsed only once,
   * I don't use union to compress the space */
  OptType type;          // interpret the param field
  unsigned int size = 0; // Used for fixed or user-defined option
  void *param = nullptr; // pointer to the user-defined varaible
  OptionFunction opt_fn{};
};

// Help message
static std::string help;
static bool has_help = false;
static bool enable_independent_non_opt_arg = false;

void EnableIndependentNonOptionArgument(bool opt) noexcept
{
  enable_independent_non_opt_arg = opt;
}

/*
 * To implement the AddUsage() and AddDescription() to
 * position-of-call-independent, split them from help e.g.
 * ```cpp
 *  takina::AddDescription()
 *  takina::AddUsage()
 * ```
 * This is also OK
 */

// Usage message
static std::string usage;

// Description of process
static std::string description;

/*
 * The long_param_map store the parameter object,
 * and the short_param_map just store the pointer to it.
 * Instread approach:
 * ```cpp
 * std::vector<OptionParameter> params;
 * std::unordered_map<std::string, OptionParameter*> long_param_map;
 * std::unordered_map<std::string, OptionParameter*> short_param_map;
 * ```
 *
 * This make the process of long_param_map and short_param_map to same.
 * But the params is not necessary in fact.
 * Therefore, I don't define params.
 */

// long option -> OptionParameter
static std::unordered_map<std::string, OptionParameter> long_param_map;

// short option -> OptionParameter
static std::unordered_map<std::string, OptionParameter *> short_param_map;

// When there are some sections at least 1
// section -> { short, long, desc }[]
static std::unordered_map<std::string, std::vector<OptionDescption>>
    section_opt_map;

// There is no section
// { short, long, desc }[]
static std::vector<OptionDescption> options;

// To make the section output in the FIFO order
// since the section_opt_map is unordered(hash table)
static std::vector<std::string const *> sections;

/* Store the non-options arguments */
static std::vector<char const *> non_opt_args;

/* Utility function */
static void AddOption_impl(OptDesc &&desc, OptionParameter opt_param);
static void GenOptions(std::vector<OptionDescption> const &opts,
                       int long_opt_param_align_len, int short_opt_align_len);
static void GenHelp();
static bool StrInt(int *param, char const *arg, std::string const &cur_option,
                   std::string *errmsg);
static bool StrDouble(double *param, char const *arg,
                      std::string const &cur_option, std::string *errmsg);
static bool SetParameter(OptionParameter *param, char const *arg,
                         unsigned int cur_arg_num,
                         std::string const &cur_option, std::string *errmsg);
static bool CheckArgumentIsLess(OptionParameter *cur_param,
                                std::string const &cur_option,
                                unsigned int cur_arg_num, std::string *errmsg);
static bool CheckArgumentIsGreater(OptionParameter *cur_param,
                                   std::string const &cur_option,
                                   unsigned int cur_arg_num,
                                   std::string *errmsg);

void AddUsage(std::string const &desc)
{
  usage.reserve(7 + desc.size() + 1);
  usage = "Usage: ";
  usage += desc;
  usage += "\n\n";
}

void AddDescription(std::string const &desc)
{
  description = desc;
  description += "\n\n";
}

void AddSection(std::string &&section)
{
  auto res = section_opt_map.insert({std::move(section), {}});

  if (!res.second) {
    ::fprintf(stderr, "The section %s does exists!\n", section.c_str());
    return;
  }
  // FIXME
  sections.emplace_back(&res.first->first);
}

void AddOption(OptDesc &&desc, bool *param)
{
  *param = false;
  AddOption_impl(std::move(desc), {.type = OT_VOID, .param = param});
}

#define DEFINE_ADD_OPTION(_ptype, _type)                                       \
  void AddOption(OptDesc &&desc, _ptype *param)                                \
  {                                                                            \
    AddOption_impl(std::move(desc), {.type = _type, .param = param});          \
  }

DEFINE_ADD_OPTION(std::string, OT_STR)
DEFINE_ADD_OPTION(int, OT_INT)
DEFINE_ADD_OPTION(double, OT_DOUBLE)
DEFINE_ADD_OPTION(std::vector<std::string>, OT_MSTR)
DEFINE_ADD_OPTION(std::vector<int>, OT_MINT)
DEFINE_ADD_OPTION(std::vector<double>, OT_MDOUBLE)

#define DEFINE_ADD_OPTION_FIXED(_ptype, _type)                                 \
  void AddOption(OptDesc &&desc, _ptype *param, unsigned int n)                \
  {                                                                            \
    AddOption_impl(std::move(desc), {_type, n, param});                        \
  }

DEFINE_ADD_OPTION_FIXED(std::string, OT_FSTR)
DEFINE_ADD_OPTION_FIXED(int, OT_FINT)
DEFINE_ADD_OPTION_FIXED(double, OT_FDOUBLE)

void AddOption(OptDesc &&desc, OptionFunction fn, unsigned int n)
{
  AddOption_impl(std::move(desc),
                 {.type = OT_USR, .size = n, .opt_fn = std::move(fn)});
}

#define CHECK_OPTION_EXISTS(iter, _map)                                        \
  auto iter = _map.find(cur_option);                                           \
  if (iter == _map.end()) {                                                    \
    *errmsg = "Option: ";                                                      \
    *errmsg += cur_option;                                                     \
    *errmsg += " is not an valid option. Please type --help to check all "     \
               "allowed options";                                              \
    return false;                                                              \
  }

bool Parse(char **argv_begin, char **argv_end, std::string *errmsg)
{
  OptionParameter *cur_param = nullptr;
  std::string cur_option;
  unsigned int cur_arg_num = 0;
  errmsg->clear();
  GenHelp();

  for (; argv_begin != argv_end; ++argv_begin) {
    char const *arg = *argv_begin;
    const size_t len = ::strlen(arg);
    assert(len != 0);

    bool is_long_opt = (arg[0] == '-' && arg[1] == '-') && len > 2;
    bool is_short_opt = (arg[0] == '-') && len > 1;

    // Check if is a option
    // short option or long option
    // PS: ---+ shouldn't think as a option.
    if (is_long_opt || is_short_opt) {
      if (CheckArgumentIsLess(cur_param, cur_option, cur_arg_num, errmsg))
        return false;

      cur_arg_num = 0;

      // long option
      if (arg[1] == '-') {
        cur_option = std::string(&arg[2], len - 2);
        CHECK_OPTION_EXISTS(iter, long_param_map)
        cur_param = &iter->second;
      } else {
        // short option
        cur_option = std::string(&arg[1], len - 1);
        CHECK_OPTION_EXISTS(iter, short_param_map)
        cur_param = iter->second;
      }

      if (cur_param->type == OT_VOID) {
        *(bool *)(cur_param->param) = true;
      }
    } else {
      // Non option arguments
      if (!cur_param) {
        if (enable_independent_non_opt_arg) {
          non_opt_args.push_back(arg);
          continue;
        } else {
          *errmsg += "No option, invalid argument";
          return false;
        }
      }
      cur_arg_num++;
      if (CheckArgumentIsGreater(cur_param, cur_option, cur_arg_num, errmsg)) {
        if (enable_independent_non_opt_arg) {
          non_opt_args.push_back(arg);
        } else {
          return false;
        }
      } else {
        if (!SetParameter(cur_param, arg, cur_arg_num, cur_option, errmsg)) {
          return false;
        }
      }
    }

    if (has_help) {
      ::fputs(help.c_str(), stdout);
      ::exit(0);
      return true;
    }
  }

  if (CheckArgumentIsLess(cur_param, cur_option, cur_arg_num, errmsg))
    return false;

  return true;
}

void DebugPrint()
{
#ifdef TAKINA_DEBUG
  printf("======= Debug Print =======\n");
  printf("All long options: \n");
  for (auto const &long_opt : long_param_map) {
    printf("--%s\n", long_opt.first.c_str());
  }

  printf("All short options: \n");
  for (auto const &short_opt : short_param_map) {
    printf("-%s\n", short_opt.first.c_str());
  }
  puts("");
#endif
}

static inline void Teardown(std::string *str)
{
  str->clear();
  str->shrink_to_fit();
}

template <typename T>
static inline void Teardown(std::vector<T> *vec)
{
  vec->clear();
  vec->shrink_to_fit();
}

template <typename T>
static inline void Teardown(T *cont)
{
  cont->clear();
}

#define TAKINA_TEARDOWN(obj) (Teardown(obj))

void Teardown()
{
  TAKINA_TEARDOWN(&help);
  TAKINA_TEARDOWN(&usage);
  TAKINA_TEARDOWN(&description);
  TAKINA_TEARDOWN(&long_param_map);
  TAKINA_TEARDOWN(&short_param_map);
  TAKINA_TEARDOWN(&section_opt_map);
  TAKINA_TEARDOWN(&sections);
  TAKINA_TEARDOWN(&options);
}

static inline void GenHelp()
{
  AddOption({"", "help", "Display the help message"}, &has_help);

  help.reserve(usage.size() + description.size());
  help = usage;
  help += description;

  int short_opt_align_len = 0;
  int long_opt_param_align_len = 0;

  if (!sections.empty()) {
    for (auto const &section_opt : section_opt_map) {
      auto &options = section_opt.second;
      for (auto const &option : options) {
        long_opt_param_align_len =
            TAKINA_MAX(long_opt_param_align_len,
                       (int)(option.param_name.size() + option.lopt.size()));
        short_opt_align_len =
            TAKINA_MAX(short_opt_align_len, (int)option.sopt.size());
      }
    }
  } else {
    for (auto const &option : options) {
      long_opt_param_align_len =
          TAKINA_MAX(long_opt_param_align_len,
                     (int)(option.param_name.size() + option.lopt.size()));
      short_opt_align_len =
          TAKINA_MAX(short_opt_align_len, (int)option.sopt.size());
    }
  }

  help += "Options: \n\n";
  if (!sections.empty()) {
    for (auto const &section : sections) {
      help += *section;
      help += ":\n";

      auto const &opts = section_opt_map[*section];
      GenOptions(opts, long_opt_param_align_len, short_opt_align_len);
      help += "\n";
    }
  } else {
    GenOptions(options, long_opt_param_align_len, short_opt_align_len);
  }
  help.pop_back();
}

void GenOptions(std::vector<OptionDescption> const &opts,
                int long_opt_param_align_len, int short_opt_align_len)
{
  char buf[65535];
  std::string lopt_param;
  lopt_param.reserve(long_opt_param_align_len + 1);
  // Format:
  // -[short-opt],--[long-opt] [param-name]   [description]
  // If there are no short options, put blackspace as placeholders.
  //              --[long-opt] [param-name]   [description]
  // To don't split long-opt and param-name too long,
  // combine them to single unit.
  for (auto const &opt : opts) {
    std::string format;
    if (opt.sopt.empty()) {
      format = " %-*s ";
    } else {
      format = "-%-*s,";
    }
    format += " --%-*s   %s\n";
    lopt_param = opt.lopt;
    lopt_param += " ";
    lopt_param += opt.param_name;
    ::snprintf(buf, sizeof buf, format.c_str(), short_opt_align_len,
               opt.sopt.c_str(), long_opt_param_align_len + 1,
               lopt_param.c_str(), opt.desc.c_str());
    help += buf;
  }
}

inline void AddOption_impl(OptDesc &&desc, OptionParameter opt_param)
{
  if (desc.lopt.empty()) return;

  auto res = long_param_map.insert({(desc.lopt), opt_param});
  if (!res.second) {
    ::fprintf(stderr, "The long option: %s does exists\n", desc.lopt.c_str());
    return;
  }

  if (!desc.sopt.empty()) {
    if (!short_param_map.insert({(desc.sopt), &res.first->second}).second) {
      ::fprintf(stderr, "The short option: %s does exists\n",
                desc.sopt.c_str());
      return;
    }
  }

  if (!sections.empty()) {
    section_opt_map[*sections.back()].push_back(std::move(desc));
  } else {
    options.push_back(std::move(desc));
  }
}

static inline bool StrInt(int *param, char const *arg,
                          std::string const &cur_option, std::string *errmsg)
{
  char *end = nullptr;
  const auto res = ::strtol(arg, &end, 10);
  if (res == 0 && end == arg) {
    *errmsg = "Option: ";
    *errmsg += cur_option;
    *errmsg += '\n';
    *errmsg += "syntax error: this is not a valid integer argument";
    return false;
  }
  *param = res;
  return true;
}

static inline bool StrDouble(double *param, char const *arg,
                             std::string const &cur_option, std::string *errmsg)
{
  char *end = nullptr;
  const auto res = ::strtod(arg, &end);
  if (res == 0 && end == arg) {
    *errmsg = "Option: ";
    *errmsg += cur_option;
    *errmsg += '\n';
    *errmsg +=
        "Syntax error: this is not a valid float-pointing number argument";
    return false;
  }
  *param = res;
  return true;
}

static inline bool SetParameter(OptionParameter *param, char const *arg,
                                unsigned int cur_arg_num,
                                std::string const &cur_option,
                                std::string *errmsg)
{
  switch (param->type) {
    case OT_STR:
      *(std::string *)(param->param) = arg;
      break;
    case OT_INT:
    {
      int res;
      if (!StrInt(&res, arg, cur_option, errmsg)) return false;
      *(int *)(param->param) = res;
    } break;
    case OT_DOUBLE:
    {
      double res;
      if (!StrDouble(&res, arg, cur_option, errmsg)) return false;
      *(double *)(param->param) = res;
    } break;
    case OT_MSTR:
    {
      ((std::vector<std::string> *)(param->param))->emplace_back(arg);
      break;
    } break;
    case OT_MINT:
    {
      int res;
      if (!StrInt(&res, arg, cur_option, errmsg)) return false;
      ((std::vector<int> *)(param->param))->emplace_back(res);
    } break;
    case OT_MDOUBLE:
    {
      double res;
      if (!StrDouble(&res, arg, cur_option, errmsg)) return false;
      ((std::vector<double> *)(param->param))->emplace_back(res);
    } break;

#define FIXED_ARGUMENTS_ERR_ROUTINE                                            \
  if (cur_arg_num > param->size) {                                             \
    *errmsg = "Option: ";                                                      \
    *errmsg += cur_option;                                                     \
    *errmsg += ", The number of arguments more than required";                 \
    return false;                                                              \
  }

    case OT_FSTR:
    {
      FIXED_ARGUMENTS_ERR_ROUTINE
      auto arr = (std::string *)(param->param);
      arr[cur_arg_num - 1] = arg;
    } break;
    case OT_FINT:
    {
      FIXED_ARGUMENTS_ERR_ROUTINE
      auto arr = (int *)(param->param);
      int res;
      if (!StrInt(&res, arg, cur_option, errmsg)) return false;
      arr[cur_arg_num - 1] = res;
    } break;
    case OT_FDOUBLE:
    {
      FIXED_ARGUMENTS_ERR_ROUTINE
      auto arr = (double *)(param->param);
      double res;
      if (!StrDouble(&res, arg, cur_option, errmsg)) return false;
      arr[cur_arg_num - 1] = res;
    } break;
    case OT_USR:
    {
      if (!param->opt_fn(arg)) {
        *errmsg += "Option error: Invalid arguments for ";
        *errmsg += cur_option;
        return false;
      }
    } break;
  }

  return true;
}

static inline bool CheckArgumentIsLess(OptionParameter *cur_param,
                                       std::string const &cur_option,
                                       unsigned int cur_arg_num,
                                       std::string *errmsg)
{
  bool condition = false;
  if (cur_param) {
    switch (cur_param->type) {
      case OT_STR:
      case OT_INT:
      case OT_DOUBLE:
        condition = cur_arg_num == 0;
        break;
      case OT_FSTR:
      case OT_FINT:
      case OT_FDOUBLE:
        condition = cur_arg_num < cur_param->size;
        break;
      case OT_USR:
        condition = (cur_arg_num < cur_param->size);
        break;
    }
  }

  if (condition) {
    *errmsg = "Option: ";
    *errmsg += cur_option;
    *errmsg += ", the number of arguments is less than required";
    return true;
  }

  return false;
}

static inline bool CheckArgumentIsGreater(OptionParameter *cur_param,
                                          std::string const &cur_option,
                                          unsigned int cur_arg_num,
                                          std::string *errmsg)
{
  unsigned int size = 0;
  assert(cur_param);
  switch (cur_param->type) {
    case OT_STR:
    case OT_INT:
    case OT_DOUBLE:
    {
      size = 1;
    } break;

    case OT_FDOUBLE:
    case OT_FINT:
    case OT_FSTR:
    case OT_USR:
    {
      size = cur_param->size;
    } break;
  }

  if (cur_arg_num > size) {
    if (!enable_independent_non_opt_arg) {
      *errmsg = "To unary argument option: ";
      *errmsg += cur_option;
      *errmsg += ", the number of arguments more than ";
      /* Because SSO, no dynamic allocation in most */
      *errmsg += std::to_string(size);
    }
    return true;
  }
  return false;
}

std::vector<char const *> &GetNonOptionArguments() { return non_opt_args; }

} // namespace takina
