#!/usr/bin/env python3

from os import geteuid

from util import *

RECIRCULATION_PORT = 6
SERVER_PORT = 1
CLIENT_PORTS = [p for p in range(3, 33)]


def main():
    ports = Ports([SERVER_PORT] + CLIENT_PORTS)

    for client_port in CLIENT_PORTS:
        print(f"[*] Testing port {client_port}...")

        client_req = build_kvs_hdr(op=KVS_OP_GET, value=bytes(KVS_VALUE_SIZE_BYTES))
        server_req = build_kvs_hdr(op=KVS_OP_GET, key=client_req.key, value=client_req.value, status=KVS_STATUS_FAIL, port=client_port)

        client_pkt = build_packet(kvs_hdr=client_req)
        server_pkt = build_packet(kvs_hdr=server_req)

        ports.send(client_port, client_pkt)
        expect_packet_from_port(ports, SERVER_PORT, server_pkt)


if __name__ == "__main__":
    if geteuid() != 0:
        print("Run with sudo.")
        exit(1)

    main()
