#include "node.hpp"
#include "../pch.hpp"

namespace Clone {
	Node::Node(const std::string &name, NodeType node_type): name(name), node_type(node_type), children() {}

	Node::~Node() = default;

	void Node::print() const {
		cout << "Node(" << name << ")" << endl;

		for(auto &child: this->get_children()) {
			unsigned port_src = child.first;
			unsigned port_dst = child.second.first;
			auto &child_node = child.second.second;
			cout << " - Port " << port_src << " -> " << child_node->get_name() << ":" << port_dst << std::endl;
		}
	}
}
