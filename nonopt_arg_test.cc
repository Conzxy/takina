#include "takina.h"

#include <stdio.h>

int main(int argc, char **argv)
{
  int port;
  takina::AddOption({"p", "port", "Port number", "PORT"}, &port);
  takina::EnableIndependentNonOptionArgument(true);
  
  std::string errmsg;
  if (!takina::Parse(argc, argv, &errmsg)) {
    fprintf(stderr, "%s\n", errmsg.c_str());
    return 0;
  }
  
  printf("port = %d\n", port);
  auto &non_opt_args = takina::GetNonOptionArguments();
  for (auto &non_opt_arg : non_opt_args) {
    printf("non-opt-arg: %s", non_opt_arg);
  }
}
