
#include "string_utils.h"

void sanitizePath(char* s) {
  for (; *s; s++) {
    if (*s == ' ') *s = '_';
  }
}