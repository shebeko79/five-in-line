#include "field_state_player.h"
#include <stdexcept>

#include "../extern/object_progress.hpp"
#include "algo_utils.h"

namespace Gomoku { namespace State5
{

unsigned common_deep = 2;
unsigned gl_threat_deep = 12;

field_state_player_t::field_state_player_t()
{
	ctxs = create_ctxs();
}

std::vector<Ctx> field_state_player_t::create_ctxs()
{
	std::vector<Ctx> ret;

	static const Ctx ctxs[] = {
		{ 6,  50000, st_krestik},
		{ 8,  50000, st_krestik},
		{ 10, 50000, st_krestik},
		{ 12, 50000, st_krestik},
		{ 16, 50000, st_krestik},
		{ 20, 50000, st_krestik},
		{ 22, 50000, st_krestik},

		{ 6,  50000, st_nolik},
		{ 8,  50000, st_nolik},
		{ 10, 50000, st_nolik},
		{ 12, 50000, st_nolik},
		{ 16, 50000, st_nolik},

		{ 8,  500000, st_krestik},
		{ 12, 500000, st_krestik},
		{ 16, 500000, st_krestik},


		{ 12, 1000000, st_krestik},
		{ 16, 1000000, st_krestik},
	};

	size_t ctx_count = sizeof(ctxs) / sizeof(Ctx);

	unsigned last_krestik_serious_budget_deep = 0;

	for (size_t i = 0; i < ctx_count; i++)
	{
		Ctx c = ctxs[i];
		if(c.threat_deep<=common_deep)
			continue;

		if (c.max_nodes >= 1000000 && c.prove_color == st_krestik)
		{
			if (last_krestik_serious_budget_deep < gl_threat_deep && c.threat_deep >= gl_threat_deep)
				ret.push_back(Ctx(gl_threat_deep, thread_deep_max_nodes, st_krestik, Ctx::fl_all_at_once));

			last_krestik_serious_budget_deep = c.threat_deep;

			if(c.threat_deep == gl_threat_deep)
				continue;
		}

		if(i+1 == ctx_count)
			c.flag = c.fl_process_low;

		ret.push_back(c);
	}

	return ret;
}

void field_state_player_t::delegate_step()
{
	node_t root = solve();
	point p=root.get_next_step();
	game().OnNextStep(*this,p);
}

node_t field_state_player_t::solve()
{
    incer_t<int> hld_thinking(thinking);
	ObjectProgress::perfomance total_perf;
	ObjectProgress::log_generator lg(true);

	field=game().field();
	field5.set_steps(field.get_steps());

	node_t root(*this, field.back(),0,gl_threat_deep);

	for (size_t i = 0; i < ctxs.size(); i++)
	{
		const Ctx& c = ctxs[i];

		ObjectProgress::perfomance step_perf;

		try
		{
			if (i == 0)
			{
				node_t::nodes_created = 0;
				node_t::ctx = c;
				root.threat_deep = c.threat_deep;
				root.process();
			}
			else
			{
				root.process_lookup(c);
			}
		}
		catch (e_cancel&)
		{
			lg << "canceled";
			hld_thinking.reset();
			throw;
		}

		auto nps = node_t::nodes_created / (step_perf.delay() / 1000000.0);
		
		if (root.is_complete() || i + 1 == ctxs.size())
		{
			point p = root.get_next_step();

			lg << "#" << field.size() + 1 << " " << print_steps(steps_t({ { root.move_color, p } }))
				<< ": time=" << step_perf << " nodes=" << node_t::nodes_created << " nps=" << nps
				<<" deep=" << c.threat_deep << " max_nodes=" << c.max_nodes <<" pc="<<to_string(c.prove_color);
			root.log_statistic();
		}
		else
		{
			lg << "N="<<root.get_neutrals().size()
				<<" time=" << step_perf << " nodes=" << node_t::nodes_created << " nps=" << nps
				<<" deep=" << c.threat_deep << " max_nodes=" << c.max_nodes <<" pc="<<to_string(c.prove_color);
		}

		if(root.is_complete())
			break;
	}

	lg<<"empty_count="<<field5.get_empty_points().size();
	lg<<"total_time="<<total_perf;
	lg<<"";

	return root;
}


//
// node_t
//
size_t node_t::nodes_created=0;
Ctx node_t::ctx(gl_threat_deep, thread_deep_max_nodes, st_krestik, Ctx::fl_all_at_once);

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
	process(pts);
}

void node_t::process(points_t& pts)
{
	auto& scores_field = player.field5.get_scores_field();
	max_step_pr pr(scores_field, move_color);

	sort(pts,pr);

	score_t scr;
	scr.cnt(move_color) = kCount5;
	scr.cnt(prev_step.step) = 0;
	auto rng = std::equal_range(pts.begin(),pts.end(), scr, pr);

	if (rng.first != rng.second)
	{
		wins.push_back(npoint(*rng.first,1));
		return;
	}

	scr.cnt(move_color) = 0;
	scr.cnt(prev_step.step) = kCount5;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);

