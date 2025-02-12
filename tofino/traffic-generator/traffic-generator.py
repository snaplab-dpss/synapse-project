#!/usr/bin/env python3

# import bfrt_grpc.client as gc
import argparse

GRPC_SERVER_IP     = "127.0.0.1"
GRPC_SERVER_PORT   = 50052
P4_PROGRAM_NAME    = "traffic-generator"
DEFAULT_PORT_SPEED = 100

ACTIVE_PORTS = [ p for p in range(1, 31) ]
TG_PORT = 1

CONFIGURATIONS = {
	# 30 LANs
	"nop": {
		"broadcast": [ p for p in ACTIVE_PORTS if p != TG_PORT ],
		"symmetric": [],
		"route": [],
	},

	# 29 LAN + 1 WAN
	# WAN in port 30
	"fw": {
		"broadcast": [ p for p in ACTIVE_PORTS if p not in [TG_PORT, 30] ],
		"symmetric": [ 30 ],
		"route": [],
	},

	# 29 LAN + 1 WAN
	# WAN in port 30
	"nat": {
		"broadcast": [ p for p in ACTIVE_PORTS if p not in [TG_PORT, 30] ],
		"symmetric": [ 30 ],
		"route": [],
	},

	# 29 clients + 1 server
	# Server requests coming from port 30
	# Server in port 2
	"kvs": {
		"broadcast": [ p for p in ACTIVE_PORTS if p not in [TG_PORT, 2, 30] ],
		"symmetric": [],
		"route": [
			(30, 2), # Server requests from clients
			(2, 30), # Server responses back to clients
		],
	},
}

class Ports:
	def __init__(self, bfrt_info):
		self.bfrt_info = bfrt_info
		self.dev_to_front_panel = {}

	def get_dev_port(self, front_panel_port, lane=0):        
		target = gc.Target(device_id=0, pipe_id=0xffff)
		port_hdl_info_table = self.bfrt_info.table_get("$PORT_HDL_INFO")
		
		key = port_hdl_info_table.make_key([
			gc.KeyTuple("$CONN_ID", front_panel_port),
			gc.KeyTuple("$CHNL_ID", lane)
		]) 

		# Convert front-panel port to dev port
		resp = port_hdl_info_table.entry_get(target, [key], {"from_hw": False})

		dev_port = next(resp)[0].to_dict()["$DEV_PORT"]
		self.dev_to_front_panel[dev_port] = front_panel_port

		return dev_port

	def get_front_panel_port(self, dev_port):
		if dev_port in self.dev_to_front_panel:
			return self.dev_to_front_panel[dev_port]
		
		target = gc.Target(device_id=0, pipe_id=0xffff)
		port_hdl_info_table = self.bfrt_info.table_get("$PORT_HDL_INFO")
		
		key = port_hdl_info_table.make_key([
			gc.KeyTuple("$DEV_PORT", dev_port),
			gc.KeyTuple("$CHNL_ID", 0)
		])

		# Convert dev port to front-panel port
		resp = port_hdl_info_table.entry_get(target, [key], {"from_hw": False})

		front_panel_port = next(resp)[0].to_dict()["$CONN_ID"]
		self.dev_to_front_panel[dev_port] = front_panel_port

		return front_panel_port

	def get_all_front_panel_ports(self):
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
			# This is kind of janky, but we can only query the entire table on some switches, who knows why.
			# Hence, we do it one by one.

			front_panel_ports = []
			front_panel_port = 1
			while True:
				try:
					resp = port_hdl_info_table.entry_get(
						target,
						[port_hdl_info_table.make_key([gc.KeyTuple("$CONN_ID", front_panel_port),
													gc.KeyTuple("$CHNL_ID", 0)])], {"from_hw": False})
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
		
		return front_panel_ports

	# Port list is a list of tuples: (front panel port, lane, speed, FEC string).
	# Speed is one of {10, 25, 40, 50, 100}.
	# FEC string is one of {"none", "fc", "rs"}.
	# Look in $SDE_INSTALL/share/bf_rt_shared/*.json for more info.
	def add_ports(self, port_list):
		speed_conversion_table = {
			10:  "BF_SPEED_10G",
			25:  "BF_SPEED_25G",
			40:  "BF_SPEED_40G",
			50:  "BF_SPEED_50G",
			100: "BF_SPEED_100G",
		}
		
		fec_conversion_table = {
			"none": "BF_FEC_TYP_NONE",
			"fec":  "BF_FEC_TYP_FC",
			"rs":   "BF_FEC_TYP_RS",
		}

		speed_to_fec = {
			10:  "none",
			25:  "none",
			40:  "none",
			50:  "none",
			100: "rs",
		}
		
		target = gc.Target(device_id=0, pipe_id=0xffff)
		port_table = self.bfrt_info.table_get("$PORT")

		for (front_panel_port, speed) in port_list:
			fec = speed_to_fec[speed]
			key = port_table.make_key([gc.KeyTuple("$DEV_PORT", self.get_dev_port(front_panel_port))])
			data = port_table.make_data([
				gc.DataTuple("$SPEED", str_val=speed_conversion_table[speed]),
				gc.DataTuple("$FEC", str_val=fec_conversion_table[fec]),
				gc.DataTuple("$PORT_ENABLE", bool_val=True),
			])
			
			port_table.entry_add(target, [key], [data])
			
	def add_port(self, front_panel_port, speed):
		self.add_ports([(front_panel_port, speed)])

