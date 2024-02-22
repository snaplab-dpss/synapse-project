#!/usr/bin/env python3

import argparse
import asyncio
import json

from pathlib import Path
from random import shuffle

from scapy.utils import PcapWriter
from scapy.all import (
    Ether,
	IP,
	UDP,
	bytes_encode,
	DLT_EN10MB
)

from rich.progress import Progress

from .pcap_utils import (
	create_n_unique_flows,
	get_flow_id,
	random_mac,
	DEFAULT_PCAP_DIR,
)

EPOCHS_IN_EXP_TIME = 1
MIN_EPOCHS         = 4 * EPOCHS_IN_EXP_TIME
MIN_PKT_SIZE_BYTES = 64
MAX_PKT_SIZE_BYTES = 1514
MAX_RATE_GBITPS    = 100

def get_pkts_in_time(t_sec, pkt_sz_bytes, rate_gbps):
	IPG      = 20
	CRC      = 4
	rate_bps = rate_gbps * 1e9
	pkts     = int(rate_bps * t_sec / ((pkt_sz_bytes + IPG + CRC) * 8))

	if pkts == 0:
		print("Epochs with 0 packets. Possible solutions:")
		print("  1. Increasing the rate")
		print("  2. Increasing the expiration time")
		print("  3. Reducing the packet size")
		exit(1)

	return pkts

def get_epoch_time(exp_time_sec):
	t_sec = exp_time_sec / EPOCHS_IN_EXP_TIME
	assert t_sec > 0
	return t_sec

def get_pkts_in_epoch(exp_time_sec, pkt_sz_bytes, rate_gbps):
	epoch_time_sec = get_epoch_time(exp_time_sec)
	epoch_pkts     = get_pkts_in_time(epoch_time_sec, pkt_sz_bytes, rate_gbps)
	return epoch_pkts

def churn_from_modified_flows(foreground_flows, epochs, epoch_time_sec):
	churn_fps  = foreground_flows / (epochs * epoch_time_sec)
	churn_fpm  = 60 * churn_fps
	return int(churn_fpm)

def print_report(data):
	s = ''
	s += f"Min churn         {data['min_churn']:,} fpm\n"
	s += f"Max churn         {data['max_churn']:,} fpm\n"
	s += f"Churn             {data['churn']:,} fpm\n"
	s += f"Packet size       {data['pkt_sz']} bytes\n"
	s += f"Epochs            {data['epochs']:,}\n"
	s += f"Packets expired   {data['exp_pkts']:,}\n"
	s += f"Packets per epoch {data['pkts_epochs']:,}\n"
	s += f"Min rate          {data['min_rate']:.2f} Gbps\n"
	s += f"Target rate       {data['target_rate']:.2f} Gbps\n"
	s += f"Pcap size         {data['pcap_sz']:,} bytes\n"
	s += f"Background flows  {data['background_flows']:,}\n"
	s += f"Foreground flows  {data['foreground_flows']:,}\n"
	s += f"Total flows       {data['total_flows']:,}\n"

	print(s)

def save_report(data, report_filename):
	with open(report_filename, 'w') as f:
		json.dump(data, f, indent = 4)

def get_required_number_of_epochs(exp_time_sec, churn_fpm, pkt_sz_bytes, rate_gbps):
	exp_tx_pkts    = get_pkts_in_time(exp_time_sec, pkt_sz_bytes, rate_gbps)
	epoch_time_sec = get_epoch_time(exp_time_sec)
	epoch_pkts     = get_pkts_in_epoch(exp_time_sec, pkt_sz_bytes, rate_gbps)
	
	epochs         = MIN_EPOCHS
	min_churn_fpm  = churn_from_modified_flows(1, epochs, epoch_time_sec)
	max_churn_fpm  = churn_from_modified_flows(epoch_pkts, epochs, epoch_time_sec)

	if max_churn_fpm < churn_fpm:
		print(f'Max churn: {max_churn_fpm:,} fpm')
		print(f'Requested: {churn_fpm:,} fpm')
		exit(1)

	while churn_fpm > 0 and not min_churn_fpm <= churn_fpm <= max_churn_fpm:
		epochs       += 2
		min_churn_fpm = churn_from_modified_flows(1, epochs, epoch_time_sec)
		max_churn_fpm = churn_from_modified_flows(epoch_pkts, epochs, epoch_time_sec)

		assert max_churn_fpm >= min_churn_fpm

	report_data = {
		'min_churn':   min_churn_fpm,
		'max_churn':   max_churn_fpm,
		'pkt_sz':      pkt_sz_bytes,
		'epochs':      epochs,
		'exp_pkts':    exp_tx_pkts,
		'pkts_epochs': epoch_pkts,
		'target_rate': rate_gbps,
		'pcap_sz':     epochs * epoch_pkts * pkt_sz_bytes,
	}

	return epochs, report_data

