#!/usr/bin/env python3

import os
import sys
import signal
import argparse

from pprint import pformat

try:
    import bfrt_grpc.client as gc
except:
    python_v = '{}.{}'.format(sys.version_info.major, sys.version_info.minor)
    sde_install = os.environ['SDE_INSTALL']
    
    tofino_libs = '{}/lib/python{}/site-packages/tofino'.format(sde_install, python_v)
    third_party_libs = '{}/lib/python{}/site-packages'.format(sde_install, python_v)

    sys.path.append(tofino_libs)
    sys.path.append(third_party_libs)

    import bfrt_grpc.client as gc

P4_PROGRAM_NAME = 'perfmodel'
PADDING = 30

class Table(object):
    def __init__(self, bfrt_info):
        self.bfrt_info = bfrt_info

        # child clases must set table
        self.table = None

        # lowest possible  priority for ternary match rules
        self.lowest_priority = 1 << 24

    def clear(self):
        """Remove all existing entries in table."""
        if self.table is not None:
            # target all pipes on device 0
            target = gc.Target(device_id=0, pipe_id=0xffff)

            # get all keys in table
            resp = self.table.entry_get(target, [], {"from_hw": False})

            # delete all keys in table
            for _, key in resp:
                if key:
                    self.table.entry_del(target, [key])

    def print_current_state(self):
        print()
        print("====================================================")

        print("Table info:")
        print("  ID   {}".format(self.table.info.id_get()))
        print("  Name {}".format(self.table.info.name_get()))

        actions = self.table.info.action_name_list_get()
        data_fields = self.table.info.data_field_name_list_get()
        key_fields = self.table.info.key_field_name_list_get()
        attributes = self.table.info.attributes_supported_get()

        print("  Actions:")
        for action in actions:
            print("  {}".format(action))

        print("  Data fields:")
        for data_field in data_fields:
            data_field_size = self.table.info.data_field_size_get(data_field)
            data_field_type = self.table.info.data_field_size_get(data_field)

            print("    Name {}".format(data_field))
            print("    Size {} bytes, {} bits".format(data_field_size[0], data_field_size[1]))
            print("    Type {}".format(data_field_type))
        
        print("  Keys:")
        for key_field in key_fields:
            key_field_size = self.table.info.key_field_size_get(key_field)
            key_field_type = self.table.info.key_field_type_get(key_field)

            print("    Name {}".format(key_field))
            print("    Size {} bytes, {} bits".format(key_field_size[0], key_field_size[1]))
            print("    Type {}".format(key_field_type))

        print("  Attributes:")
        for attribute in attributes:
            print("    Name {}".format(attribute))

        target = gc.Target(device_id=0, pipe_id=0xffff)
        keys = self.table.entry_get(target, [], {"from_hw": False})

        print("Table entries:")
        for data, key in keys:
            print("  -------------------------------------------------")
            for key_field in key.to_dict().keys():
                print("  {} {}".format(key_field.ljust(PADDING), key.to_dict()[key_field]['value']))

            for data_field in data.to_dict().keys():
                if data_field in [ 'is_default_entry', 'action_name' ]:
                    continue

                print("  {} {}".format(data_field.ljust(PADDING), data.to_dict()[data_field]))

            print("  {} {}".format("Actions".ljust(PADDING), data.to_dict()['action_name']))

        print("====================================================")
        print()

