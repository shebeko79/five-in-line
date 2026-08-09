#ifndef gomoku_field_stateH
#define gomoku_field_stateH

#include <functional>
#include "field.h"
#include <array>
#include "../extern/binary_find.h"

namespace Gomoku { namespace State5
{
	constexpr unsigned kScore2 = 41;
	constexpr unsigned kScore3 = 1681;
	constexpr unsigned kScore4 = 68921;
	constexpr unsigned kScore5 = 2825761;


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
		
		score_t() = default;
		score_t(unsigned k, unsigned n) : krestik_score(k), nolik_score(n) {}
		
		inline unsigned total(Step move_color) const 
		{
			return move_color == st_krestik? krestik_score*2+nolik_score : nolik_score*2 + krestik_score;
		}
	};

	unsigned max_steps(unsigned score);

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
		using score_set = std::function<void (const point&,const score_t&)>;

		step_t st;
		
		line_t hl;
		line_t vl;
		line_t tbl;
		line_t btl;

		score_t  central_s;
		scores_t hs;
		scores_t vs;
		scores_t tbs;
		scores_t bts;

		snapshot_t() = default;
		snapshot_t(const step_t& _st, const matrix<lines5_t>& lines_field, const matrix<score_t>& scores_field) : 
			st(_st),
			central_s(scores_field.get(_st))
		{
			fill(lines_field); 
			fill(scores_field); 
		}

		void apply(matrix<lines5_t>& lines_field, const score_set& scr_set) const;

	private:
		void fill(const matrix<lines5_t>& lines_field);
		void fill(const matrix<score_t>& scores_field);
		void apply(matrix<lines5_t>& lines_field) const;
		void apply(const score_set& scr_set) const;

		void fill(const matrix<lines5_t>& lines_field, line_t& line, int dx, int dy);
		void fill(const matrix<score_t>& scores_field, scores_t& line, int dx, int dy);
		void apply(matrix<lines5_t>& lines_field, const line_t& line, int dx, int dy) const;
		void apply(const score_set& scr_set, const scores_t& line, int dx, int dy) const;
	};

	struct sorted_scores
	{
		points_t p5;
		points_t p4;
		points_set_t p3;
		points_set_t p2;

		sorted_scores();
		
		void update(const point& pt, unsigned old_scr, unsigned new_scr);

		bool p5_exists(const point& p) const {return std::find(p5.begin(),p5.end(),p)!=p5.end();}
		bool p4_exists(const point& p) const {return binary_find(p4.begin(),p4.end(),p,less_point_pr())!=p4.end();}
		bool p3_exists(const point& p) const {return p3.find(p) != p3.end();}
		bool p2_exists(const point& p) const {return p2.find(p) != p2.end();}
	
	private:
		void add(const point& pt, unsigned step);
		void remove(const point& pt, unsigned step);
	};


	class field5_t
	{
	public:
		snapshot_t make_snapshot(const step_t& st) const { return snapshot_t(st, lines_field, scores_field); }
		void apply_shapshot(const snapshot_t& snapshot);
		
		void change_state(const field_t& field);
		void set_steps(const field_t::steps_t& steps);
		inline const sorted_scores& get_sorted(Step st) const {return st==st_krestik? sorted_krestik: sorted_nolik;};
		inline const matrix<score_t>& get_scores_field() const {return scores_field;}
	private:
		matrix<lines5_t> lines_field;
		matrix<score_t> scores_field;
		sorted_scores sorted_krestik;
		sorted_scores sorted_nolik;


		void change_state(const field_t& field, int dx, int dy);
		void change_line(const field_t& field,Step color, const point& line_point, int dx, int dy);
		void change_score(const field_t& field,const line5_t& old_line, const line5_t& new_line, const point& pt);

		void set_score(const point& pt, const score_t& new_scr);
	};

	
	void sort(points_t& arr, const matrix<score_t>& scores_field, Step move_color);

} }//namespace gomoku


#endif