def get_real_churn(churn_fpm, epochs, exp_time_sec, n_epoch_pkts, total_flows, report):
	epoch_time_sec = get_epoch_time(exp_time_sec)

	assert epochs % 2 == 0

	n_modified_flows  = 0
	current_churn_fpm = 0

	while current_churn_fpm < churn_fpm:
		n_modified_flows += 1
		current_churn_fpm = churn_from_modified_flows(n_modified_flows, epochs, epoch_time_sec)

	assert n_modified_flows > 0 or churn_fpm == 0
	assert n_modified_flows <= n_epoch_pkts

	if total_flows == -1:
		total_flows = n_modified_flows

	if not (n_modified_flows <= total_flows <= n_epoch_pkts):
		print(f'Error: not enough flows')
		print(f'Requested {total_flows} total flows')
		print(f'Epoch flows: {n_epoch_pkts}')
		print(f'Foreground flows: {n_modified_flows}')
		print(f'Failed condition: foreground <= total <= epoch packets')
		exit(1)

	n_background_flows = total_flows - n_modified_flows
	min_rate_gbps      = 1e-9 * n_modified_flows * (MIN_PKT_SIZE_BYTES*8) / exp_time_sec

	report['churn']            = current_churn_fpm
	report['min_rate']         = min_rate_gbps
	report['background_flows'] = n_background_flows
	report['foreground_flows'] = n_modified_flows
	report['total_flows']      = total_flows
	
	return current_churn_fpm

def get_epochs_flows(epochs, n_epoch_pkts, foreground_flows, n_background_flows, private_only, internet_only):
	n_padding_flows  = n_epoch_pkts - foreground_flows - n_background_flows
	old_flows        = create_n_unique_flows(foreground_flows, private_only, internet_only)
	new_flows        = create_n_unique_flows(foreground_flows, private_only, internet_only, old_flows)
	background_flows = create_n_unique_flows(n_background_flows, private_only, internet_only, old_flows + new_flows)
	padding_flows    = [ f for f in old_flows for _ in range(int(n_padding_flows/foreground_flows) + 1) ]
	padding_flows    = padding_flows[:n_padding_flows]
	translation      = {}

	shuffle(padding_flows)

	assert len(old_flows) + len(background_flows) + len(padding_flows) == n_epoch_pkts

	for old_flow, new_flow in zip(old_flows, new_flows):
		translation[get_flow_id(old_flow)] = new_flow

	# list() for doing a deep copy
	epochs_flows = [ list(old_flows + background_flows + padding_flows) for _ in range(epochs) ]
	
	for epoch in range(int(epochs/2), epochs, 1):
		flows = epochs_flows[epoch]
		for i, flow in enumerate(flows):
			flow_id = get_flow_id(flow)
			if flow_id in translation:
				epochs_flows[epoch][i] = translation[flow_id]
	
	return epochs_flows

def get_churn_config(
	expiration_us,
	rate_Gbps,
	churn_fpm,
	pkt_size_bytes=MIN_PKT_SIZE_BYTES,
	total_flows=-1,
):
	exp_time_sec   = expiration_us * 1e-6
	epoch_pkts     = get_pkts_in_epoch(exp_time_sec, pkt_size_bytes, rate_Gbps)
	epochs, report = get_required_number_of_epochs(exp_time_sec, churn_fpm, pkt_size_bytes, rate_Gbps)
	
	get_real_churn(
		churn_fpm,
		epochs,
		exp_time_sec,
		epoch_pkts,
		total_flows,
		report
	)

	report["pkt_size_bytes"] = pkt_size_bytes

	return report

async def generate_pkts(config, pcap_name: Path):
	churn_fpm      = config["churn"]
	epochs_flows   = config["epochs_flows"]
	pkt_size_bytes = config["pkt_size_bytes"]
	total_pkts     = sum([ len(ef) for ef in epochs_flows ])
	encoded        = {}
	
	src_mac       = random_mac()
	dst_mac       = random_mac()

	with Progress() as progress:
		task_id = progress.add_task(f"[cyan]Generating {churn_fpm}fpm churn pcap", total=total_pkts)

		# Bypassing scapy's awfully slow wrpcap, have to use raw packets as input
		# To get a raw packet from a scapy packet use `bytes_encode(pkt)`.
		with PcapWriter(str(pcap_name), linktype=DLT_EN10MB) as pkt_wr:
			for epoch_flows in epochs_flows:
				for flow in epoch_flows:
					flow_id = get_flow_id(flow)

					if flow_id in encoded:
						raw_pkt = encoded[flow_id]
					else:
						pkt = Ether(src=src_mac, dst=dst_mac)
						pkt = pkt/IP(src=flow["src_ip"], dst=flow["dst_ip"])
						pkt = pkt/UDP(sport=flow["src_port"], dport=flow["dst_port"])

						crc_size      = 4
						overhead      = len(pkt) + crc_size
						payload_size  = pkt_size_bytes - overhead
						payload       = "\x00" * payload_size
						pkt          /= payload

						raw_pkt          = bytes_encode(pkt)
						encoded[flow_id] = raw_pkt 

					if not pkt_wr.header_present:
						pkt_wr._write_header(raw_pkt)
					pkt_wr._write_packet(raw_pkt)
					
					progress.update(task_id, advance=1)

			progress.update(task_id, description="[bold green]done!")

	return pcap_name

