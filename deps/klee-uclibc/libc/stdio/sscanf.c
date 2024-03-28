/* Copyright (C) 2004      Manuel Novoa III <mjn3@uclibc.org>
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 *
 * Dedicated to Toni.  See uClibc/DEDICATION.mjn3 for details.
 */
#define L_sscanf
#include "_scanf.c"
int __isoc99_sscanf(const char *str, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int ret = vsscanf(str, format, args);
  va_end(args);
  return ret;
}
