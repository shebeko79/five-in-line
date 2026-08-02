#include <stdio.h>
#include "field_state.h"


namespace Gomoku
{
	void state5_t::increase(Step new_color)
	{
		if (steps == 0)
			return;

		if (color == st_empty)
		{
			color = new_color;
			++steps;
		}
		else if (color == new_color)
			++steps;
		else
			steps = 0;
	}

	void states5_t::evaluate()
	{
		krestik_score = 0;
		nolik_score = 0;

		evaluate(h);
		evaluate(v);
		evaluate(tb);
		evaluate(bt);
	}

	void states5_t::evaluate(const line_t& arr)
	{
		for (const auto& v : arr)
		{
			if(v.steps == 0)
				continue;

			switch (v.color)
			{
			case st_empty:
				krestik_score += v.steps;
				nolik_score += v.steps;
				break;
			case st_krestik:
				krestik_score += v.steps;
				break;
			case st_nolik:
				nolik_score += v.steps;
				break;
			}
		}
	}

	states5_t::line_t& states5_t::get_line(int dx, int dy)
	{
		if(dy == 0) return h;
		if(dx == 0) return v;
		if(dy == 1) return tb;
		return bt;
	}


	void states5_snapshot_t::fill(const step_t& st, const matrix<states5_t>& states)
	{
		fill(st,states,h,1,0);
		fill(st,states,v,0,1);
		fill(st,states,tb,1,1);
		fill(st,states,bt,1,-1);
	}

	void states5_snapshot_t::fill(const step_t& st, const matrix<states5_t>& states, line_t& line, int dx, int dy)
	{
		for (int i = 0; i < 4; i++)
		{
			line[i]   = states.get(point(st.x+(i-4)*dx, st.y+(i-4)*dy));
			line[i+4] = states.get(point(st.x+(i+1)*dx, st.y+(i+1)*dy));
		}
	}
		


	void field5_t::push(const field_t& field)
	{
		shapshot_stack.emplace_back(states5_snapshot_t(field.back(), fld));
		change_state(field, 1, 0);
	}

	void field5_t::pop()
	{
		shapshot_stack.pop_back();
	}

	void field5_t::change_state(const field_t& field, int dx, int dy)
	{
		const step_t& last_move = field.back();

		for (int i = 0; i < 4; i++)
		{
			point pt(last_move.x+(i-4)*dx, last_move.y+(i-4)*dy);
			
			if(field.at(pt) != st_empty)
				continue;

			states5_t sts = fld.get(pt);
			auto& line = sts.get_line(dx,dy);

			for (int j = 0; j <= i; j++)
				line[j].increase(last_move.step);

			sts.evaluate();
			fld.set(pt, sts);
		}

		for (int i = 0; i < 4; i++)
		{
			point pt(last_move.x+(i+1)*dx, last_move.y+(i+1)*dy);
			
			if(field.at(pt) != st_empty)
				continue;

			states5_t sts = fld.get(pt);
			auto& line = sts.get_line(dx,dy);

			for (int j = i+1; j <= 4; j++)
				line[j].increase(last_move.step);

			sts.evaluate();
			fld.set(pt, sts);
		}
	}
}
