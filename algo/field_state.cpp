#include <stdio.h>
#include <stdexcept>
#include "field_state.h"
#include "../extern/object_progress.hpp"


namespace Gomoku { namespace State5
{
	bool line5_t::adjust(Step new_color)
	{
		if (steps == 0)
			return false;

		if (color == st_empty)
		{
			color = new_color;
			++steps;
		}
		else if (color == new_color)
			++steps;
		else
			steps = 0;

		return true;
	}

	Score line5_t::next_cnt(Step for_color) const
	{
		if (steps == 0)
			return 0;

		if(color != st_empty && color != for_color)
			return 0;

		switch (steps)
		{
		case 2:
			return kCount2;
		case 3:
			return kCount3;
		case 4:
			return kCount4;
		case 5:
			return kCount5;
		}
		
		return 1;
	}

	Score line5_t::cur_cnt(Step for_color) const
	{
		if (steps == 0)
			return 0;

		if(color != st_empty && color != for_color)
			return 0;

		switch (steps)
		{
		case 2:
			return 1;
		case 3:
			return kCount2;
		case 4:
			return kCount3;
		case 5:
			return kCount4;
		}
		
		return 0;
	}

	line5_t& lines5_t::get_line(int dx, int dy)
	{
		if(dy == 0) return h;
		if(dx == 0) return v;
		if(dy == 1) return tb;
		return bt;
	}

	unsigned max_step(Score score)
	{
		if(score == 0)
			return 0;

		if(score>=kCount5)
			return 5;

		if(score>=kCount4)
			return 4;

		if(score>=kCount3)
			return 3;

		if(score>=kCount2)
			return 2;

		return 1;
	}

	Score prev_cnt(Score score)
	{
		Score ret=0;

		if (score >= kCount5)
		{
			ret += score / kCount5 * kCount4;
			score %= kCount5;
		}

		if (score >= kCount4)
		{
			ret += score / kCount4 * kCount3;
			score %= kCount4;
		}

		if (score >= kCount3)
		{
			ret += score / kCount3 * kCount2;
			score %= kCount3;
		}

		if (score >= kCount2)
			ret += score / kCount2;

		return ret;
	}

	void snapshot_t::fill(const matrix<lines5_t>& lines_field)
	{
		fill(lines_field,hl,1,0);
		fill(lines_field,vl,0,1);
		fill(lines_field,tbl,1,1);
		fill(lines_field,btl,1,-1);
	}

	void snapshot_t::apply(matrix<lines5_t>& lines_field, const score_set& scr_set) const
	{
		apply(lines_field);
		apply(scr_set);
		scr_set(st, central_s);
	}

	void snapshot_t::apply(matrix<lines5_t>& lines_field) const
	{
		apply(lines_field,hl,1,0);
		apply(lines_field,vl,0,1);
		apply(lines_field,tbl,1,1);
		apply(lines_field,btl,1,-1);
	}

	void snapshot_t::fill(const matrix<lines5_t>& lines_field, line_t& line, int dx, int dy)
	{
		for (int i = -2; i <= 2; i++)
			line[i+2] = lines_field.get(point(st.x + i * dx, st.y + i * dy));
	}

	void snapshot_t::apply(matrix<lines5_t>& lines_field, const line_t& line, int dx, int dy) const
	{
		for (int i = -2; i <= 2; i++)
			lines_field.set(point(st.x + i * dx, st.y + i * dy), line[i+2]);
	}
		
	void snapshot_t::fill(const matrix<score_t>& scores_field)
	{
		fill(scores_field,hs,1,0);
		fill(scores_field,vs,0,1);
		fill(scores_field,tbs,1,1);
		fill(scores_field,bts,1,-1);
	}

	void snapshot_t::apply(const score_set& scr_set) const
	{
		apply(scr_set,hs,1,0);
		apply(scr_set,vs,0,1);
		apply(scr_set,tbs,1,1);
		apply(scr_set,bts,1,-1);
	}

	void snapshot_t::fill(const matrix<score_t>& scores_field, scores_t& line, int dx, int dy)
	{
		for (int i = 0; i < 4; i++)
		{
			line[i]   = scores_field.get(point(st.x+(i-4)*dx, st.y+(i-4)*dy));
			line[i+4] = scores_field.get(point(st.x+(i+1)*dx, st.y+(i+1)*dy));
		}
	}

	void snapshot_t::apply(const score_set& scr_set, const scores_t& line, int dx, int dy) const
	{
		for (int i = 0; i < 4; i++)
		{
			scr_set(point(st.x+(i-4)*dx, st.y+(i-4)*dy), line[i]);
			scr_set(point(st.x+(i+1)*dx, st.y+(i+1)*dy), line[i+4]);
		}
	}


	void field5_t::change_state(const field_t& field)
	{
		set_score(field.back(), score_t(0,0));

		change_state(field,1,0);
		change_state(field,0,1);
		change_state(field,1,1);
		change_state(field,1,-1);
	}
	

