#pragma once

#ifdef NDEBUG
#define LOG_DEBUG(...)
#else
#define LOG_DEBUG(fmt, ...)                                                                                                                \
  ({                                                                                                                                       \
    fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__);                                                                                    \
    fflush(stderr);                                                                                                                        \
  })
#endif