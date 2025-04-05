#include <sycon/sycon.h>

using namespace sycon;

constexpr const u32 HASH_SALT_0{0xfbc31fc7};
constexpr const u32 HASH_SALT_1{0x2681580b};
constexpr const u32 HASH_SALT_2{0x486d7e2f};

struct state_t : public nf_state_t {
  CRC32 crc32;

  state_t() {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
}

void sycon::nf_exit() {}

void sycon::nf_user_signal_handler() {}

void sycon::nf_args(CLI::App &app) {}

struct app_t {
  u32 hash;
  u32 hash0;
  u32 hash1;
  u32 hash2;
} __attribute__((packed));

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  eth_hdr_t *eth = packet_consume<eth_hdr_t>(pkt);
  ipv4_hdr_t *ip = packet_consume<ipv4_hdr_t>(pkt);
  udp_hdr_t *udp = packet_consume<udp_hdr_t>(pkt);
  app_t *app     = packet_consume<app_t>(pkt);

  buffer_t input_hash(12);
  buffer_t input_hash0(16);
  buffer_t input_hash1(16);
  buffer_t input_hash2(16);

  input_hash.set_big_endian(0, 4, ip->src_ip);
  input_hash.set_big_endian(4, 4, ip->dst_ip);
  input_hash.set_big_endian(8, 2, udp->src_port);
  input_hash.set_big_endian(10, 2, udp->dst_port);

  input_hash0.set_big_endian(0, 4, ip->src_ip);
  input_hash0.set_big_endian(4, 4, ip->dst_ip);
  input_hash0.set_big_endian(8, 2, udp->src_port);
  input_hash0.set_big_endian(10, 2, udp->dst_port);
  input_hash0.set(12, 4, HASH_SALT_0);

  input_hash1.set_big_endian(0, 4, ip->src_ip);
  input_hash1.set_big_endian(4, 4, ip->dst_ip);
  input_hash1.set_big_endian(8, 2, udp->src_port);
  input_hash1.set_big_endian(10, 2, udp->dst_port);
  input_hash1.set(12, 4, HASH_SALT_1);

  input_hash2.set_big_endian(0, 4, ip->src_ip);
  input_hash2.set_big_endian(4, 4, ip->dst_ip);
  input_hash2.set_big_endian(8, 2, udp->src_port);
  input_hash2.set_big_endian(10, 2, udp->dst_port);
  input_hash2.set(12, 4, HASH_SALT_2);

  printf("\n");
  printf("========================================\n");
  printf("input hash:     %s\n", input_hash.to_string(true).c_str());
  printf("hdr hash:       %08x\n", bswap32(app->hash));
  printf("local hash:     %08x\n", state->crc32.hash(input_hash));

  printf("\n");
  printf("input hash0:    %s\n", input_hash0.to_string(true).c_str());
  printf("hdr hash0:      %08x\n", bswap32(app->hash0));
  printf("local hash:     %08x\n", state->crc32.hash(input_hash0));

  printf("\n");
  printf("input hash1:    %s\n", input_hash1.to_string(true).c_str());
  printf("hdr hash1:      %08x\n", bswap32(app->hash1));
  printf("local hash:     %08x\n", state->crc32.hash(input_hash1));

  printf("\n");
  printf("input hash2:    %s\n", input_hash2.to_string(true).c_str());
  printf("hdr hash2:      %08x\n", bswap32(app->hash2));
  printf("local hash:     %08x\n", state->crc32.hash(input_hash2));
  printf("========================================\n");

  result.forward = false;
  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
