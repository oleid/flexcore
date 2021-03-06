#include <flexcore/extended/base_node.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/format.hpp>

#include <stack>

namespace fc
{
static forest_t::iterator find_self(forest_t& forest, const tree_node& node)
{
	auto node_id = node.graph_info().get_id();
	auto self = std::find_if(forest.begin(), forest.end(), [=](auto& other_uniq_ptr)
	                         {
		                         return node_id == other_uniq_ptr->graph_info().get_id();
	                         });
	assert(self != forest.end());
	return adobe::trailing_of(self);
}

static constexpr auto name_seperator = "/";

std::string full_name(forest_t& forest,
                      const tree_node& node)
{
	auto position = find_self(forest, node);
	//push names of parent / grandparent ... to stack to later reverse order.
	std::stack<std::string> name_stack;
	for (auto parent = adobe::find_parent(position); parent != forest.end();
	        parent = adobe::find_parent(parent))
	{
		name_stack.emplace((*parent)->name());
	}
	std::string full_name;
	while (!name_stack.empty())
	{
		full_name += (name_stack.top() + name_seperator);
		name_stack.pop();
	}
	full_name += (*position)->name();
	return full_name;
}

tree_base_node::tree_base_node(const node_args& args)
	: fg_(args.fg)
	, region_(args.r)
	, graph_info_(args.graph_info)
{
	assert(fg_);
	assert(region_);
}

std::string tree_base_node::name() const
{
	return graph_info_.name();
}

graph::graph_node_properties tree_base_node::graph_info() const
{
	return graph_info_;
}

graph::connection_graph& tree_base_node::get_graph()
{
	return fg_->graph;
}

forest_t::iterator owning_base_node::self() const
{
	return self_;
}

forest_t::iterator owning_base_node::add_child(std::unique_ptr<tree_node> child)
{
	assert(fg_);
	assert(child);
	auto& forest = fg_->forest;
	auto child_it = adobe::trailing_of(forest.insert(self(), std::move(child)));
	assert(adobe::find_parent(child_it) == self());
	assert(adobe::find_parent(child_it) != forest.end());
	return child_it;
}

std::shared_ptr<parallel_region> owner_holder::region()
{
	assert(owner_);
	return owner_->region();
}
graph::graph_node_properties owner_holder::graph_info() const
{
	assert(owner_);
	return owner_->graph_info();
}
graph::connection_graph& owner_holder::get_graph()
{
	assert(owner_);
	return owner_->get_graph();
}

std::string owner_holder::name() const
{
	assert(owner_);
	return owner_->name();
}

forest_owner::forest_owner(graph::connection_graph& graph, std::string n,
                           std::shared_ptr<parallel_region> r)
    : fg_(std::make_unique<forest_graph>(graph)), tree_root(nullptr)
{
	assert(fg_);
	auto& forest = fg_->forest;
	auto iter = adobe::trailing_of(forest.insert(forest.begin(), std::make_unique<owner_holder>()));
	auto& holder = static_cast<owner_holder&>(*iter->get());
	tree_root = &holder.set_owner(std::make_unique<owning_base_node>(iter, node_args{fg_.get(), r, n}));
	assert(tree_root);
}

void forest_owner::print_forest(std::ostream& out) const
{
	auto id = [](auto& node)
	{
		return hash_value(node->graph_info().get_id());
	};
	out << "{\n";
	auto range = adobe::preorder_range(fg_->forest);
	std::vector<std::string> lines;
	for (auto node = range.first, last = range.second; node != last; ++node)
	{
		std::vector<std::string> parents;
		// first entry in the parents array is the name of the node
		parents.emplace_back(str(boost::format("\"%s\"") % (*node)->name()));
		for (auto parent = adobe::find_parent(node.base()); parent != fg_->forest.end();
		     parent = adobe::find_parent(parent))
		{
			parents.emplace_back((boost::format("\"0x%x\"") % id(*parent)).str());
		}
		auto parents_str = boost::algorithm::join(parents, ",");
		lines.emplace_back((boost::format("\t\"0x%x\": [%s]") % id(*node) % parents_str).str());
	}
	out << boost::algorithm::join(lines, ",\n");
	out << "\n}\n";
}
}
