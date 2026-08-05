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
#if 0
	root=item_ptr(new item_t(*this,field.back()));

	ObjectProgress::log_generator lg(true);

	init_states();

	try
	{
		root->process_deep_common();
	}
	catch(e_cancel&)
	{
		lg<<"canceled";
        hld_thinking.reset();
        throw;
	}

	if(!root->get_wins().empty())
	{
		unsigned int depth=root->get_chain_depth()-1;
		std::string chain=print_chain(root->get_wins().get_best());
		lg<<"field_state_player_t::delegate_step(): find win chain_depth="<<depth<<": "<<chain;
	}
	if(!root->get_fails().empty() && root->get_neitrals().empty())lg<<"field_state_player_t::delegate_step(): find fail chain_depth="<<(root->get_chain_depth()-1)
		<<": "<<print_chain(root->get_fails().get_best());

	point p=*root->get_next_step();
    game().OnNextStep(*this,p);
#endif
}


} }//namespace Gomoku
