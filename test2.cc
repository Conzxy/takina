#include "takina.h"

int main(int argc, char *argv[])
{
  int test1;
  int test2;
  std::string errmsg;
  takina::AddOption({"t1", "test1", "test1"}, &test1);
  takina::AddSection("test1");
  takina::AddOption({"t2", "test2", "test2"}, &test2);
  if (!takina::Parse(argc, argv, &errmsg)) {
    fprintf(stderr, "%s\n", errmsg.c_str());
    return 0;
  }
}