	void field5_t::set_steps(const field_t::steps_t& steps)
	{
		lines_field.clear();
		scores_field.clear();
		empty_points.clear();
		field_score = 0;

		field_t field;
		for (const auto& st : steps)
		{
			field.add(st, st.step);
			change_state(field);
		}
	}

	void field5_t::change_state(const field_t& field, int dx, int dy)
	{
		const step_t& st = field.back();
		for (int i = -2; i <= 2; i++)
			change_line(field, st.step, point(st.x + i * dx, st.y + i * dy), dx, dy);
	}

	void field5_t::change_line(const field_t& field, Step color, const point& line_point, int dx, int dy)
	{
		lines5_t& sts = lines_field.get_ref(line_point);
		auto& line = sts.get_line(dx,dy);
		auto old_line = line;

		if (!line.adjust(color))
			return;

		change_score(field,old_line,line,point(line_point.x + (-2) * dx, line_point.y + (-2) * dy));
		change_score(field,old_line,line,point(line_point.x + (-1) * dx, line_point.y + (-1) * dy));
		change_score(field,old_line,line,point(line_point.x            , line_point.y            ));
		change_score(field,old_line,line,point(line_point.x + (1) * dx, line_point.y + (1) * dy));
		change_score(field,old_line,line,point(line_point.x + (2) * dx, line_point.y + (2) * dy));


		field_score+= line.cur_cnt(st_krestik) - old_line.cur_cnt(st_krestik);
		field_score-= line.cur_cnt(st_nolik) - old_line.cur_cnt(st_nolik);
	}
	
	void field5_t::change_score(const field_t& field, const line5_t& old_line, const line5_t& new_line, const point& pt)
	{
		if(field.at(pt) != st_empty)
			return;

		score_t scr = scores_field.get(pt);

		if (scr.krestik_cnt + new_line.next_cnt(st_krestik) < old_line.next_cnt(st_krestik))
		{
			throw std::runtime_error("field5_t::change_score(st_krestik): <0");
		}

		if (scr.nolik_cnt + new_line.next_cnt(st_nolik) < old_line.next_cnt(st_nolik))
		{
			throw std::runtime_error("field5_t::change_score(st_nolik): <0");
		}

		scr.krestik_cnt -= old_line.next_cnt(st_krestik);
		scr.krestik_cnt += new_line.next_cnt(st_krestik);

		scr.nolik_cnt   -= old_line.next_cnt(st_nolik);
		scr.nolik_cnt   += new_line.next_cnt(st_nolik);
		
		set_score(pt, scr);
	}

	void field5_t::set_score(const point& pt, const score_t& new_scr)
	{
		score_t& old_scr = scores_field.get_ref(pt);

		bool new_add = new_scr.krestik_cnt > 20 || new_scr.nolik_cnt > 20;
		bool old_add = old_scr.krestik_cnt > 20 || old_scr.nolik_cnt > 20;

		if (new_add != old_add)
		{
			if(new_add)
				empty_points.insert(pt);
			else
				empty_points.erase(pt);
		}

		old_scr = new_scr;
	}

	void field5_t::apply_shapshot(const snapshot_t& snapshot)
	{
		snapshot.apply(lines_field, [this](const point& pt,const score_t& scr) {set_score(pt,scr);});
		field_score = snapshot.field_score;
	}

	void field5_t::iterate_involved_lines(const point& pt, const line_visitor& visitor)
	{
		iterate_involved_lines(pt,visitor,1,0);
		iterate_involved_lines(pt,visitor,0,1);
		iterate_involved_lines(pt,visitor,1,1);
		iterate_involved_lines(pt,visitor,1,-1);
	}

	void field5_t::iterate_involved_lines(const point& pt, const line_visitor& visitor, int dx, int dy)
	{
		for (int i = -2; i <= 2; i++)
		{
			point line_point(pt.x + i * dx, pt.y + i * dy);
			lines5_t sts = lines_field.get(line_point);
			auto& line = sts.get_line(dx,dy);
			visitor(line_point,line,dx,dy);
		}
	}


	bool max_step_pr::operator()(const score_t& sa, const score_t& sb) const
	{
		Score move_a = sa.cnt(move_color);
		Score opp_a = sa.cnt(oposite_color);

		Score move_b = sb.cnt(move_color);
		Score opp_b = sb.cnt(oposite_color);

		
		bool ba = move_a>=kCount5;
		bool bb = move_b>=kCount5;
		if(ba || bb)
			return ba && !bb;

		ba = opp_a>=kCount5;
		bb = opp_b>=kCount5;
		if(ba || bb)
			return ba && !bb;


		Score ca = move_a/kCount4;
		Score cb = move_b/kCount4;
		if(ca>=2 || cb>=2)
			return ca>=2 && !(cb>=2);

		if(ca==1 || cb==1)
			return ca==1 && !(cb==1);

		ca = opp_a/kCount4;
		cb = opp_b/kCount4;
		if(ca>=2 || cb>=2)
			return ca>=2 && !(cb>=2);


		ca = move_a/kCount3;
		cb = move_b/kCount3;
		if(ca>=2 || cb>=2)
			return ca>=2 && !(cb>=2);

		return false;
	}
} }
