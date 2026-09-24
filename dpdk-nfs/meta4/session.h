#ifndef _SESSION_H_INCLUDED_
#define _SESSION_H_INCLUDED_

#include <stdint.h>

#include "lib/util/boilerplate.h"

// A client-server pair, the key of the DNS Response Table. It is learnt from a DNS response
// (client = the response's destination, server = the A record's address) and looked up again on
// the data packets of the session, which travel server -> client.
struct session {
  uint32_t client_ip;
  uint32_t server_ip;
} PACKED_FOR_KLEE_VERIFICATION;

#define LOG_SESSION(obj, p)                                                                                                                          \
  ;                                                                                                                                                  \
  p("{");                                                                                                                                            \
  p("client_ip: %u", obj->client_ip);                                                                                                                \
  p("server_ip: %u", obj->server_ip);                                                                                                                \
  p("}");

#ifdef KLEE_VERIFICATION
#include "lib/models/str-descr.h"

extern struct str_field_descr session_descrs[2];
extern struct nested_field_descr session_nests[0];
#endif // KLEE_VERIFICATION

#endif //_SESSION_H_INCLUDED_