class Ports(object):
    def __init__(self, bfrt_info):
        self.bfrt_info = bfrt_info
        self.dev_to_front_panel = {}

    # get dev port
    def get_dev_port(self, front_panel_port, lane):        
        # target all pipes on device 0
        target = gc.Target(device_id=0, pipe_id=0xffff)
        
        port_hdl_info_table = self.bfrt_info.table_get("$PORT_HDL_INFO")
        
        # convert front-panel port to dev port
        resp = port_hdl_info_table.entry_get(
            target,
            [port_hdl_info_table.make_key([gc.KeyTuple('$CONN_ID', front_panel_port),
                                           gc.KeyTuple('$CHNL_ID', lane)])], {"from_hw": False})

        dev_port = next(resp)[0].to_dict()["$DEV_PORT"]
        self.dev_to_front_panel[dev_port] = front_panel_port

        return dev_port

    def get_all_front_panel_ports(self):
        # target all pipes on device 0
        target = gc.Target(device_id=0, pipe_id=0xffff)
        port_hdl_info_table = self.bfrt_info.table_get("$PORT_HDL_INFO")

        def clean_query(target, port_hdl_info_table):
            front_panel_ports = set()
            resp = port_hdl_info_table.entry_get(target, [], {"from_hw": False})

            for _, data in resp:
                data_dict = data.to_dict()

                front_panel_port = data_dict["$CONN_ID"]["value"]
                front_panel_ports.add(front_panel_port)

            return list(front_panel_ports)
        
        def old_query(target, port_hdl_info_table):
            # This is kind of janky, but we can only query the entire
            # table on some switches, who knows why.
            # Hence, one by one it is.

            front_panel_ports = []
            front_panel_port = 1
            while True:
                try:
                    resp = port_hdl_info_table.entry_get(
                        target,
                        [port_hdl_info_table.make_key([gc.KeyTuple('$CONN_ID', front_panel_port),
                                                    gc.KeyTuple('$CHNL_ID', 0)])], {"from_hw": False})
                    _, data = next(resp)
                    data_dict = data.to_dict()

                    port = data_dict["$CONN_ID"]["value"]
                    assert port == front_panel_port

                    front_panel_ports.append(port)
                    front_panel_port += 1
                except:
                    break
            
            # We expect 32 ports, not 33
            if len(front_panel_ports) == 33:
                front_panel_ports.pop()

            return front_panel_ports
        
        try:
            front_panel_ports = clean_query(target, port_hdl_info_table)
        except:
            front_panel_ports = old_query(target, port_hdl_info_table)
        
        assert len(front_panel_ports) in [ 16, 32, 64 ]
        return front_panel_ports

    # add ports
    #
    # port list is a list of tuples: (front panel port, lane, speed, FEC string)
    # speed is one of {10, 25, 40, 50, 100}
    # FEC string is one of {'none', 'fc', 'rs'}
    # Look in $SDE_INSTALL/share/bf_rt_shared/*.json for more info
    def add_ports(self, port_list):
        print("Bringing up ports:\n {}".format(pformat(port_list)))
        
        speed_conversion_table = {10: "BF_SPEED_10G",
                                  25: "BF_SPEED_25G",
                                  40: "BF_SPEED_40G",
                                  50: "BF_SPEED_50G",
                                  100: "BF_SPEED_100G"}
        
        fec_conversion_table = {'none': "BF_FEC_TYP_NONE",
                                'fec': "BF_FEC_TYP_FC",
                                'rs': "BF_FEC_TYP_RS"}

        speed_to_fec = {
            10: 'none',
            25: 'none',
            40: 'none',
            50: 'none',
            100: 'rs',
        }
        
        # target all pipes on device 0
        target = gc.Target(device_id=0, pipe_id=0xffff)
        
        # get relevant table
        port_table = self.bfrt_info.table_get("$PORT")

        for (front_panel_port, speed) in port_list:
            fec = speed_to_fec[speed]
            lane = 0
            
            print("Adding port {}".format((front_panel_port, lane, speed, fec)))
            port_table.entry_add(
                target,
                [port_table.make_key([gc.KeyTuple('$DEV_PORT', self.get_dev_port(front_panel_port, lane))])],
                [port_table.make_data([gc.DataTuple('$SPEED', str_val=speed_conversion_table[speed]),
                                       gc.DataTuple('$FEC', str_val=fec_conversion_table[fec]),
                                       gc.DataTuple('$PORT_ENABLE', bool_val=True)])])
            
    # add one port
    def add_port(self, front_panel_port, speed):
        self.add_ports([(front_panel_port, speed)])
            
    # delete all ports
    def delete_all_ports(self):
        print("Deleting all ports...")
        
        # target all pipes on device 0
        target = gc.Target(device_id=0, pipe_id=0xffff)
        
        # list of all possible external dev ports (don't touch internal ones)
        two_pipe_dev_ports = list(range(0,64)) + list(range(128,192))
        four_pipe_dev_ports = two_pipe_dev_ports + list(range(256,320)) + list(range(384,448))
        
        # get relevant table
        port_table = self.bfrt_info.table_get("$PORT")
        
        # delete all dev ports from largest device
        # (can't use the entry_get -> entry_del process for port_table like we can for normal tables)
        dev_ports = four_pipe_dev_ports
        port_table.entry_del(
            target,
            [port_table.make_key([gc.KeyTuple('$DEV_PORT', i)])
             for i in dev_ports])
    
    def get_available_ports(self):
        port_table = self.bfrt_info.table_get("$PORT")
        target = gc.Target(device_id=0, pipe_id=0xffff)
        keys = port_table.entry_get(target, [], {"from_hw": False})

        ports = []

        for data, key in keys:
            key_dict = key.to_dict()
            data_dict = data.to_dict()

            port = {
                'port': data_dict['$CONN_ID'],
                'valid': data_dict['$IS_VALID'],
                'enabled': data_dict['$PORT_ENABLE'],
                'up': data_dict['$PORT_UP'],
            }

            ports.append(port)

        return ports
    
    def get_enabled_ports(self):
        port_table = self.bfrt_info.table_get("$PORT")
        target = gc.Target(device_id=0, pipe_id=0xffff)
        keys = port_table.entry_get(target, [], {"from_hw": False})

        ports = []

        for data, key in keys:
            key_dict = key.to_dict()
            data_dict = data.to_dict()

            port = {
                'port': data_dict['$CONN_ID'],
                'valid': data_dict['$IS_VALID'],
                'enabled': data_dict['$PORT_ENABLE'],
                'up': data_dict['$PORT_UP'],
            }

            if data_dict['$PORT_ENABLE']:   
                ports.append(port['port'])

        return ports

