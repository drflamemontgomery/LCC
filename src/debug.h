/*
    Text-based debugging helper functions
    Copyright (C) 2025  Ashton Warner

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef DEBUG_H
#define DEBUG_H

#include "term.h"
#include <stdio.h>
#include <stdlib.h>

#define log(msg, ...) fprintf(stderr, msg "\n" __VA_OPT__(, ) __VA_ARGS__)
#define info(msg, ...)                                                         \
  log(TERM_BOLD TERM_BLUE "info: " TERM_RESET msg __VA_OPT__(, ) __VA_ARGS__)
#define warning(msg, ...)                                                      \
  log(TERM_BOLD TERM_YELLOW "warning: " TERM_RESET msg __VA_OPT__(, )          \
          __VA_ARGS__)
#define success(msg, ...)                                                      \
  log(TERM_BOLD TERM_GREEN "success: " TERM_RESET msg __VA_OPT__(, )           \
          __VA_ARGS__)
#define error(msg, ...)                                                        \
  log(TERM_BOLD TERM_RED "error: " TERM_RESET msg __VA_OPT__(, ) __VA_ARGS__)
#define errorat(msg, ...)                                                      \
  log(TERM_BOLD TERM_RED "error: " TERM_BOLD                                   \
                         "%s:%d:%d: " TERM_RESET msg __VA_OPT__(, )            \
                             __VA_ARGS__)
#define logat(msg, ...)                                                        \
  log(TERM_BOLD "%s:%d:%d: " TERM_RESET msg __VA_OPT__(, ) __VA_ARGS__)

#ifdef DEBUG

#define trace(msg, ...)                                                        \
  fprintf(stderr, TERM_BOLD "%s:%d: " TERM_RESET msg "\n", __FILE__,           \
          __LINE__ __VA_OPT__(, ) __VA_ARGS__)

#define errtrace(msg, ...)                                                     \
  trace(TERM_BOLD TERM_RED "error: " TERM_RESET msg __VA_OPT__(, ) __VA_ARGS__)

#define debugAssert(cond, msg, ...)                                            \
  if (!(cond)) {                                                               \
    errtrace("%s() " msg, __func__ __VA_OPT__(, ) __VA_ARGS__);                \
    abort();                                                                   \
  }

#define infotrace(msg, ...)                                                    \
  trace(TERM_BOLD TERM_BLUE "info: " TERM_RESET msg __VA_OPT__(, ) __VA_ARGS__)

#define successtrace(msg, ...)                                                 \
  trace(TERM_BOLD TERM_GREEN "success: " TERM_RESET msg __VA_OPT__(, )         \
            __VA_ARGS__)
#else
#define trace(msg, ...)
#define errtrace(msg, ...)
#define debugAssert(cond, msg, ...)
#endif

#endif // DEBUG_H
