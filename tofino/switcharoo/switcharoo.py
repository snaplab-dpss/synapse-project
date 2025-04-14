#!/usr/bin/env python3

import bfrt_grpc.client as gc

GRPC_SERVER_IP = "127.0.0.1"
GRPC_SERVER_PORT = 50052
P4_PROGRAM_NAME = "switcharoo"
DEFAULT_PORT_SPEED = 100

FRONT_PANEL_PORTS = [p for p in range(1, 33)]


class Ports:
    def __init__(self, bfrt_info):
        self.bfrt_info = bfrt_info
        self.port_table = self.bfrt_info.table_get("$PORT")
        self.port_hdl_info_table = self.bfrt_info.table_get("$PORT_HDL_INFO")
        self.port_stat = self.bfrt_info.table_get("$PORT_STAT")

        self.dev_to_front_panel = {}
        self.front_panel_to_dev_port = {}

    def get_dev_port(self, front_panel_port, lane=0):
        target = gc.Target(device_id=0, pipe_id=0xFFFF)

        key = self.port_hdl_info_table.make_key([gc.KeyTuple("$CONN_ID", front_panel_port), gc.KeyTuple("$CHNL_ID", lane)])
        resp = self.port_hdl_info_table.entry_get(target, [key], {"from_hw": False})

        dev_port = next(resp)[0].to_dict()["$DEV_PORT"]
        self.dev_to_front_panel[dev_port] = front_panel_port
        self.front_panel_to_dev_port[front_panel_port] = dev_port

        return dev_port

    def get_dev_ports(self, front_panel_ports):
        target = gc.Target(device_id=0, pipe_id=0xFFFF)

        resp = self.port_hdl_info_table.entry_get(
            target,
            [
                self.port_hdl_info_table.make_key(
                    [
                        gc.KeyTuple("$CONN_ID", front_panel_port),
                        gc.KeyTuple("$CHNL_ID", 0),
                    ]
                )
                for front_panel_port in front_panel_ports
            ],
            {"from_hw": False},
        )

        for entry in resp:
            data = entry[0].to_dict()
            key = entry[1].to_dict()

            dev_port = data["$DEV_PORT"]
            front_panel_port = key["$CONN_ID"]["value"]

            self.dev_to_front_panel[dev_port] = front_panel_port
            self.front_panel_to_dev_port[front_panel_port] = dev_port

        return self.front_panel_to_dev_port

    def get_front_panel_port(self, dev_port):
        if dev_port in self.dev_to_front_panel:
            return self.dev_to_front_panel[dev_port]

        self.get_dev_ports(FRONT_PANEL_PORTS)
        print("I want", dev_port)
        print(self.dev_to_front_panel)
        return self.dev_to_front_panel[dev_port]

    # Port list is a list of tuples: (front panel port, lane, speed, FEC string).
    # Speed is one of {10, 25, 40, 50, 100}.
    # FEC string is one of {"none", "fc", "rs"}.
    # Look in $SDE_INSTALL/share/bf_rt_shared/*.json for more info.
    def add_ports(self, port_list):
        speed_conversion_table = {
            10: "BF_SPEED_10G",
            25: "BF_SPEED_25G",
            40: "BF_SPEED_40G",
            50: "BF_SPEED_50G",
            100: "BF_SPEED_100G",
        }

        fec_conversion_table = {
            "none": "BF_FEC_TYP_NONE",
            "fec": "BF_FEC_TYP_FC",
            "rs": "BF_FEC_TYP_RS",
        }

        speed_to_fec = {
            10: "none",
            25: "none",
            40: "none",
            50: "none",
            100: "rs",
        }

        target = gc.Target(device_id=0, pipe_id=0xFFFF)

        for front_panel_port, speed in port_list:
            fec = speed_to_fec[speed]
            key = self.port_table.make_key([gc.KeyTuple("$DEV_PORT", self.get_dev_port(front_panel_port))])
            data = self.port_table.make_data(
                [
                    gc.DataTuple("$SPEED", str_val=speed_conversion_table[speed]),
                    gc.DataTuple("$FEC", str_val=fec_conversion_table[fec]),
                    gc.DataTuple("$PORT_ENABLE", bool_val=True),
                ]
            )

            self.port_table.entry_add(target, [key], [data])

    def add_port(self, front_panel_port, speed):
        self.add_ports([(front_panel_port, speed)])

    def get_ports_state(self, dev_ports):
        target = gc.Target(device_id=0, pipe_id=0xFFFF)

        resp = self.port_table.entry_get(
            target,
            [self.port_table.make_key([gc.KeyTuple("$DEV_PORT", dev_port)]) for dev_port in dev_ports],
            {"from_hw": True},
        )

        state = {}
        for entry in resp:
            data = entry[0].to_dict()
            key = entry[1].to_dict()

            dev_port = key["$DEV_PORT"]["value"]
            speed = data["$SPEED"]
            fec = data["$FEC"]
            port_enable = data["$PORT_ENABLE"]
            loopback_mode = data["$LOOPBACK_MODE"]
            port_up = data["$PORT_UP"]

            state[dev_port] = {
                "speed": speed,
                "fec": fec,
                "loopback_mode": loopback_mode,
                "port_enable": port_enable,
                "port_up": port_up,
            }

        return state

    def get_ports_stats(self, dev_ports):
        target = gc.Target(device_id=0, pipe_id=0xFFFF)

        resp = self.port_stat.entry_get(
            target,
            [self.port_table.make_key([gc.KeyTuple("$DEV_PORT", dev_port)]) for dev_port in dev_ports],
            {"from_hw": True},
        )

        stats = {}
        for entry in resp:
            data = entry[0].to_dict()
            key = entry[1].to_dict()

            dev_port = key["$DEV_PORT"]["value"]
            FramesTransmittedOK = data["$FramesTransmittedOK"]
            FramesReceivedOK = data["$FramesReceivedOK"]

            stats[dev_port] = {
                "FramesReceivedOK": FramesReceivedOK,
                "FramesTransmittedOK": FramesTransmittedOK,
            }

        return stats


if __name__ == "__main__":
    grpc_client = gc.ClientInterface("{}:{}".format(GRPC_SERVER_IP, GRPC_SERVER_PORT), 0, 0)
    grpc_client.bind_pipeline_config(P4_PROGRAM_NAME)
    bfrt_info = grpc_client.bfrt_info_get(P4_PROGRAM_NAME)

    ports = Ports(bfrt_info)

    for port in FRONT_PANEL_PORTS:
        ports.add_port(port, DEFAULT_PORT_SPEED)

    print("Configured ports: {}".format(FRONT_PANEL_PORTS))
