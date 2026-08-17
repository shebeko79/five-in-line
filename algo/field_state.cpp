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

	void max_step(unsigned score, unsigned& step, unsigned& lines_count)
	{
		if (score == 0)
		{
			step = 0;
			lines_count = 0;
			return;
		}

		if (score >= kScore5)
		{
			step = 5;
			lines_count = score / kScore5;
			return;
		}

		if (score >= kScore4)
		{
			step = 4;
			lines_count = score / kScore4;
			return;
		}

		if (score >= kScore3)
		{
			step = 3;
			lines_count = score / kScore3;
			return;
		}

		if (score >= kScore2)
		{
			step = 2;
			lines_count = score / kScore2;
			return;
		}

		step = 0;
		lines_count = score;
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

	sorted_scores::sorted_scores()
	{
		p5.reserve(4);
		p4h.reserve(8);
		p4l.reserve(20);
		p3h.reserve(20);
	}

	void sorted_scores::update(const point& pt, unsigned old_scr, unsigned new_scr)
	{
		unsigned old_step, old_count;
		max_step(old_scr, old_step, old_count);

		unsigned new_step, new_count;
		max_step(new_scr, new_step, new_count);

		if (old_step == new_step)
		{
			switch (new_step)
			{
			case 4:
				if( (old_count>=2) == (new_count>=2) )
					return;
				break;
			case 3:
				if( (old_count>=4) == (new_count>=4) )
					return;
				break;
			default:
				return;
			}
		}

		remove(pt, old_step,old_count);
		add(pt, new_step, new_count);
	}
	
	void sorted_scores::add(const point& pt, unsigned step, unsigned lines_count)
	{
		switch (step)
		{
		case 5:
			insert(p5, pt);
			break;
		case 4:
			if(lines_count>=2)
				insert(p4h, pt);
			else
				sorted_insert(p4l, pt);
			break;
		case 3:
			if(lines_count>=4)
				sorted_insert(p3h, pt);
			else
				p3l.insert(pt);
			break;
		case 2:
			p2.insert(pt);
			break;
		}
	}

	void sorted_scores::remove(const point& pt, unsigned step, unsigned lines_count)
	{
		switch (step)
		{
		case 5:
			erase(p5, pt);
			break;
		case 4:
			if(lines_count>=2)
				erase(p4h, pt);
			else
				sorted_erase(p4l, pt);
			break;
		case 3:
			if(lines_count>=4)
				sorted_erase(p3h, pt);
			else
				p3l.erase(pt);
			break;
		case 2:
			p2.erase(pt);
			break;
		}
	}

	void sorted_scores::log_statistic() const
	{
		ObjectProgress::log_generator lg(true);
		
		lg<<"Capacity: p5="<<p5.capacity()
			<<" p4h="<<p4h.capacity()
			<<" p4l="<<p4l.capacity()
			<<" p3h="<<p3h.capacity();
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
		sorted_krestik = sorted_scores();
		sorted_nolik = sorted_scores();

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
		lines5_t sts = lines_field.get(line_point);
		auto& line = sts.get_line(dx,dy);
		auto old_line = line;

		if (!line.adjust(color))
			return;

		lines_field.set(line_point, sts);

		change_score(field,old_line,line,point(line_point.x + (-2) * dx, line_point.y + (-2) * dy));
		change_score(field,old_line,line,point(line_point.x + (-1) * dx, line_point.y + (-1) * dy));
		change_score(field,old_line,line,point(line_point.x            , line_point.y            ));
		change_score(field,old_line,line,point(line_point.x + (1) * dx, line_point.y + (1) * dy));
		change_score(field,old_line,line,point(line_point.x + (2) * dx, line_point.y + (2) * dy));
	}
	
	void field5_t::change_score(const field_t& field, const line5_t& old_line, const line5_t& new_line, const point& pt)
	{
		if(field.at(pt) != st_empty)
			return;

		score_t scr = scores_field.get(pt);

		if (scr.krestik_score + new_line.get_score(st_krestik) < old_line.get_score(st_krestik))
		{
			throw std::runtime_error("field5_t::change_score(st_krestik): <0");
		}

		if (scr.nolik_score + new_line.get_score(st_nolik) < old_line.get_score(st_nolik))
		{
			throw std::runtime_error("field5_t::change_score(st_nolik): <0");
		}

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


	
	void sort(points_t& arr, const matrix<score_t>& scores_field, Step move_color)
	{
		std::sort(arr.begin(), arr.end(), [&scores_field, move_color](const point& pa, const point& pb)
		{
			score_t sa = scores_field.get(pa);
			score_t sb = scores_field.get(pb);
			return sa.total(move_color) > sb.total(move_color);
		});
	}

} }
