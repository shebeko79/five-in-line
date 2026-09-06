#include "field_state_player.h"
#include <stdexcept>

#include "../extern/pair_comparator.h"
#include "../extern/binary_find.h"
#include "../extern/object_progress.hpp"
#include "algo_utils.h"

namespace Gomoku { namespace State5
{

unsigned common_deep = 2;
unsigned gl_threat_deep = 8;
bool prove_mode = false;


void field_state_player_t::delegate_step()
{
	node_t root = solve();
	point p=root.get_next_step();
	game().OnNextStep(*this,p);
}

node_t field_state_player_t::solve()
{
    incer_t<int> hld_thinking(thinking);
	ObjectProgress::perfomance perf;
	node_t::nodes_created=0;

	field=game().field();
	field5.set_steps(field.get_steps());

	node_t root(*this, field.back(),0,gl_threat_deep);

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

	point p=root.get_next_step();

	auto nps = node_t::nodes_created/(perf.delay()/1000000.0);
	lg<<"";
	lg << "#" << field.size() + 1 << " " << print_steps(steps_t({ {root.move_color, p} }))
		<<": time="<<perf<<" nodes="<<node_t::nodes_created<<" nps="<<nps;
	lg<<"old_scores="<<field5.get_score()<<" new_scores="<<field5.get_score(p,root.move_color);
	root.log_statistic();
	lg<<"empty_count="<<field5.get_empty_points().size();

	
	squeeze_win(root);
	squeeze_fail(root);

	return root;
}

void field_state_player_t::squeeze_win(node_t& root)
{
	const npoint* pmin_win = root.get_min_win();
	if (!pmin_win)
		return;

	npoint min_win = *pmin_win;

	ObjectProgress::log_generator lg(true);

	for (unsigned td = gl_threat_deep - 1; td >= 2; td--)
	{
		lg<<"Squeeze win: threat_deep="<<td;

		node_t dist_root(*this, field.back(),0,td);

		dist_root.process();

		const npoint* dist_win = dist_root.get_min_win();
		if(!dist_win)
			break;

		if (dist_win->n < min_win.n)
		{
			root.replace_shorter_wins(dist_root);
			min_win = *dist_win;
			lg << "Better win: " << print_points(npoints_t({ min_win}));
		}
	}
}

void field_state_player_t::squeeze_fail(node_t& root)
{
	const npoint* pmax_fail = root.get_max_fail();

	if(!pmax_fail || root.get_min_win()!= nullptr || !root.get_neutrals().empty())
		return;

	npoint max_fail = *pmax_fail;

	ObjectProgress::log_generator lg(true);

	for (unsigned td = gl_threat_deep - 1; td >= 2; td--)
	{
		lg<<"Squeeze fail: threat_deep="<<td;

		node_t dist_root(*this, field.back(),0,td);

		dist_root.process();

		const npoint* dist_fail = dist_root.get_max_fail();
		if(!dist_fail)
			break;
		
		if(dist_root.get_min_win()!= nullptr)
			throw std::runtime_error("squeeze_fail(): win found");
		
		if(!dist_root.get_neutrals().empty())
			break;

		if (dist_fail->n < max_fail.n)
		{
			root.replace_shorter_fails(dist_root);
			max_fail = *dist_fail;
			lg << "Better fail: " << print_points(npoints_t({ max_fail}));
		}
	}
}


//
// node_t
//
size_t node_t::nodes_created=0;

node_t::node_t(field_state_player_t& _player, const step_t& st, unsigned _deep, unsigned _threat_deep) :
	prev_step(st),
	deep(_deep),
	threat_deep(_threat_deep),
	move_color(other_color(prev_step.step)),
	player(_player)
{
	++nodes_created;
}

void node_t::process()
{
	points_t pts = set_to_point(player.field5.get_empty_points());

	auto& scores_field = player.field5.get_scores_field();
	max_step_pr pr(scores_field, move_color);

	sort(pts,pr);

	score_t scr;
	scr.score(move_color) = kScore5;
	scr.score(prev_step.step) = 0;
	auto rng = std::equal_range(pts.begin(),pts.end(), scr, pr);

	if (rng.first != rng.second)
	{
		wins.push_back(npoint(*rng.first,1));
		return;
	}

	scr.score(move_color) = 0;
	scr.score(prev_step.step) = kScore5;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);

	if (rng.first != rng.second)
	{
		if (rng.second - rng.first > 1)
		{
			for(;rng.first != rng.second; ++rng.first)
				fails.emplace_back(npoint(*rng.first,2));

			return;
		}

		make_move(*rng.first);
		return;
	}

	if (deep >= threat_deep)
		deep_limit_reached = true;

	scr.score(move_color) = kScore4*2;
	scr.score(prev_step.step) = 0;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);

	if (rng.first != rng.second)
	{
		wins.push_back(npoint(*rng.first,3));
		return;
	}

	scr.score(move_color) = kScore4;
	scr.score(prev_step.step) = 0;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);

	if(mark_unchecked_make_move(rng, scores_field))
		return;

	scr.score(move_color) = 0;
	scr.score(prev_step.step) = kScore4*2;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);

	points_t p4_pts(pts.begin(), rng.second);
	remove_if(p4_pts, [&scores_field,cl = prev_step.step](const point& p)
		{
			Score scr = scores_field.get(p).score(cl) % kScore5;
			return scr < kScore4*2;
		});

	limit_to_p4_fork(p4_pts);

	auto cut_rng = rng;
	process_oposite_forked(cut_rng);
	if(mark_unchecked_make_move(cut_rng, scores_field))
		return;

	scr.score(move_color) = kScore3*2;
	scr.score(prev_step.step) = 0;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);
	cut_rng = rng;
	process_oposite_forked(cut_rng);
	if(mark_unchecked_make_move(cut_rng, scores_field))
		return;

	if (deep >= common_deep || prove_mode&&move_color==st_krestik)
		deep_limit_reached = true;

	rng.first = rng.second;
	rng.second = pts.end();
	cut_rng = rng;
	process_oposite_forked(cut_rng);
	if(mark_unchecked_make_move(cut_rng, scores_field))
		return;
}