# brute force
def find_closest_approximation(capacity, rs):
    winner = -1
    delta_winner = -1

    for i in range(capacity + 1):
        approximation = float(i) / capacity
        delta = abs(approximation-rs)

        if winner == -1 or delta_winner > delta:
            winner = i
            delta_winner = delta
    
    return winner

class RS_Table(Table):
    def __init__(self, bfrt_info, rs, rs_i):
        # set up base class
        super(RS_Table, self).__init__(bfrt_info)

        print("Setting up rs0_table table...")
        
        # get this table
        self.rs_i = rs_i
        self.table = self.bfrt_info.table_get(f"Ingress.rs{rs_i}_table")

        capacity = self.table.info.size_get()

        total_recirc = find_closest_approximation(capacity, rs)
        error = abs(rs - float(total_recirc) / capacity)
        print("rs{}={} recirc={}/{} error={}".format(rs_i, rs, total_recirc, capacity, error))

        for i in range(capacity):
            if i < total_recirc:
                self.set_recirc(i)
            else:
                self.set_no_recirc(i)

    def set_no_recirc(self, i):
        self._add_or_mod_entry(i, 'no_recirc')
    
    def set_recirc(self, i):
        self._add_or_mod_entry(i, 'recirc')
    
    def _add_or_mod_entry(self, i, action):
        try:
            self._add_entry(i, action)
        except gc.BfruntimeReadWriteRpcException:
            self._mod_entry(i, action)
    
    def _add_entry(self, i, action):
        # target all pipes on device 0
        target = gc.Target(device_id=0, pipe_id=0xffff)
        
        self.table.entry_add(
            target,
            [
                self.table.make_key([gc.KeyTuple('rs{}_counter_value'.format(self.rs_i), i)])
            ],
            [
                self.table.make_data([], action)
            ]
        )

    def _mod_entry(self, i, action):
        # target all pipes on device 0
        target = gc.Target(device_id=0, pipe_id=0xffff)
        
        self.table.entry_mod(
            target,
            [
                self.table.make_key([gc.KeyTuple('rs{}_counter_value'.format(self.rs_i), i)])
            ],
            [
                self.table.make_data([], action)
            ]
        )

def setup_grpc_client(server, port):
    print("Connecting to GRPC server {}:{} and binding to program {}...".format(server, port, P4_PROGRAM_NAME))

    grpc_client = gc.ClientInterface("{}:{}".format(args.grpc_server, args.grpc_port), 0, 0)
    grpc_client.bind_pipeline_config(P4_PROGRAM_NAME)

    return grpc_client

def configure_switch(grpc_client, rs0, rs1):
    # get all tables for program
    bfrt_info = grpc_client.bfrt_info_get(P4_PROGRAM_NAME)

    # setup ports
    ports = Ports(bfrt_info)

    all_ports = ports.get_all_front_panel_ports()
    default_speed = 100
    topology = { "ports": [] }
    for port in all_ports:
        topology["ports"].append({
            "port": port,
            "capacity": default_speed,
        })

    for entry in topology['ports']:
        ports.add_port(entry['port'], entry['capacity'])
    
    rs0_table = RS_Table(bfrt_info, rs0, 0)
    rs0_table.print_current_state()

    rs1_table = RS_Table(bfrt_info, rs1, 1)
    rs1_table.print_current_state()

    print("Switch configured successfully!")

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(description="Controller.")

    argparser.add_argument('--grpc_server', type=str, default='127.0.0.1', help='GRPC server name/address')
    argparser.add_argument('--grpc_port', type=int, default=50052, help='GRPC server port')
    argparser.add_argument('--rs0', type=float, required=True, help='Recirculation Surplus (0<=rs0<=1)')
    argparser.add_argument('--rs1', type=float, required=True, help='Recirculation Surplus (0<=rs1<=1)')
    
    args = argparser.parse_args()

    assert args.rs0 >= 0
    assert args.rs0 <= 1

    assert args.rs1 >= 0
    assert args.rs1 <= 1

    grpc_client = setup_grpc_client(args.grpc_server, args.grpc_port)
    configure_switch(grpc_client, args.rs0, args.rs1)
    
    # exit (bug workaround)
    os.kill(os.getpid(), signal.SIGTERM)