	if (rng.first != rng.second)
	{
		if (rng.second - rng.first > 1)
		{
			for(;rng.first != rng.second; ++rng.first)
				fails.emplace_back(npoint(*rng.first,2));

			return;
		}
		
		if (deep >= threat_deep)
			neutrals.emplace_back(*rng.first, player.field5.get_score(*rng.first, move_color));
		else
			make_move(*rng.first);
		
		return;
	}

	if (deep >= threat_deep || 
		deep >=common_deep && nodes_created>=ctx.max_nodes)
		deep_limit_reached = true;

	scr.cnt(move_color) = kCount4*2;
	scr.cnt(prev_step.step) = 0;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);

	if (rng.first != rng.second)
	{
		auto p = find_p4_fork_that_really_wins(rng);
		if (p != rng.second)
		{
			wins.push_back(npoint(*rng.first, 3));
			return;
		}

		if(mark_unchecked_make_move(rng))
			return;
	}

	scr.cnt(move_color) = kCount4;
	scr.cnt(prev_step.step) = 0;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);

	if(mark_unchecked_make_move(rng))
		return;

	scr.cnt(move_color) = 0;
	scr.cnt(prev_step.step) = kCount4*2;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);

	//pts.begin() instead of rng.first because oposite fork move also might be higher move
	points_t p4_pts(pts.begin(), rng.second);
	remove_if(p4_pts, [&scores_field,cl = prev_step.step](const point& p)
		{
			Score scr = scores_field.get(p).cnt(cl) % kCount5;
			return scr < kCount4*2;
		});

	limit_to_p4_fork(p4_pts);

	auto cut_rng = rng;
	process_oposite_forked(cut_rng);
	if(mark_unchecked_make_move(cut_rng))
		return;

	scr.cnt(move_color) = kCount3*2;
	scr.cnt(prev_step.step) = 0;
	rng = std::equal_range(rng.second,pts.end(), scr, pr);
	cut_rng = rng;
	process_oposite_forked(cut_rng);
	if(mark_unchecked_make_move(cut_rng))
		return;

	if (deep >= common_deep)
	{
		deep_limit_reached = true;
		expect_all_fails = fails_exists() && neutrals.empty();
	}
	else if(ctx.prove_color == move_color)
		threat_deep = std::min(common_deep+2,threat_deep);

	rng.first = rng.second;
	rng.second = pts.end();
	cut_rng = rng;
	process_oposite_forked(cut_rng);
	if(mark_unchecked_make_move(cut_rng))
		return;
}

