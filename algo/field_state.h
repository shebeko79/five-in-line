#ifndef gomoku_field_stateH
#define gomoku_field_stateH

#include "field.h"
#include <array>

namespace Gomoku { namespace State5
{
	struct line5_t
	{
		Step color = st_empty;
		unsigned steps = 1; //number of steps made if we move here
		
		bool adjust(Step new_color);
		unsigned get_score(Step for_color) const;
	};

	struct score_t
	{
		unsigned krestik_score = 20;
		unsigned nolik_score = 20;
	};

	struct lines5_t
	{
		line5_t h;
		line5_t v;
		line5_t tb;
		line5_t bt;

		line5_t& get_line(int dx, int dy);
	};

	struct snapshot_t
	{
		using line_t = std::array<lines5_t,5>;
		using scores_t = std::array<score_t,8>;

		step_t st;
		
		line_t hl;
		line_t vl;
		line_t tbl;
		line_t btl;

		scores_t hs;
		scores_t vs;
		scores_t tbs;
		scores_t bts;

		snapshot_t() = default;
		snapshot_t(const step_t& _st, const matrix<lines5_t>& lines_field, const matrix<score_t>& scores_field) : 
			st(_st)
		{
			fill(lines_field); 
			fill(scores_field); 
		}

		void apply(matrix<lines5_t>& lines_field, matrix<score_t>& scores_field) const;

	private:
		void fill(const matrix<lines5_t>& lines_field);
		void fill(const matrix<score_t>& scores_field);
		void apply(matrix<lines5_t>& lines_field) const;
		void apply(matrix<score_t>& scores_field) const;

		void fill(const matrix<lines5_t>& lines_field, line_t& line, int dx, int dy);
		void fill(const matrix<score_t>& scores_field, scores_t& line, int dx, int dy);
		void apply(matrix<lines5_t>& lines_field, const line_t& line, int dx, int dy) const;
		void apply(matrix<score_t>& scores_field, const scores_t& line, int dx, int dy) const;
	};


	class field5_t
	{
	public:
		snapshot_t make_snapshot(const step_t& st) const { return snapshot_t(st, lines_field, scores_field); }
		void apply_shapshot(const snapshot_t& snapshot) {snapshot.apply(lines_field, scores_field);}
		
		void change_state(const field_t& field);
	private:
		matrix<lines5_t> lines_field;
		matrix<score_t> scores_field;

		void change_state(const field_t& field, int dx, int dy);
		void change_line(const field_t& field,const step_t& st, const point& line_point, int dx, int dy);
		void change_score(const field_t& field,const line5_t& old_line, const line5_t& new_line, const point& pt);
	};

} }//namespace gomoku


#endif
