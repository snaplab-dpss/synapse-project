#!/usr/bin/env python3

from os import geteuid
from random import choice

from util import *

RECIRCULATION_PORT = 6
SERVER_PORT = 1
CLIENT_PORTS = [p for p in range(3, 33)]


def client_port_to_nf_dev(client_port):
    return client_port - 1


def main():
    ports = Ports([SERVER_PORT] + CLIENT_PORTS)

    kvs_hdr = build_kvs_hdr()
    flow = build_flow(dst_port=KVS_PORT)
    client_port = choice(CLIENT_PORTS)
    client_nf_dev = bswap16(client_port_to_nf_dev(client_port))

    kvs_server_req = build_kvs_hdr(op=KVS_OP_GET, key=kvs_hdr.key, value=kvs_hdr.value, port=client_nf_dev)
    kvs_server_pkt = build_packet(src_mac=SRC_MAC, dst_mac=DST_MAC, flow=flow, kvs_hdr=kvs_server_req)

    print(f"[*] Testing KVS server response (client={client_port})...")
    ports.send(SERVER_PORT, kvs_server_pkt)
    expect_packet_from_port(
        ports,
        client_port,
        kvs_server_pkt,
        ignore_fields=[HeaderField.IP_CHECKSUM, HeaderField.UDP_CHECKSUM, HeaderField.KVS_CLIENT_PORT],
    )

    # testing_ports = CLIENT_PORTS.copy()
    testing_ports = [choice(CLIENT_PORTS)]

    for client_port in testing_ports:
        print(f"[*] Testing port {client_port}...")

        kvs_hdr = build_kvs_hdr()
        flow = build_flow(dst_port=KVS_PORT)
        inverted_flow = flow.invert()

        print(f"[{client_port}] Sending GET request")

        kvs_client_req = kvs_hdr.copy()
        kvs_client_req.op = KVS_OP_GET
        client_pkt = build_packet(src_mac=SRC_MAC, dst_mac=DST_MAC, flow=flow, kvs_hdr=kvs_client_req)

        kvs_server_req = kvs_client_req.copy()
        kvs_server_req.port = bswap16(client_port_to_nf_dev(client_port))
        kvs_server_pkt = build_packet(src_mac=SRC_MAC, dst_mac=DST_MAC, flow=flow, kvs_hdr=kvs_server_req)

        ports.send(client_port, client_pkt)
        expect_packet_from_port(
            ports,
            SERVER_PORT,
            kvs_server_pkt,
            ignore_fields=[HeaderField.IP_CHECKSUM, HeaderField.UDP_CHECKSUM],
        )

        print(f"[{client_port}] Sending PUT request to cache")

        kvs_req = build_kvs_hdr(op=KVS_OP_PUT, key=kvs_hdr.key, value=kvs_hdr.value)
        ksv_res = build_kvs_hdr(op=KVS_OP_PUT, key=kvs_hdr.key, value=kvs_hdr.value, status=KVS_STATUS_OK)

        client_pkt = build_packet(src_mac=SRC_MAC, dst_mac=DST_MAC, flow=flow, kvs_hdr=kvs_req)
        res_pkt = build_packet(src_mac=DST_MAC, dst_mac=SRC_MAC, flow=inverted_flow, kvs_hdr=ksv_res)

        ports.send(client_port, client_pkt)
        expect_packet_from_port(
            ports,
            client_port,
            res_pkt,
            ignore_fields=[HeaderField.IP_CHECKSUM, HeaderField.UDP_CHECKSUM, HeaderField.KVS_CLIENT_PORT],
        )

        print(f"[{client_port}] Sending cached GET requests")

        for _ in range(5):
            kvs_req = build_kvs_hdr(op=KVS_OP_GET, key=kvs_hdr.key, value=kvs_hdr.value)
            ksv_res = build_kvs_hdr(op=KVS_OP_GET, key=kvs_hdr.key, value=kvs_hdr.value, status=KVS_STATUS_OK)

            client_pkt = build_packet(src_mac=SRC_MAC, dst_mac=DST_MAC, flow=flow, kvs_hdr=kvs_req)
            res_pkt = build_packet(src_mac=DST_MAC, dst_mac=SRC_MAC, flow=inverted_flow, kvs_hdr=ksv_res)

            ports.send(client_port, client_pkt)
            expect_packet_from_port(
                ports,
                client_port,
                res_pkt,
                ignore_fields=[HeaderField.IP_CHECKSUM, HeaderField.UDP_CHECKSUM, HeaderField.KVS_CLIENT_PORT],
            )

        print(f"[{client_port}] Sending cached PUT requests")

        for _ in range(5):
            kvs_req = build_kvs_hdr(op=KVS_OP_PUT, key=kvs_hdr.key)
            ksv_res = build_kvs_hdr(op=KVS_OP_PUT, key=kvs_hdr.key, value=kvs_req.value, status=KVS_STATUS_OK)

            client_pkt = build_packet(src_mac=SRC_MAC, dst_mac=DST_MAC, flow=flow, kvs_hdr=kvs_req)
            res_pkt = build_packet(src_mac=DST_MAC, dst_mac=SRC_MAC, flow=inverted_flow, kvs_hdr=ksv_res)

            ports.send(client_port, client_pkt)
            expect_packet_from_port(
                ports,
                client_port,
                res_pkt,
                ignore_fields=[HeaderField.IP_CHECKSUM, HeaderField.UDP_CHECKSUM, HeaderField.KVS_CLIENT_PORT],
            )


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