class Router:
	def __init__(self, bfrt_info):
		self.router = bfrt_info.table_get("Ingress.router_tbl")

	def clear(self):
		target = gc.Target(device_id=0, pipe_id=0xffff)
		for _, key in self.router.entry_get(target, [], {"from_hw": False}):
			if key:
				self.router.entry_del(target, [key])
		
	def add_route_tg_entry(self, dev_port):
		target = gc.Target(device_id=0, pipe_id=0xffff)
		self.router.entry_add(
			target,
			[
				self.router.make_key([
					gc.KeyTuple("ig_intr_md.ingress_port", dev_port),
				])
			],
			[
				self.router.make_data([], "Ingress.route_tg")
			]
		)
	
	def add_route_symmetric_response(self, dev_port):
		target = gc.Target(device_id=0, pipe_id=0xffff)
		self.router.entry_add(
			target,
			[
				self.router.make_key([
					gc.KeyTuple("ig_intr_md.ingress_port", dev_port),
				])
			],
			[
				self.router.make_data([], "Ingress.route_symmetric_response")
			]
		)
	
	def add_forward_entry(self, ingress_dev_port, egress_dev_port):
		target = gc.Target(device_id=0, pipe_id=0xffff)
		self.router.entry_add(
			target,
			[
				self.router.make_key([
					gc.KeyTuple("ig_intr_md.ingress_port", ingress_dev_port),
				])
			],
			[
				self.router.make_data([
					gc.DataTuple("port", egress_dev_port),
				], "Ingress.forward")
			]
		)

class Multicaster:
	def __init__(self, bfrt_info):
		self.mgid_table = bfrt_info.table_get("$pre.mgid")
		self.node_table = bfrt_info.table_get("$pre.node")
		self.ecmp_table = bfrt_info.table_get("$pre.ecmp")
		self.lag_table = bfrt_info.table_get("$pre.lag")
		self.prune_table = bfrt_info.table_get("$pre.prune")
		self.port_table = bfrt_info.table_get("$pre.port")
	
	def setup_multicast(self, dev_ports):
		target = gc.Target(device_id=0, pipe_id=0xffff)

		mg_id = 1
		l1_id = 1
		rid = 5

		try:
			self.node_table.entry_add(
				target,
				[
					self.node_table.make_key([
						gc.KeyTuple("$MULTICAST_NODE_ID", l1_id),
					])
				],
				[
					self.node_table.make_data([
						gc.DataTuple("$MULTICAST_RID", rid),
						gc.DataTuple("$MULTICAST_LAG_ID", int_arr_val=[]),
						gc.DataTuple("$DEV_PORT", int_arr_val=dev_ports),
					])
				]
			)

			self.mgid_table.entry_add(
				target,
				[
					self.mgid_table.make_key([
						gc.KeyTuple("$MGID", mg_id),
					])
				],
				[
					self.mgid_table.make_data([
						gc.DataTuple("$MULTICAST_NODE_ID", int_arr_val=[l1_id]),
						gc.DataTuple("$MULTICAST_NODE_L1_XID_VALID", bool_arr_val=[0]),
						gc.DataTuple("$MULTICAST_NODE_L1_XID", int_arr_val=[0]),
					])
				]
			)
		except:
			self.node_table.entry_mod(
				target,
				[
					self.node_table.make_key([
						gc.KeyTuple("$MULTICAST_NODE_ID", l1_id),
					])
				],
				[
					self.node_table.make_data([
						gc.DataTuple("$MULTICAST_RID", rid),
						gc.DataTuple("$MULTICAST_LAG_ID", int_arr_val=[]),
						gc.DataTuple("$DEV_PORT", int_arr_val=dev_ports),
					])
				]
			)

