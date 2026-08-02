#ifndef gomoku_field_stateH
#define gomoku_field_stateH

#include "field.h"
#include <array>

namespace Gomoku
{
	struct state5_t
	{
		Step color = st_empty;
		unsigned steps = 1; //number of steps made if we move here
		
		void increase(Step new_color);
	};


	struct states5_t
	{
		using line_t = std::array<state5_t,5>;

		unsigned krestik_score = 20;
		unsigned nolik_score = 20;
		line_t h;
		line_t v;
		line_t tb;
		line_t bt;

		void evaluate();
		line_t& get_line(int dx, int dy);
	
	private:
		void evaluate(const line_t& arr);
	};

	struct states5_snapshot_t
	{
		using line_t = std::array<states5_t,8>;

		line_t h;
		line_t v;
		line_t tb;
		line_t bt;

		states5_snapshot_t() = default;
		states5_snapshot_t(const step_t& st, const matrix<states5_t>& states) { fill(st, states); }

		void fill(const step_t& st, const matrix<states5_t>& states);
	private:
		void fill(const step_t& st, const matrix<states5_t>& states, line_t& line, int dx, int dy);
	};


	class field5_t
	{
	public:
		void push(const field_t& field);
		void pop();
	private:
		matrix<states5_t> fld;
		std::vector<states5_snapshot_t> shapshot_stack;

		void change_state(const field_t& field, int dx, int dy);
	};

}//namespace gomoku


#endif
