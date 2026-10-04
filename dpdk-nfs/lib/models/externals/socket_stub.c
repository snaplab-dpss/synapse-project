// NOTE: The name of this file is because if it's just called "socket.c",
//       it doesn't get included somehow... maybe a conflict with KLEE-uclibc?

#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>

int stub_socket(int family, int type, int protocol) {
  // "On success, a file descriptor for the new socket is returned.  On error,
  // -1 is returned, and errno is set appropriately."
  // -- http://man7.org/linux/man-pages/man2/socket.2.html
  errno = EAFNOSUPPORT; // "The implementation does not support the specified
                        // address family."
  return -1;
}

// Under KLEE the uclibc socket is left out of the link (see the Makefile), so this
// definition is the one the program calls.
int socket(int family, int type, int protocol) { return stub_socket(family, type, protocol); }
