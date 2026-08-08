#include <stdio.h>
#include "field_state.h"


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

	unsigned line5_t::get_score(Step for_color) const
	{
		if (steps == 0)
			return 0;

		if(color != st_empty && color != for_color)
			return 0;

		switch (steps)
		{
		case 2:
			return kScore2;
		case 3:
			return kScore3;
		case 4:
			return kScore4;
		case 5:
			return kScore5;
		}
		
		return 1;
	}

	line5_t& lines5_t::get_line(int dx, int dy)
	{
		if(dy == 0) return h;
		if(dx == 0) return v;
		if(dy == 1) return tb;
		return bt;
	}

	unsigned max_steps(unsigned score)
	{
		if(score == 0)
			return 0;

		if(score>=kScore5)
			return 5;

		if(score>=kScore4)
			return 4;

		if(score>=kScore3)
			return 3;

		if(score>=kScore2)
			return 2;

		return 1;
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
			line[i] = lines_field.get(point(st.x + i * dx, st.y + i * dy));
	}

	void snapshot_t::apply(matrix<lines5_t>& lines_field, const line_t& line, int dx, int dy) const
	{
		for (int i = -2; i <= 2; i++)
			lines_field.set(point(st.x + i * dx, st.y + i * dy), line[i]);
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

	sorted_scores::sorted_scores()
	{
		p5.reserve(4);
		p4.reserve(20);
	}

	void sorted_scores::update(const point& pt, unsigned old_scr, unsigned new_scr)
	{
		unsigned old_step = max_steps(old_scr);
		unsigned new_step = max_steps(new_scr);

		if(old_step == new_step)
			return;


	}
	
	void sorted_scores::add(const point& pt, unsigned step)
	{
		switch (step)
		{
		case 5:
			insert(p5, pt);
			break;
		case 4:
			sorted_insert(p4, pt);
			break;
		case 3:
			p3.insert(pt);
			break;
		case 2:
			p2.insert(pt);
			break;
		}
	}

	void sorted_scores::remove(const point& pt, unsigned step)
	{
		switch (step)
		{
		case 5:
			erase(p5, pt);
			break;
		case 4:
			sorted_erase(p4, pt);
			break;
		case 3:
			p3.erase(pt);
			break;
		case 2:
			p2.erase(pt);
			break;
		}
	}


	void field5_t::change_state(const field_t& field)
	{
		change_state(field,1,0);
		change_state(field,0,1);
		change_state(field,1,1);
		change_state(field,1,-1);
	}
	

	void field5_t::set_steps(const field_t::steps_t& steps)
	{
		lines_field.clear();
		scores_field.clear();

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
			change_line(field, st, point(st.x + i * dx, st.y + i * dy), dx, dy);
	}

	void field5_t::change_line(const field_t& field, const step_t& st, const point& line_point, int dx, int dy)
	{
		lines5_t sts = lines_field.get(line_point);
		auto& line = sts.get_line(dx,dy);
		auto old_line = line;

		if (!line.adjust(st.step))
			return;

		lines_field.set(line_point, sts);

		change_score(field,old_line,line,point(st.x + (-2) * dx, st.y + (-2) * dy));
		change_score(field,old_line,line,point(st.x + (-1) * dx, st.y + (-1) * dy));
		change_score(field,old_line,line,point(st.x + (1) * dx, st.y + (1) * dy));
		change_score(field,old_line,line,point(st.x + (2) * dx, st.y + (2) * dy));
	}
	
	void field5_t::change_score(const field_t& field, const line5_t& old_line, const line5_t& new_line, const point& pt)
	{
		if(field.at(pt) != st_empty)
			return;

		score_t scr = scores_field.get(pt);

		scr.krestik_score -= old_line.get_score(st_krestik);
		scr.krestik_score += new_line.get_score(st_krestik);

		scr.nolik_score   -= old_line.get_score(st_nolik);
		scr.nolik_score   += new_line.get_score(st_nolik);
		
		set_score(pt, scr);
	}

	void field5_t::set_score(const point& pt, const score_t& new_scr)
	{
		score_t old_scr = scores_field.get(pt);
		scores_field.set(pt, new_scr);

		sorted_krestik.update(pt,old_scr.krestik_score, new_scr.krestik_score);
		sorted_nolik  .update(pt,old_scr.nolik_score,   new_scr.nolik_score);
	}

	void field5_t::apply_shapshot(const snapshot_t& snapshot)
	{
		snapshot.apply(lines_field, [this](const point& pt,const score_t& scr) {set_score(pt,scr);});
	}

} }
