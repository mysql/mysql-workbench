#ifndef __WB_UTILS_H__
#define __WB_UTILS_H__

//#define DEBUG 1
#undef DEBUG
#ifdef DEBUG

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

inline void wb_ptrace(void) {
  enum { SIZE = 100 };
  void *buffer[100];

  int nptrs = backtrace(buffer, SIZE);

  char **strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL)
    perror("backtrace_symbols");

  for (int j = 0; j < nptrs; j++)
    printf("%s\n", strings[j]);

  free(strings);
}
#endif

#ifdef DEBUG
#define ptrace() wb_ptrace()
#define dprint(...) fprintf(stderr, __VA_ARGS__)
#else
#define ptrace()
#define dprint(...)
#endif

#endif