def get_churn_from_pcap(pcap, observed_rate_gbps):
	properties = pcap.split("_")
		
	assert len(properties) == 5

	fpm = properties[1]
	rate = properties[4]

	assert "fpm" in fpm
	assert "Gbps" in rate

	fpm_value = int(fpm.split("fpm")[0])
	rate_value = int(rate.split("Gbps")[0])

	return fpm_value * (observed_rate_gbps / rate_value)

async def generate_churn_pcap(
	churn_fpm,
	expiration_us,
	pkt_size_bytes=MIN_PKT_SIZE_BYTES,
	rate_Gbps=MAX_RATE_GBITPS,
	total_flows=-1,
	private_only=False,
	internet_only=False,
	out_dir=DEFAULT_PCAP_DIR,
):
	report = get_churn_config(
		expiration_us,
		rate_Gbps,
		churn_fpm,
		pkt_size_bytes,
		total_flows,
	)

	churn  = report["churn"]

	rate = int(rate_Gbps) if int(rate_Gbps) == rate_Gbps else str(rate_Gbps).replace('.','')
	name = f'churn_{churn}fpm_{pkt_size_bytes}B_{expiration_us}us_{rate}Gbps'

	output_fname = out_dir / Path(f'{name}.pcap')
	report_fname = out_dir / Path(f'{name}.json')

	if output_fname.exists():
		pcap_report = Path(out_dir) / Path(f"{output_fname.stem}.json")

		if pcap_report.exists():

			with open(pcap_report, 'r') as r:
				report = json.load(r)
				return output_fname, report
		
	save_report(report, report_fname)

	report["epochs_flows"] = get_epochs_flows(
		report["epochs"],
		report["pkts_epochs"],
		report["foreground_flows"],
		report["background_flows"],
		private_only,
		internet_only
	)

	await generate_pkts(report, output_fname)

	return output_fname, report

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Generate a pcap with uniform traffic.\n')

	parser.add_argument('--expiration',
		type=int,
		required=True,
		help='expiration time in us (>= 1us)')

	parser.add_argument('--churn',
		type=int,
		required=True,
		help='churn in fpm (>= 1)')
	
	parser.add_argument('--rate',
		type=float,
		required=True,
		default=100,
		help='rate in Gbps')

	parser.add_argument('--size',
		type=int,
		required=True,
		help=f'packet size ([{MIN_PKT_SIZE_BYTES},{MAX_PKT_SIZE_BYTES}])')
	
	parser.add_argument('--total-flows',
		type=int,
		default=-1,
		help=f'total number of flows (background + foreground)')

	parser.add_argument('--private-only',
		action='store_true',
		required=False,
		help='generate only flows on private networks')

	parser.add_argument('--internet-only',
		action='store_true',
		required=False,
		help='generate Internet only IPs')
	
	parser.add_argument('--out-dir',
		help='directory where the pcap will be placed',
		type=str,
		required=False,
		default=str(DEFAULT_PCAP_DIR))

	args = parser.parse_args()

	assert args.size >= MIN_PKT_SIZE_BYTES and args.size <= MAX_PKT_SIZE_BYTES
	assert args.expiration >= 1
	assert args.churn >= 0

	report = get_churn_config(
		args.expiration,
		args.rate,
		args.churn,
		args.size,
		args.total_flows,
	)

	rate = int(args.rate) if int(args.rate) == args.rate else str(args.rate).replace('.','')
	name = f'churn_{report["churn"]}fpm_{args.size}B_{args.expiration}us_{rate}Gbps'

	output_fname = Path(args.out_dir) / Path(f'{name}.pcap')
	report_fname = Path(args.out_dir) / Path(f'{name}.json')

	print(f"Out:    {output_fname}")
	print(f"Report: {report_fname}")
	print()

	print_report(report)

	report["epochs_flows"] = get_epochs_flows(
		report["epochs"],
		report["pkts_epochs"],
		report["foreground_flows"],
		report["background_flows"],
		args.private_only,
		args.internet_only
	)

	asyncio.run(generate_pkts(report, output_fname))

	save_report(report, report_fname)