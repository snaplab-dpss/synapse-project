#include "parser.hpp"
#include "../pch.hpp"
#include "../util/logger.hpp"

#include "../models/device.hpp"
#include "../models/nf.hpp"
#include "../models/link.hpp"
#include "../models/port.hpp"
#include "../network/network.hpp"


namespace Clone {
	ifstream open_file(const string &path) {
		ifstream fstream;
		fstream.open(path);

		if(!fstream.is_open()) {
			danger("Could not open input file ",  path);
		}

		return fstream;
	}

	DevicePtr parse_device(const vector<string> &words) {
		if(words.size() != LENGTH_DEVICE_INPUT) {
			danger("Invalid device at line ");
			exit(1);
		}

		const string id = words[1];

		return DevicePtr(new Device(id));
	}

	NFPtr parse_nf(const vector<string> &words) {
		if(words.size() != LENGTH_NF_INPUT) {
			danger("Invalid network function at line ");
			exit(1);
		}

		const string id = words[1];
		const string path = words[2];

		return NFPtr(new NF(id, path));;
	}

	LinkPtr parse_link(const vector<string> &words, const DeviceMap &devices, const NFMap &nfs) {
		if(words.size() != LENGTH_LINK_INPUT) {
			danger("Invalid link at line: ");
			exit(1);
		}

		const string node1 = words[1];
		const string sport1 = words[2];
		const unsigned port1 = stoul(sport1);

		if (nfs.find(node1) == nfs.end() && node1 != "port") { 
			danger("Could not find node " + node1 + " at line: ");
			exit(1);
		}

		const string node2 = words[3];
		const string sport2 = words[4];
		const unsigned port2 = stoul(sport2);

		if (nfs.find(node2) == nfs.end() && node2 != "port") { 
			danger("Could not find node " + node2 + " at line: ");
			exit(1);
		}

		return LinkPtr(new Link(node1, port1, node2, port2));
	}

	PortPtr parse_port(const vector<string> &words, DeviceMap &devices) {
		if(words.size() != LENGTH_PORT_INPUT) {
			danger("Invalid port at line: ");
			exit(1);
		}

		const unsigned global_port = stoul(words[1]);
		const string device_name = words[2];
		const unsigned device_port = stoul(words[3]);

		if (devices.find(device_name) == devices.end()) { 
			danger("Could not find device " + device_name + " at line: ");
			exit(1);
		}

		auto device = devices.at(device_name);
		device->add_port(device_port, global_port);

		return PortPtr(new Port(device, device_port, global_port));
	}

	unique_ptr<Network> parse(const string &network_file) {
		ifstream fstream = open_file(network_file);

		NFMap nfs;
		BDDs bdds;
		LinkList links;
		DeviceMap devices;
		PortMap ports;
		
		string line;
		
		while(getline(fstream, line)) {
			stringstream ss(line);
			vector<string> words;
			string word;

			while(ss >> word) {
				words.push_back(word);
			}

			if(words.size() == 0) {
				continue;
			}

			const string type = words[0];

			if(type == STRING_DEVICE) {
				auto device = parse_device(words);
				devices[device->get_id()] = move(device);
			} 
			else if(type == STRING_NF) {
				auto nf = parse_nf(words);	
				nfs[nf->get_id()] = move(nf);
			} 
			else if(type == STRING_LINK) {
				auto link = parse_link(words, devices, nfs);
				links.push_back(move(link));
			}
			else if(type == STRING_PORT) {
				auto port = parse_port(words, devices);
				ports[port->get_global_port()] = move(port);
			}
			else {
				danger("Invalid line: ", line);
			}
		}

		return Network::create(move(devices), move(nfs), move(links), move(ports));
	}
}