class PacketModifier:
	def __init__(self, bfrt_info):
		self.packet_modifier = bfrt_info.table_get("Egress.packet_modifier_tbl")

	def clear(self):
		target = gc.Target(device_id=0, pipe_id=0xffff)
		for _, key in self.packet_modifier.entry_get(target, [], {"from_hw": False}):
			if key: self.packet_modifier.entry_del(target, [key])
	
	def set_prefix(self, dev_port, prefix):
		target = gc.Target(device_id=0, pipe_id=0xffff)
		self.packet_modifier.entry_add(
			target,
			[
				self.packet_modifier.make_key([
					gc.KeyTuple("eg_intr_md.egress_port", dev_port),
				])
			],
			[
				self.packet_modifier.make_data([
					gc.DataTuple("prefix", prefix)
				], "Egress.set_prefix")
			]
		)

def setup(bfrt_info, cfg):
	ports = Ports(bfrt_info)
	router = Router(bfrt_info)
	multicaster = Multicaster(bfrt_info)
	packet_modifier = PacketModifier(bfrt_info)

	for port in ACTIVE_PORTS:
		ports.add_port(port, DEFAULT_PORT_SPEED)
	print("Configured ports: {}".format(ACTIVE_PORTS))
	
	router.clear()
	packet_modifier.clear()
	print("Cleared the router and packet modifier tables")

	tg_dev_port = ports.get_dev_port(TG_PORT)
	router.add_route_tg_entry(tg_dev_port)
	print("Configured TG port: {}".format(TG_PORT))

	broadcast_dev_ports = [ ports.get_dev_port(p) for p in cfg["broadcast"] ]
	multicaster.setup_multicast(broadcast_dev_ports)
	print("Configured broadcasting ports: {}".format(cfg["broadcast"]))

	for port in cfg["symmetric"]:
		dev_port = ports.get_dev_port(port)
		router.add_route_symmetric_response(dev_port)
		print("Configured symmetric port: {}".format(port))
	
	for ingress_port, egress_port in cfg["route"]:
		ingress_dev_port = ports.get_dev_port(ingress_port)
		egress_dev_port = ports.get_dev_port(egress_port)
		router.add_forward_entry(ingress_dev_port, egress_dev_port)
		print("Added forwarding rule: {} -> {}".format(ingress_port, egress_port))

	for port in cfg["broadcast"] + [ egress_port for _, egress_port in cfg["route"] ]:
		dev_port = ports.get_dev_port(port)
		prefix = port
		packet_modifier.set_prefix(dev_port, prefix)
		print("Set prefix {} for egress port {} (dev={})".format(prefix, port, dev_port))

if __name__ == "__main__":
	parser = argparse.ArgumentParser(
		description=f"Controller for the {P4_PROGRAM_NAME} P4 program, responsible for managing traffic coming from a DPDK" \
					"traffic generator and broadcasting it to the DUT.",
	)

	parser.add_argument("--nf", choices=CONFIGURATIONS.keys(), required=False)
	parser.add_argument("--broadcast", type=int, nargs="+", default=[], required=False)
	parser.add_argument("--symmetric", type=int, nargs="+", default=[], required=False)
	parser.add_argument("--route", type=int, nargs=2, action="append", default=[], required=False)
	parser.add_argument("--dry-run", default=False, action="store_true")
	args = parser.parse_args()

	if args.nf is None and (not args.broadcast and not args.symmetric and not args.route):
		print("No configuration provided. Please provide either an NF or a custom routing configuration.")
		print("Use --help for more information.")
		exit(1)

	config = CONFIGURATIONS[args.nf] if args.nf else {
		"broadcast": args.broadcast,
		"symmetric": args.symmetric,
		"route": [(src,dst) for src,dst in args.route ],
	}

	print("======== Configuration ========")
	if args.nf:
		print(f"NF:     {args.nf}")
	print(f"Config: {config}")
	print("==============================")

	if args.dry_run:
		print("Dry run, not connecting to the switch.")
		exit(0)
					
	grpc_client = gc.ClientInterface("{}:{}".format(GRPC_SERVER_IP, GRPC_SERVER_PORT), 0, 0)
	grpc_client.bind_pipeline_config(P4_PROGRAM_NAME)
	bfrt_info = grpc_client.bfrt_info_get(P4_PROGRAM_NAME)

	setup(bfrt_info, config)
