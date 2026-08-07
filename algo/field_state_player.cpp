#include "field_state_player.h"
#include <stdexcept>

#include "../extern/pair_comparator.h"
#include "../extern/binary_find.h"
#include "../extern/object_progress.hpp"
#include "algo_utils.h"

namespace Gomoku { namespace State5
{


void field_state_player_t::delegate_step()
{
    incer_t<int> hld_thinking(thinking);

	field=game().field();
	field5.set_steps(field.get_steps());

	node_t root(*this, field.back());

	ObjectProgress::log_generator lg(true);

	try
	{
		root.process();
	}
	catch(e_cancel&)
	{
		lg<<"canceled";
        hld_thinking.reset();
        throw;
	}

	if(!root.get_wins().empty())
	{
		//unsigned int depth=root->get_chain_depth()-1;
		//std::string chain=print_chain(root->get_wins().get_best());
		//lg<<"field_state_player_t::delegate_step(): find win chain_depth="<<depth<<": "<<chain;
		lg<<"field_state_player_t::delegate_step(): win";
	}
	else if (!root.get_fails().empty() && root.get_neitrals().empty())
	{
		lg << "field_state_player_t::delegate_step(): fail";

		//lg << "field_state_player_t::delegate_step(): find fail chain_depth=" << (root->get_chain_depth() - 1)
		//	<< ": " << print_chain(root->get_fails().get_best());
	}

	point p=root.get_next_step();
	game().OnNextStep(*this,p);
}

//
// node_t
//
node_t::node_t(field_state_player_t& _player, const step_t& st) :
	step_t(st),
	player(_player)
{
}

void node_t::process()
{
}

point node_t::get_next_step() const
{
	if(!wins.empty())return wins.front();
	if(!neitrals.empty())return neitrals.front();
	if(fails.empty())throw std::runtime_error("node_t::get_next_step(): invalid state");
	return fails.front();
}


} }//namespace Gomoku
