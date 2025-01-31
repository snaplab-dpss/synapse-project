#!/usr/bin/env python3

import bfrt_grpc.client as gc

GRPC_SERVER_IP     = "127.0.0.1"
GRPC_SERVER_PORT   = 50052
P4_PROGRAM_NAME    = "pktgen"
DEFAULT_PORT_SPEED = 100

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
			
			print("Adding port {}".format((front_panel_port, speed, fec)))
			port_table.entry_add(target, [key], [data])
			
	def add_port(self, front_panel_port, speed):
		self.add_ports([(front_panel_port, speed)])
			
	def get_available_ports(self):
		port_table = self.bfrt_info.table_get("$PORT")
		target = gc.Target(device_id=0, pipe_id=0xffff)
		keys = port_table.entry_get(target, [], {"from_hw": False})

		ports = []

		for data, key in keys:
			key_dict = key.to_dict()
			data_dict = data.to_dict()

			port = {
				"port": data_dict["$CONN_ID"],
				"valid": data_dict["$IS_VALID"],
				"enabled": data_dict["$PORT_ENABLE"],
				"up": data_dict["$PORT_UP"],
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
				"port": data_dict["$CONN_ID"],
				"valid": data_dict["$IS_VALID"],
				"enabled": data_dict["$PORT_ENABLE"],
				"up": data_dict["$PORT_UP"],
			}

			if data_dict["$PORT_ENABLE"]:   
				ports.append(port["port"])

		return ports

class Router:
	def __init__(self, bfrt_info):
		self.router = bfrt_info.table_get("Ingress.router_tbl")

		# Clear all entries first
		target = gc.Target(device_id=0, pipe_id=0xffff)
		for _, key in self.router.entry_get(target, [], {"from_hw": False}):
			if key: self.router.entry_del(target, [key])
		
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

		# Clear all entries first
		target = gc.Target(device_id=0, pipe_id=0xffff)
		for _, key in self.packet_modifier.entry_get(target, [], {"from_hw": False}):
			if key: self.packet_modifier.entry_del(target, [key])
	
	def pick_salt_for_egress_port(self, dev_port, salt_number):
		assert salt_number in range(0, 32)
		target = gc.Target(device_id=0, pipe_id=0xffff)
		self.packet_modifier.entry_add(
			target,
			[
				self.packet_modifier.make_key([
					gc.KeyTuple("eg_intr_md.egress_port", dev_port),
				])
			],
			[
				self.packet_modifier.make_data([], "Egress.pick_salt{}".format(salt_number))
			]
		)

def setup_kvs(bfrt_info, tg_port, lan_ports):
	assert tg_port not in lan_ports

	ports = Ports(bfrt_info)
	router = Router(bfrt_info)
	multicaster = Multicaster(bfrt_info)
	packet_modifier = PacketModifier(bfrt_info)

	all_ports = [tg_port] + lan_ports
	for port in all_ports:
		ports.add_port(port, DEFAULT_PORT_SPEED)
		print("Configured port {}".format(port))

	tg_dev_port = ports.get_dev_port(tg_port)
	lan_dev_ports = [ ports.get_dev_port(p) for p in lan_ports if p != tg_port ]

	router.add_route_tg_entry(tg_dev_port)
	multicaster.setup_multicast(lan_dev_ports)
	for i, lan_dev_port in enumerate(lan_dev_ports):
		packet_modifier.pick_salt_for_egress_port(lan_dev_port, i)
		print("Picked salt {} for egress port {} (dev={})".format(
			i, ports.get_front_panel_port(lan_dev_port), lan_dev_port))

def setup_fw(bfrt_info, tg_port, lan_ports, wan_ports):
	assert tg_port not in lan_ports
	assert tg_port not in wan_ports
	assert set(lan_ports).isdisjoint(wan_ports)

	ports = Ports(bfrt_info)
	router = Router(bfrt_info)
	multicaster = Multicaster(bfrt_info)
	packet_modifier = PacketModifier(bfrt_info)

	all_ports = [tg_port] + lan_ports + wan_ports
	for port in all_ports:
		ports.add_port(port, DEFAULT_PORT_SPEED)
		print("Configured port {}".format(port))

	tg_dev_port = ports.get_dev_port(tg_port)
	lan_dev_ports = [ ports.get_dev_port(p) for p in lan_ports ]
	wan_dev_ports = [ ports.get_dev_port(p) for p in wan_ports ]

	router.add_route_tg_entry(tg_dev_port)
	for i, lan_dev_port in enumerate(lan_dev_ports):
		packet_modifier.pick_salt_for_egress_port(lan_dev_port, i)
		print("Picked salt {} for egress port {} (dev={})".format(
			i, ports.get_front_panel_port(lan_dev_port), lan_dev_port))
	
	for wan_dev_port in wan_dev_ports:
		router.add_route_symmetric_response(wan_dev_port)
		print("Configured symmetric response on wan port {} (dev={})".format(
			ports.get_front_panel_port(wan_dev_port), wan_dev_port))

	multicaster.setup_multicast(lan_dev_ports)

if __name__ == "__main__":
	grpc_client = gc.ClientInterface("{}:{}".format(GRPC_SERVER_IP, GRPC_SERVER_PORT), 0, 0)
	grpc_client.bind_pipeline_config(P4_PROGRAM_NAME)
	bfrt_info = grpc_client.bfrt_info_get(P4_PROGRAM_NAME)
	
	tg_port = 2
	lan_ports = [ p for p in range(1, 33) if p != tg_port ]
	setup_kvs(bfrt_info, tg_port, lan_ports)

	# tg_port = 2
	# lan_ports = [ 4, 6, 8 ]
	# wan_ports = [ 5, 7, 9 ]
	# setup_fw(bfrt_info, tg_port, lan_ports, wan_ports)
