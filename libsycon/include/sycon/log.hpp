#pragma once

#include <assert.h>
#include <stdio.h>

extern "C" {
#include <bf_switchd/bf_switchd.h>
}

#include "util.hpp"

#ifdef NDEBUG
#define DEBUG(...)

#define ASSERT_BF_STATUS(status)                                                                                       \
  {                                                                                                                    \
    if (unlikely((status) != BF_SUCCESS)) {                                                                            \
      ERROR("BF function failed (%s)", bf_err_str((status)));                                                          \
    }                                                                                                                  \
  }

#else
#define DEBUG(fmt, ...)                                                                                                \
  {                                                                                                                    \
    fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__);                                                                \
    fflush(stderr);                                                                                                    \
  }

#define ASSERT_BF_STATUS(status)                                                                                       \
  {                                                                                                                    \
    if (unlikely((status) != BF_SUCCESS)) {                                                                            \
      DEBUG("BF function failed (%s)", bf_err_str((status)));                                                          \
    }                                                                                                                  \
    assert((status) == BF_SUCCESS);                                                                                    \
  }
#endif

#define LOG(fmt, ...)                                                                                                  \
  {                                                                                                                    \
    printf(fmt "\n", ##__VA_ARGS__);                                                                                   \
    fflush(stdout);                                                                                                    \
  }

#define ERROR(fmt, ...)                                                                                                \
  {                                                                                                                    \
    fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__);                                                                \
    fflush(stderr);                                                                                                    \
    exit(1);                                                                                                           \
  }

#define WARNING(fmt, ...)                                                                                              \
  {                                                                                                                    \
    printf("WARNING: " fmt "\n", ##__VA_ARGS__);                                                                       \
    fflush(stdout);                                                                                                    \
  }
