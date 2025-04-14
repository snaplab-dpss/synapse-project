#ifndef _HEADERS_
#define _HEADERS_

header ethernet_h {
	mac_addr_t		dst_addr;
	mac_addr_t		src_addr;
	ether_type_t	ether_type;
}

header ipv4_h {
	bit<4>		version;
	bit<4>		ihl;
	bit<8>		diffserv;
	bit<16>		total_len;
	bit<16>		identification;
	bit<3>		flags;
	bit<13>		frag_offset;
	bit<8>		ttl;
	ip_proto_t	protocol;
	bit<16>		hdr_checksum;
	ipv4_addr_t src_addr;
	ipv4_addr_t dst_addr;
}

header tcp_h {
	bit<16> src_port;
	bit<16> dst_port;
	bit<32> seq_no;
	bit<32> ack_no;
	bit<4>	data_offset;
	bit<4>	res;
	bit<8>	flags;
	bit<16> window;
	bit<16> checksum;
	bit<16> urgent_ptr;
}

header udp_h {
	bit<16> src_port;
	bit<16> dst_port;
	bit<16> len;
	bit<16> checksum;
}

header kv_h {
	bit<8>	op;
	key_t	key;
	val_t	val;
	bit<8>	status;
	bit<16>	port;
}

header cuckoo_h {
	bit<8>	op;
	bit<8>	recirc_cntr;
}

header swap_entry_h {
	bit<8>	op;
	bit<8>	kv_op;
	key_t	key;
	val_t	val;
	bit<8>	status;
	bit<16>	port;
	bit<32> ts;
	bit<16> ts_2;
	// bit<32> ip_src_addr;
	// bit<32> ip_dst_addr;
	// bit<32> ports;
	// bit<16> entry_value;
	// bit<32> ts;
	// bit<16> ts_2;
	// bit<8>	has_swap;
}

struct ingress_metadata_t {
	l4_lookup_t				l4_lookup;
	bit<1>					has_next_swap;
	bit<CUCKOO_IDX_WIDTH>	hash_table_1;
	bit<CUCKOO_IDX_WIDTH>	hash_table_2;
	bit<CUCKOO_IDX_WIDTH>	hash_table_2_r;
	key_t					cur_key;
	val_t					cur_val;
	key_t					table_1_key;
	key_t					table_2_key;
	val_t					table_1_val;
	val_t					table_2_val;
	bit<32>					entry_ts;
	bit<16>					entry_ts_2;
	key_t					swapped_key;
	bit<8>					is_server_reply;
	bit<8>					cur_recirc_port_cntr;
}

struct egress_metadata_t {}

struct header_t {
	ethernet_h		ethernet;
	kv_h			kv;
	cuckoo_h		cuckoo;
	swap_entry_h	cur_swap;
	swap_entry_h	next_swap;
	ipv4_h			ipv4;
	tcp_h			tcp;
	udp_h			udp;
}

#endif