void node_t::process_lookup(Ctx _ctx)
{
	if(is_complete())
		return;

	if (_ctx.flag == _ctx.fl_all_at_once)
	{
		points_t pts(neutrals.begin(), neutrals.end());
		neutrals.clear();

		ctx = _ctx;
		threat_deep = ctx.threat_deep;
		nodes_created = 0;
		reset_intermediate_state();
		process(pts);
	}
	else
	{
		max_step_pr pr(player.field5.get_scores_field(), move_color);
		//unnecessary now because they are already in sorted order, but who know
		// anyway there is no measurable performance hit by this sort
		sort(neutrals, pr);

		score_t scr;
		scr.cnt(move_color) = kCount3*2;
		scr.cnt(prev_step.step) = 0;

		auto end_it = std::upper_bound(neutrals.begin(), neutrals.end(), scr, pr);

		points_t high_pts(neutrals.begin(), end_it);
		points_t low_pts(end_it, neutrals.end());

		if (_ctx.flag == _ctx.fl_process_low)
			neutrals.clear();
		else
			neutrals.erase(neutrals.begin(), end_it);

		if (!high_pts.empty())
		{
			ctx = _ctx;
			threat_deep = ctx.threat_deep;
			nodes_created = 0;
			reset_intermediate_state();
			process(high_pts);
		}

		if (_ctx.flag == _ctx.fl_process_low)
		{
			ctx = _ctx;
			threat_deep = ctx.threat_deep;
			nodes_created = 0;

			points_range rng(low_pts.begin(), low_pts.end());
			std::sort(rng.first, rng.second, score_pr(player.field5, move_color));
			make_move_find_win(rng);
		}
	}
}

void node_t::reset_intermediate_state()
{
	oposite_fork = fork_t();
	deep_limit_reached = false;
	expect_all_fails = false;
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

bool node_t::mark_unchecked_make_move(const points_range& rng)
{
	if (deep_limit_reached && !expect_all_fails)
	{
		for(auto p = rng.first; p != rng.second; ++p)
			neutrals.emplace_back(*p, player.field5.get_score(*p, move_color));
		
		return false;
	}

	std::sort(rng.first, rng.second, score_pr(player.field5, move_color));
	return make_move_find_win(rng);
}

bool node_t::make_move_find_win(const points_range& rng)
{
	for(auto p = rng.first; p != rng.second; ++p)
	{
		if (( expect_all_fails || ctx.prove_color == prev_step.step && deep >= common_deep ) 
			&& !neutrals.empty())
		{
			neutrals.emplace_back(*p, player.field5.get_score(*p, move_color));
			continue;
		}

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
		fork_t f(p);

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

		if(!f.is_complete())
			continue;
		
		if(f.is_out_of_bound())
			f.limit_to_move_point_only();
		
		oposite_fork.merge(f);
		if(oposite_fork.is_empty_set())
			return;
	}
}

points_t::const_iterator node_t::find_p4_fork_that_really_wins(const points_range& rng)
{
	const matrix<score_t>& scores_field = player.field5.get_scores_field();

	for(auto p = rng.first; p != rng.second; ++p)
	{
		fork_t f(*p);

		player.field5.iterate_involved_lines(*p, [&](const point& line_point, const line5_t& line, int dx, int dy)
			{
				if(line.steps != 4 || line.color != move_color)
					return;

				for (int i = -2; i <= 2; i++)
				{
					point p_empty(line_point.x + i * dx, line_point.y + i * dy);
					if(player.field.at(p_empty) == st_empty)
						f.add(p_empty);
				}
			});

		if(f.is_complete())
			return p;
	}

	return rng.second;
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
		
		ipoints_t tmpf = tmp;
		for(auto& p : tmpf)
			p.i = player.field5.get_score(p, move_color);

		if (tmp.size() <= 10)
		{
			lg << "Neutrals: " << print_points(tmp);
			lg << "Near est: " << print_points(tmpf);
		}
		else
		{
			tmp.resize(10);
			tmpf.resize(10);
			lg << "Neutrals: " << print_points(tmp)<<"...";
			lg << "Near est: " << print_points(tmpf);
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
