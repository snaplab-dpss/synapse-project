// vim: syntax=P4
/*
    HyperLogLog approximated distinct counting algorithm for Tofino programmable switch
    
    Copyright (C) 2020 Xiaoqi Chen, Princeton University
    xiaoqic [at] cs.princeton.edu / https://doi.org/10.1145/3387514.3405865
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.
    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


//== Parameters for HyperLogLog estimators
// number of estimator
// M = 64
// LOG_M = 6

// scaling factor for inversed probabilities
// SCALING = 20

// estimation correction factor, calculated from M
// alpha = 0.7093409548395029

// sanity check for parameters
#if (64 != (1<<6))
  #error M LOG_M mismatch
#endif


#include <core.p4>

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;
const ether_type_t ETHERTYPE_VLAN = 16w0x0810;

typedef bit<8> ip_protocol_t;
const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;


header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16> ether_type;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3> flags;
    bit<13> frag_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header tcp_h {
    bit<16> src_port;
    bit<16> dst_port;
    
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4> data_offset;
    bit<4> res;
    bit<8> flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> udp_total_len;
    bit<16> checksum;
}

struct header_t {
    ethernet_h ethernet;
    ipv4_h ipv4;
    tcp_h tcp;
    udp_h udp;
}

struct paired_8bit {
    bit<8> lo;
    bit<8> hi;
}

//==HyperLogLog local variables
typedef bit<64> hll_flowid_t;
header hll_metadata_t {
    bit<8> maxzeroes;
    bit<32> hash_value;    
    bit<6> bin_id;
    bit<10> _padding;
    
    bit<8> shadow_maxzeroes;
    bit<32> inverse_probability_scaled_new;      // 2^ * (2**-maxzeroes).
    bit<32> inverse_probability_scaled_existing; // 2^ * (2**-shadow_maxzeroes)
    bit<32> invprob_diff;
    bit<32> sum_invprob_diff;
}
struct ig_metadata_t {
    hll_metadata_t hll_meta;
}
struct eg_metadata_t {
}


parser TofinoIngressParser(
        packet_in pkt,
        inout ig_metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {
    state start {
        pkt.extract(ig_intr_md);
        transition parse_port_metadata;
    }

    state parse_port_metadata {
        pkt.advance(PORT_METADATA_SIZE);  // arch-provided: 64 (Tofino1) / 192 (Tofino2)
        transition accept;
    }
}
parser SwitchIngressParser(
        packet_in pkt,
        out header_t hdr,
        out ig_metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    TofinoIngressParser() tofino_parser;

    state start {
        tofino_parser.apply(pkt, ig_md, ig_intr_md);
        transition parse_ethernet;
    }
    
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select (hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4 : parse_ipv4;
            default : reject;
        }
    }
    
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_TCP : parse_tcp;
            IP_PROTOCOLS_UDP : parse_udp;
            default : accept;
        }
    }
    
    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition select(hdr.ipv4.total_len) {
            default : accept;
        }
    }
    
    state parse_udp {
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port) {
            default: accept;
        }
    }
}

control SwitchIngressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in ig_metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md) {
    
    apply {        
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.tcp);
        pkt.emit(hdr.udp);
    }
}

// Do nothing in egress
parser SwitchEgressParser(
        packet_in pkt,
        out header_t hdr,
        out eg_metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }
}

control SwitchEgressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in eg_metadata_t eg_md,
        in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {
    apply {
    }
}


// == HyperLogLog logic control block ==
// Given fids, estimate the number of distinct fids seen.
// Note: it's more accurate to use the inversed_scaled_estimate. The inverse within data plane is approxiamte, thus there's additional error (6%~10%) introduced in estimate.
control HyperLogLog (
        in  hll_flowid_t fid,
        out hll_metadata_t hll_meta,
        out bit<32> inversed_scaled_estimate,
        out bit<32> estimate) {

        //== Step 1: prepare hashes; how many consecutive zeroes?
        Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_combined;
        action calc_hash(){
            hll_meta.hash_value=hash_combined.get({
                6w32,fid,
                4w8,fid,
                5w19,fid
                });
        }
        action set_maxzeroes(bit<8> m){
            hll_meta.maxzeroes=m;
        }
        bit<1> ignore_me;
        // Here we count the number of zeros; we also +1 to differentiate from "no value ever"
        table tb_maxzeroes{
            key = {
                hll_meta.hash_value[26-1:0]: ternary;
                ignore_me: ternary;
            }
            size = 32;
            actions = {
                set_maxzeroes;
            }
            default_action = set_maxzeroes(0);
            const entries = {
                    (26w1 &&& 26w1, 1w0&&&1w0): set_maxzeroes(1);
                    (26w2 &&& 26w3, 1w0&&&1w0): set_maxzeroes(2);
                    (26w4 &&& 26w7, 1w0&&&1w0): set_maxzeroes(3);
                    (26w8 &&& 26w15, 1w0&&&1w0): set_maxzeroes(4);
                    (26w16 &&& 26w31, 1w0&&&1w0): set_maxzeroes(5);
                    (26w32 &&& 26w63, 1w0&&&1w0): set_maxzeroes(6);
                    (26w64 &&& 26w127, 1w0&&&1w0): set_maxzeroes(7);
                    (26w128 &&& 26w255, 1w0&&&1w0): set_maxzeroes(8);
                    (26w256 &&& 26w511, 1w0&&&1w0): set_maxzeroes(9);
                    (26w512 &&& 26w1023, 1w0&&&1w0): set_maxzeroes(10);
                    (26w1024 &&& 26w2047, 1w0&&&1w0): set_maxzeroes(11);
                    (26w2048 &&& 26w4095, 1w0&&&1w0): set_maxzeroes(12);
                    (26w4096 &&& 26w8191, 1w0&&&1w0): set_maxzeroes(13);
                    (26w8192 &&& 26w16383, 1w0&&&1w0): set_maxzeroes(14);
                    (26w16384 &&& 26w32767, 1w0&&&1w0): set_maxzeroes(15);
                    (26w32768 &&& 26w65535, 1w0&&&1w0): set_maxzeroes(16);
                    (26w65536 &&& 26w131071, 1w0&&&1w0): set_maxzeroes(17);
                    (26w131072 &&& 26w262143, 1w0&&&1w0): set_maxzeroes(18);
                    (26w262144 &&& 26w524287, 1w0&&&1w0): set_maxzeroes(19);
                    (26w524288 &&& 26w1048575, 1w0&&&1w0): set_maxzeroes(20);
            }
        }
#if 6+20 >= 32
    #warning SCALING and M mismatch, there's not that many zeros for you to check -- only (32-LOG_M) bits in hash function left.
#endif
        action set_bin_id(){
            hll_meta.bin_id=hll_meta.hash_value[31:(32-6)];
        }

        //== Step 2: Use register to save max num zero
        Register<paired_8bit,_>(256) reg_estimators_maxzeros;
        RegisterAction<paired_8bit, bit<6>, bit<8>>(reg_estimators_maxzeros) swap_if_larger = {
            void apply(inout paired_8bit val, out bit<8> rv) {
                paired_8bit in_value = val;
                //lo: estimator max, hi: auxiliary for return value
                if(hll_meta.maxzeroes>val.lo){
                    val.lo=hll_meta.maxzeroes;
                    val.hi=in_value.lo;
                }else{
                    val.lo=in_value.lo;
                    val.hi=hll_meta.maxzeroes;
                }
                rv=val.hi;
            }
        };
        action regexec_swap_if_larger(){
            hll_meta.shadow_maxzeroes = swap_if_larger.execute(hll_meta.bin_id);
        }

        //== Step 3: Invert maxzeroes back into probabilities, to calculate (1/2^maxzeroes-1/2^shadow_maxzeroes)
        action set_invprob_maxz(bit<32> inv){
            hll_meta.inverse_probability_scaled_new=inv; // 2^20 * (2**-maxzeroes).
        }
        table tb_invert_maxz {
            key = {
                hll_meta.maxzeroes: exact;
            }
            size = 32;
            actions = {
                set_invprob_maxz;
            }
            const entries = {
                    0: set_invprob_maxz(32w1048576);
                    1: set_invprob_maxz(32w524288);
                    2: set_invprob_maxz(32w262144);
                    3: set_invprob_maxz(32w131072);
                    4: set_invprob_maxz(32w65536);
                    5: set_invprob_maxz(32w32768);
                    6: set_invprob_maxz(32w16384);
                    7: set_invprob_maxz(32w8192);
                    8: set_invprob_maxz(32w4096);
                    9: set_invprob_maxz(32w2048);
                    10: set_invprob_maxz(32w1024);
                    11: set_invprob_maxz(32w512);
                    12: set_invprob_maxz(32w256);
                    13: set_invprob_maxz(32w128);
                    14: set_invprob_maxz(32w64);
                    15: set_invprob_maxz(32w32);
                    16: set_invprob_maxz(32w16);
                    17: set_invprob_maxz(32w8);
                    18: set_invprob_maxz(32w4);
                    19: set_invprob_maxz(32w2);
                    20: set_invprob_maxz(32w1);
            }
        }
        action set_invprob_shadow_maxz(bit<32> inv){
            hll_meta.inverse_probability_scaled_existing=inv;    // 2^20 * (2**-shadow_maxzeroes)
        }
        table tb_invert_shadow_maxz {
            key = {
                hll_meta.shadow_maxzeroes: exact;
            }
            size = 32;
            actions = {
                set_invprob_shadow_maxz;
            }
            
            const entries = {
                    0: set_invprob_shadow_maxz(32w1048576);
                    1: set_invprob_shadow_maxz(32w524288);
                    2: set_invprob_shadow_maxz(32w262144);
                    3: set_invprob_shadow_maxz(32w131072);
                    4: set_invprob_shadow_maxz(32w65536);
                    5: set_invprob_shadow_maxz(32w32768);
                    6: set_invprob_shadow_maxz(32w16384);
                    7: set_invprob_shadow_maxz(32w8192);
                    8: set_invprob_shadow_maxz(32w4096);
                    9: set_invprob_shadow_maxz(32w2048);
                    10: set_invprob_shadow_maxz(32w1024);
                    11: set_invprob_shadow_maxz(32w512);
                    12: set_invprob_shadow_maxz(32w256);
                    13: set_invprob_shadow_maxz(32w128);
                    14: set_invprob_shadow_maxz(32w64);
                    15: set_invprob_shadow_maxz(32w32);
                    16: set_invprob_shadow_maxz(32w16);
                    17: set_invprob_shadow_maxz(32w8);
                    18: set_invprob_shadow_maxz(32w4);
                    19: set_invprob_shadow_maxz(32w2);
                    20: set_invprob_shadow_maxz(32w1);
            }
        }
        action calc_diff_maxz(){
            hll_meta.invprob_diff=(hll_meta.inverse_probability_scaled_existing-hll_meta.inverse_probability_scaled_new);
        }
        //Accumulate the difference to calculate a sum

        //== Step 4: sum of inverse
        Register<bit<32>,_>(1) reg_accumulator;
        RegisterAction<bit<32>, bit<32>, bit<32>>(reg_accumulator) regact_sum_invprob_diff = {
            void apply(inout bit<32> val, out bit<32> rv) {
                val = val + hll_meta.invprob_diff;
                rv = val;
            }
        };
        action regexec_sum_invprob_diff(){
            hll_meta.sum_invprob_diff = regact_sum_invprob_diff.execute( 0 );
        }
#if 1048576*64 > 4294967296
    #error Scaling too big, the sum of initial inversed probability (M*2^SCALING) exceeded 2^32.
#endif
        action const_offset_sum_invprob(){
            inversed_scaled_estimate = ( 1048576*64 ) - hll_meta.sum_invprob_diff;
        }

        //==Step 5: get estimate! which should be inverse.
        // formula: alpha*m*m/sum( 2**-maxzeroes )
        // inversed_scaled_estimate is 2^* sum(2**-est)
        // estimate = alpha*m*m*(2^)/inversed_scaled_estimate
#define MAGNIFY_FACTOR 3046596202
#if MAGNIFY_FACTOR >= 4294967296
    #error Scaling and M combination invalid, we need to divide the inversed sum by MAGNIFY_FACTOR, which is now larger than 2^32.
#endif
        MathUnit<bit<32>>(MathOp_t.DIV, MAGNIFY_FACTOR) mu_for_inverse;
        Register <bit<32>, _> (1) reg_dummy_inverse;
        RegisterAction<bit<32>, bit<32>, bit<32>>(reg_dummy_inverse) regact_calc_inverse = {
              void apply(inout bit<32> val, out bit<32> rv){
                    val=mu_for_inverse.execute(inversed_scaled_estimate);
                    rv=val;
             }
        };
        action calc_final_inverse(){
            estimate=regact_calc_inverse.execute(0);
        }

        //==Step 6: Linear Counting correction
        Register<bit<32>,_>(1) reg_nonzero_estimators_count;
        RegisterAction<bit<32>, bit<32>, bit<32>>(reg_nonzero_estimators_count) regact_nonzero_estimators_count = {
            void apply(inout bit<32> val, out bit<32> rv) {
                if(hll_meta.shadow_maxzeroes == 0){ //first time an estimator set to nonzero
                    val = val + 1;
                }
                rv = val;
            }
        };
        bit<7> nonzero_estimators_count;
        action regexec_nonzero_estimators_count(){
            nonzero_estimators_count = (bit<7>) regact_nonzero_estimators_count.execute( 0 );
        }
        action lc_correction(bit<32> val){
            estimate=val;
        }
        action lc_keep(){}
        table tb_linear_counter_correction{
            key = {
                nonzero_estimators_count: exact;
            }
            size = 65;
            actions = {
                lc_correction;
                lc_keep;
            }
            const entries = {
                    (0): lc_correction( 0 );
                    (1): lc_correction( 1 );
                    (2): lc_correction( 2 );
                    (3): lc_correction( 3 );
                    (4): lc_correction( 4 );
                    (5): lc_correction( 5 );
                    (6): lc_correction( 6 );
                    (7): lc_correction( 7 );
                    (8): lc_correction( 8 );
                    (9): lc_correction( 9 );
                    (10): lc_correction( 10 );
                    (11): lc_correction( 12 );
                    (12): lc_correction( 13 );
                    (13): lc_correction( 14 );
                    (14): lc_correction( 15 );
                    (15): lc_correction( 17 );
                    (16): lc_correction( 18 );
                    (17): lc_correction( 19 );
                    (18): lc_correction( 21 );
                    (19): lc_correction( 22 );
                    (20): lc_correction( 23 );
                    (21): lc_correction( 25 );
                    (22): lc_correction( 26 );
                    (23): lc_correction( 28 );
                    (24): lc_correction( 30 );
                    (25): lc_correction( 31 );
                    (26): lc_correction( 33 );
                    (27): lc_correction( 35 );
                    (28): lc_correction( 36 );
                    (29): lc_correction( 38 );
                    (30): lc_correction( 40 );
                    (31): lc_correction( 42 );
                    (32): lc_correction( 44 );
                    (33): lc_correction( 46 );
                    (34): lc_correction( 48 );
                    (35): lc_correction( 50 );
                    (36): lc_correction( 52 );
                    (37): lc_correction( 55 );
                    (38): lc_correction( 57 );
                    (39): lc_correction( 60 );
                    (40): lc_correction( 62 );
                    (41): lc_correction( 65 );
                    (42): lc_correction( 68 );
                    (43): lc_correction( 71 );
                    (44): lc_correction( 74 );
                    (45): lc_correction( 77 );
                    (46): lc_correction( 81 );
                    (47): lc_correction( 84 );
                    (48): lc_correction( 88 );
                    (49): lc_correction( 92 );
                    (50): lc_correction( 97 );
                    (51): lc_correction( 102 );
                    (52): lc_correction( 107 );
                    (53): lc_correction( 112 );
                    (54): lc_correction( 118 );
                    (55): lc_correction( 125 );
                    (56): lc_correction( 133 );
                    (57): lc_correction( 141 );
                    (58): lc_correction( 151 );
                    (59): lc_correction( 163 );
                    (60): lc_correction( 177 );
                    (61): lc_correction( 195 );
                    (62): lc_correction( 221 );
                    (63): lc_correction( 266 );
                (64): lc_keep();
            }
        }
    
    apply {
        //stateless: init hash values
        calc_hash();
        tb_maxzeroes.apply();
        set_bin_id();

        //stateful: maintain HLL estimators (max number of zeros)
        regexec_swap_if_larger();

        //stateless: inverse probability
        tb_invert_maxz.apply();
        tb_invert_shadow_maxz.apply();
        calc_diff_maxz();

        //stateful: get sum of inverse
        regexec_sum_invprob_diff();
        const_offset_sum_invprob();
        calc_final_inverse();

        //bonus: linear counter correction, if estimate < 2.5*M
        regexec_nonzero_estimators_count();
        if(estimate[31:8]==0){
            if(estimate[7:0] < 160){
                tb_linear_counter_correction.apply();
            }
        }
    }
}

// == Main pipeline control block ==
control SwitchIngress(
        inout header_t hdr,
        inout ig_metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {
        
        HyperLogLog() hyperloglog;
        
        action nop() {
        }
        action drop() {
            ig_intr_dprsr_md.drop_ctl = 0x1; // Drop packet.
        }
        action reflect(){
            //send you back to where you're from
            ig_intr_tm_md.ucast_egress_port=ig_intr_md.ingress_port;
        }
        action route_to(bit<9> port){
            //route to CPU NIC. on model, it is veth250
            ig_intr_tm_md.ucast_egress_port=port;
        }

        apply {
            bit<32> estimate;
            bit<32> inversed_scaled_estimate;
            hll_flowid_t fid=hdr.ipv4.src_addr ++ hdr.ipv4.dst_addr;
            hyperloglog.apply(fid, ig_md.hll_meta, inversed_scaled_estimate, estimate);
            
            //for demo purpose, send back to incoming port
            reflect();
            hdr.ethernet.src_addr=(bit<48>) estimate;
        }
}

control SwitchEgress(
        inout header_t hdr,
        inout eg_metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    apply {
    }
}

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         SwitchEgressParser(),
         SwitchEgress(),
         SwitchEgressDeparser()
         ) pipe;

Switch(pipe) main;