void node_t::process_oposite_forked(points_range& rng)
{
	if (!oposite_fork.is_active())
		return;

	for(auto p = rng.first; p != rng.second; ++p)
	{
		if (!oposite_fork.inside(*p))
		{
			if (deep > 0)
				forced_max_fail = 4;
			else
				fails.emplace_back(npoint(*p, 4));
		}
	}

	rng.second = std::remove_if(rng.first,rng.second,[this](const point& p){return !oposite_fork.inside(p);});
}

bool node_t::mark_unchecked_make_move(const points_range& rng, const matrix<score_t>& scores_field)
{
	if (deep_limit_reached)
	{
		for(auto p = rng.first; p != rng.second; ++p)
			neutrals.emplace_back(*p, player.field5.get_score(*p, move_color));
		
		return false;
	}

	std::sort(rng.first, rng.second, score_pr(scores_field, move_color));
	return make_move_find_win(rng);
}

bool node_t::make_move_find_win(const points_range& rng)
{
	for(auto p = rng.first; p != rng.second; ++p)
	{
		make_move(*p);
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

	node_t sub(player, new_step, deep+1, threat_deep);
	sub.process();

	auto* win = sub.get_min_win();
	if (win)
	{
		fails.emplace_back(npoint(new_step,win->n+1));
	}
	else if (!sub.neutrals.empty())
	{
		neutrals.push_back(ipoint(new_step,sub.best_neutral_score()));
	}
	else
	{
		if(auto* fail = sub.get_max_fail()) 
			wins.emplace_back(npoint(new_step,std::max(fail->n,sub.forced_max_fail)+1));
		else if(sub.forced_max_fail>0)
			wins.emplace_back(npoint(new_step,sub.forced_max_fail+1));
		else
			throw std::runtime_error("node_t::make_move(): invalid state");
	}
	
	player.field5.apply_shapshot(snapshot);
	player.field.pop(old_bound);
}

void node_t::limit_to_p4_fork(const points_t& other_p4h)
{
	if(other_p4h.empty())
		return;

	const matrix<score_t>& scores_field = player.field5.get_scores_field();

	for(const auto& p : other_p4h)
	{
		auto count4 = scores_field.get(p).score(prev_step.step)/kScore4;

		fork_t f(p);
		if(count4 == 2) 
		{
			player.field5.iterate_involved_lines(p, [&](const point& line_point, const line5_t& line, int dx, int dy)
				{
					if(line.steps != 4 || line.color != prev_step.step)
						return;

					for (int i = -2; i <= 2; i++)
					{
						point p_empty(line_point.x + i * dx, line_point.y + i * dy);
						if(player.field.at(p_empty) == st_empty)
							f.add(p_empty);
					}
				});
		}
		
		oposite_fork.merge(f);
		if(oposite_fork.is_empty_set())
			return;
	}
}



point node_t::get_next_step() const
{
	auto* win = get_min_win();
	if (win)return *win;

	if (!neutrals.empty())
		return *std::min_element(neutrals.begin(),neutrals.end(), fscore_pr(move_color) );

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

int node_t::best_neutral_score() const
{
	return std::min_element(neutrals.begin(),neutrals.end(),fscore_pr(move_color))->i;
}


void node_t::log_statistic() const
{
	ObjectProgress::log_generator lg(true);

	lg<<"Node stat:";
	
	if(!wins.empty())
	{
		npoints_t tmp = wins;
		std::stable_sort(tmp.begin(),tmp.end(), less_n_pr());
		lg<<"Wins: "<<print_points(tmp);
	}

	if (forced_max_fail > 0)
		lg<<"forced_max_fail=" <<forced_max_fail;

	if (!fails.empty())
	{
		npoints_t tmp = fails;
		std::stable_sort(tmp.begin(),tmp.end(), less_n_pr());
		std::reverse(tmp.begin(),tmp.end());
		lg<<"Fails count: "<<tmp.size();
		lg<<"Fails: "<<print_points(tmp);
	}

	lg<<"Neutrals count: "<<neutrals.size();
	if (!neutrals.empty())
	{
		ipoints_t tmp = neutrals;
		std::stable_sort(tmp.begin(),tmp.end(), fscore_pr(move_color));
		if (tmp.size() <= 10)
			lg << "Neutrals: " << print_points(tmp);
		else
		{
			tmp.resize(10);
			lg << "Neutrals: " << print_points(tmp)<<"...";
		}
	}
}

void node_t::replace_shorter_wins(const node_t& rnode)
{
	for (const npoint& p : rnode.wins)
	{
		auto it = std::find(wins.begin(), wins.end(), point(p));
		if (it == wins.end())
		{
			wins.push_back(p);

			auto nit = std::find(neutrals.begin(), neutrals.end(), point(p));
			if(nit != neutrals.end())
				neutrals.erase(nit);
		}
		else
		{
			if (it->n > p.n)
				*it = p;
		}
	}
}

void node_t::replace_shorter_fails(const node_t& rnode)
{
	for (const npoint& p : rnode.fails)
	{
		auto it = std::find(fails.begin(), fails.end(), point(p));
		if(it == fails.end())
			throw std::runtime_error("replace_shorter_fails(): longer chain doesn't exist");

		if(it->n > p.n)
			*it = p;
	}
}


} }//namespace Gomoku
