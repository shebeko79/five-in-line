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
node_t::node_t(field_state_player_t& _player, const step_t& st, unsigned _deep) :
	prev_step(st),
	deep(_deep),
	move_color(other_color(prev_step.step)),
	player(_player)
{
}

void node_t::process()
{
	const sorted_scores& move_srt = player.field5.get_sorted(move_color);
	const sorted_scores& other_srt = player.field5.get_sorted(prev_step.step);

	if (!move_srt.p5.empty())
	{
		wins.push_back(npoint(move_srt.p5.front(),1));
		return;
	}

	if (!other_srt.p5.empty())
	{
		if (other_srt.p5.size() > 1)
		{
			for(const point& p : other_srt.p5)
				fails.emplace_back(npoint(p,2));

			return;
		}

		if(make_move_find_win(other_srt.p5))
			return;
	}

	if(deep>=threat_deep)
		return;

	auto& scores_field = player.field5.get_scores_field();

	points_t pts = move_srt.p4;
	pts.erase(std::remove_if(pts.begin(), pts.end(), 
		[&](const point& p)
		{
			return other_srt.p5_exists(p);
		}
	), pts.end());
	sort(pts, scores_field, move_color);
	if(make_move_find_win(pts))
		return;

	pts = other_srt.p4;
	pts.erase(std::remove_if(pts.begin(), pts.end(), 
		[&](const point& p)
		{
			return move_srt.p5_exists(p) ||  move_srt.p4_exists(p);
		}
	), pts.end());
	sort(pts, scores_field, move_color);
	if(make_move_find_win(pts))
		return;

	if(deep>=common_deep)
		return;

	pts = set_to_point(move_srt.p3);
	pts.erase(std::remove_if(pts.begin(), pts.end(), 
		[&](const point& p)
		{
			return other_srt.p5_exists(p) || other_srt.p4_exists(p);
		}
	), pts.end());
	sort(pts, scores_field, move_color);
	if(make_move_find_win(pts))
		return;

	pts = set_to_point(other_srt.p3);
	pts.erase(std::remove_if(pts.begin(), pts.end(), 
		[&](const point& p)
		{
			return move_srt.p5_exists(p) ||  move_srt.p4_exists(p) || move_srt.p3_exists(p);
		}
	), pts.end());
	sort(pts, scores_field, move_color);
	if(make_move_find_win(pts))
		return;

	pts = set_to_point(move_srt.p2);
	pts.erase(std::remove_if(pts.begin(), pts.end(), 
		[&](const point& p)
		{
			return other_srt.p5_exists(p) || other_srt.p4_exists(p) || other_srt.p3_exists(p);
		}
	), pts.end());
	sort(pts, scores_field, move_color);
	if(make_move_find_win(pts))
		return;

	pts = set_to_point(other_srt.p2);
	pts.erase(std::remove_if(pts.begin(), pts.end(), 
		[&](const point& p)
		{
			return move_srt.p5_exists(p) ||  move_srt.p4_exists(p) || move_srt.p3_exists(p) || move_srt.p2_exists(p);
		}
	), pts.end());
	sort(pts, scores_field, move_color);
	if(make_move_find_win(pts))
		return;
}

bool node_t::make_move_find_win(const points_t& pts)
{
	for (const point& p : pts)
	{
		make_move(p);
		if(!wins.empty())
			return true;
	}

	return false;
}

void node_t::make_move(const point& p)
{
	step_t new_step(move_color, p.x, p.y);

	rect old_bound = player.field.get_bound();
	player.field.add(new_step,new_step.step);

	snapshot_t snapshot = player.field5.make_snapshot(new_step);
	player.field5.change_state(player.field);

	node_t sub(player, new_step, deep+1);
	sub.process();

	auto* win = sub.get_min_win();
	if (win)
	{
		fails.emplace_back(npoint(new_step,win->n+1));
	}
	else if (!sub.neitrals.empty())
	{
		neitrals.push_back(new_step);
	}
	else
	{
		auto* fail = sub.get_max_fail();
		if(!fail)throw std::runtime_error("node_t::make_move(): invalid state");
		wins.emplace_back(npoint(new_step,fail->n+1));
	}
	
	player.field5.apply_shapshot(snapshot);
	player.field.pop(old_bound);
}


const point& node_t::get_next_step() const
{
	auto* win = get_min_win();
	if (win)return *win;

	if(!neitrals.empty())return neitrals.front();

	auto* fail = get_max_fail();

	if(!fail)throw std::runtime_error("node_t::get_next_step(): invalid state");
	return *fail;
}

const npoint* node_t::get_min_win() const
{
	if(wins.empty())
		return nullptr;

	return &*std::min_element(wins.begin(), wins.end(), less_n_pr());
}

const npoint* node_t::get_max_fail() const
{
	if(fails.empty())
		return nullptr;

	return &*std::max_element(fails.begin(), fails.end(), less_n_pr());
}



} }//namespace Gomoku
