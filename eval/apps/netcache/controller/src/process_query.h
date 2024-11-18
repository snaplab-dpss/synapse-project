#pragma once

#include <memory>
#include <string>

#include "query.h"
#include "server_reply.h"

#define BUFFER_SIZE 65536

typedef unsigned char byte_t;

namespace netcache {

struct ProcessQuery {
	int sockfd;
	struct sockaddr_in servaddr;

	ProcessQuery();

	void process_hot_read_query(const query_t& query);
	void process_write_query(const query_t& query);
	void process_del_query(const query_t& query);
};

}  // netcache
