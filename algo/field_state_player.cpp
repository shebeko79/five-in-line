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
	ObjectProgress::perfomance perf;
	node_t::nodes_created=0;

	field=game().field();
	field5.set_steps(field.get_steps());

	node_t root(*this, field.back());

	ObjectProgress::log_generator lg(true);

	try
	{
		field5.is_update_empty_fields = false;
		root.process();
		field5.is_update_empty_fields = true;
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
	lg<<"empty_count="<<(field5.get_sorted(st_krestik).size() + field5.get_sorted(st_krestik).size());
	lg<<"Sorted krestik:";
	field5.get_sorted(st_krestik).log_statistic();
	lg<<"Sorted nolik:";
	field5.get_sorted(st_nolik).log_statistic();


	game().OnNextStep(*this,p);
}

//
// node_t
//
size_t node_t::nodes_created=0;

node_t::node_t(field_state_player_t& _player, const step_t& st, unsigned _deep) :
	prev_step(st),
	deep(_deep),
	move_color(other_color(prev_step.step)),
	player(_player)
{
	++nodes_created;
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

		make_move(other_srt.p5.front());
		return;
	}

	if (deep >= threat_deep)
		deep_limit_reached = true;

	auto& scores_field = player.field5.get_scores_field();

	if (!move_srt.p4h.empty())
	{
		wins.push_back(npoint(move_srt.p4h.front(),3));
		return;
	}

	points_t pts = move_srt.p4l;
	remove_if(pts, other_srt.p5_pr());
	if(mark_unchecked_make_move(pts, scores_field))
		return;

	limit_to_p4_fork(other_srt);

	pts = other_srt.p4h;
	remove_if(pts, move_srt.p4_pr());
	process_oposite_forked(pts);
	if(mark_unchecked_make_move(pts, scores_field))
		return;

	pts = set_to_point(move_srt.p3h);
	remove_if(pts, other_srt.p4h_pr());
	process_oposite_forked(pts);
	if(mark_unchecked_make_move(pts, scores_field))
		return;

	if (deep >= common_deep)
		deep_limit_reached = true;

	pts = other_srt.p4l;
	remove_if(pts, move_srt.p3h_pr());
	process_oposite_forked(pts);
	if(mark_unchecked_make_move(pts, scores_field))
		return;

	pts = set_to_point(move_srt.p3l);
	remove_if(pts, other_srt.p4_pr());
	process_oposite_forked(pts);
	if(mark_unchecked_make_move(pts, scores_field))
		return;

	pts = set_to_point(other_srt.p3h);
	remove_if(pts, move_srt.p3_pr());
	process_oposite_forked(pts);
	if(mark_unchecked_make_move(pts, scores_field))
		return;

	pts = set_to_point(other_srt.p3l);
	remove_if(pts, move_srt.p3_pr());
	process_oposite_forked(pts);
	if(mark_unchecked_make_move(pts, scores_field))
		return;

	pts = set_to_point(move_srt.p2);
	remove_if(pts, other_srt.p3_pr());
	process_oposite_forked(pts);
	if(mark_unchecked_make_move(pts, scores_field))
		return;

	pts = set_to_point(other_srt.p2);
	remove_if(pts, move_srt.p2_pr());
	process_oposite_forked(pts);
	if(mark_unchecked_make_move(pts, scores_field))
		return;
}

void node_t::process_oposite_forked(points_t& pts)
{
	if (!oposite_fork.is_active())
		return;

	for (const point& p : pts)
	{
		if (!oposite_fork.inside(p))
		{
			if (deep > 0)
				forced_max_fail = 4;
			else
				fails.emplace_back(npoint(p, 4));
		}
	}

	pts.erase(std::remove_if(pts.begin(),pts.end(),[this](const point& p){return !oposite_fork.inside(p);}), pts.end());
}

bool node_t::mark_unchecked_make_move(points_t& pts, const matrix<score_t>& scores_field)
{
	if (deep_limit_reached)
	{
		for(const point& p : pts)
			neutrals.emplace_back(p, player.field5.get_score(p, move_color));
		
		return false;
	}

	sort(pts, score_pr(scores_field, move_color));
	return make_move_find_win(pts);
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

void node_t::limit_to_p4_fork(const sorted_scores& other_srt)
{
	if(other_srt.p4h.empty())
		return;

	const matrix<score_t>& scores_field = player.field5.get_scores_field();

	for (const point& p : other_srt.p4h)
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

} }//namespace Gomoku
