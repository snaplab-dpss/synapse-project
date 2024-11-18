#include "packet.h"

#include <sstream>
#include <iomanip>
#include <string>

namespace netcache {

std::string mac_to_str(mac_t mac) {
	std::stringstream ss;

	ss << std::hex << std::setfill('0') << std::setw(2) << int(mac[0]);
	ss << ":";
	ss << std::hex << std::setfill('0') << std::setw(2) << int(mac[1]);
	ss << ":";
	ss << std::hex << std::setfill('0') << std::setw(2) << int(mac[2]);
	ss << ":";
	ss << std::hex << std::setfill('0') << std::setw(2) << int(mac[3]);
	ss << ":";
	ss << std::hex << std::setfill('0') << std::setw(2) << int(mac[4]);
	ss << ":";
	ss << std::hex << std::setfill('0') << std::setw(2) << int(mac[5]);

	return ss.str();
}

std::string ip_to_str(ipv4_t ip) {
	std::stringstream ss;

	ss << ((ip >> 0) & 0xff) << "." << ((ip >> 8) & 0xff) << "."
	   << ((ip >> 16) & 0xff) << "." << ((ip >> 24) & 0xff);

	return ss.str();
}

std::string port_to_str(port_t port) {
	std::stringstream ss;
	ss << ntohs(port);
	return ss.str();
}

}  // namespace netcache
