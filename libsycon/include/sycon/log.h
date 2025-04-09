#pragma once

#include <assert.h>
#include <stdio.h>

extern "C" {
#include <bf_switchd/bf_switchd.h>
}

#ifdef NDEBUG
#define LOG_DEBUG(...)

#define ASSERT_BF_STATUS(status)                                                                                                                     \
  ({                                                                                                                                                 \
    if (unlikely((status) != BF_SUCCESS)) {                                                                                                          \
      ERROR("%s@%s:%d BF function failed (%s)", __func__, __FILE__, __LINE__, bf_err_str((status)));                                                 \
    }                                                                                                                                                \
  })

#else
#define LOG_DEBUG(fmt, ...)                                                                                                                          \
  ({                                                                                                                                                 \
    fprintf(stderr, "LOG_DEBUG: " fmt "\n", ##__VA_ARGS__);                                                                                          \
    fflush(stderr);                                                                                                                                  \
  })

#define ASSERT_BF_STATUS(status)                                                                                                                     \
  ({                                                                                                                                                 \
    if (unlikely((status) != BF_SUCCESS)) {                                                                                                          \
      LOG_DEBUG("%s@%s:%d BF function failed (%s)", __func__, __FILE__, __LINE__, bf_err_str((status)));                                             \
    }                                                                                                                                                \
    assert((status) == BF_SUCCESS);                                                                                                                  \
  })
#endif

#define SHOW_PROMPT() (printf("Sycon> "), fflush(stdout))

#define LOG(fmt, ...)                                                                                                                                \
  ({                                                                                                                                                 \
    printf(fmt "\n", ##__VA_ARGS__);                                                                                                                 \
    fflush(stdout);                                                                                                                                  \
  })

#define ERROR(fmt, ...)                                                                                                                              \
  ({                                                                                                                                                 \
    fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__);                                                                                              \
    fflush(stderr);                                                                                                                                  \
    exit(1);                                                                                                                                         \
  })

#define WARNING(fmt, ...)                                                                                                                            \
  ({                                                                                                                                                 \
    printf("WARNING: " fmt "\n", ##__VA_ARGS__);                                                                                                     \
    fflush(stdout);                                                                                                                                  \
  })
