#ifndef gomoku_field_stateH
#define gomoku_field_stateH

#include <functional>
#include "field.h"
#include <array>
#include "../extern/binary_find.h"

namespace Gomoku { namespace State5
{
	using Score = int;
	
	constexpr Score kCount2 = (1<<5);
	constexpr Score kCount3 = (1<<10);
	constexpr Score kCount4 = (1<<15);
	constexpr Score kCount5 = (1<<20);

	constexpr Score kScore3 = 6;
	constexpr Score kScore4 = 17;
	constexpr Score kScore5 = 33;

	unsigned max_step(Score score);
	Score prev_cnt(Score score);

	struct line5_t
	{
		Step color = st_empty;
		unsigned steps = 1; //number of steps made if we move here
		
		bool adjust(Step new_color);
		Score next_cnt(Step for_color) const;
		Score cur_cnt(Step for_color) const;
	};

	struct score_t
	{
		Score krestik_cnt = 20;
		Score nolik_cnt = 20;
		
		score_t() = default;
		score_t(Score k, Score n) : krestik_cnt(k), nolik_cnt(n) {}
		
		inline Score cnt(Step color) const {return color == st_krestik? krestik_cnt : nolik_cnt;}
		inline Score& cnt(Step color) {return color == st_krestik? krestik_cnt : nolik_cnt;}
	};

	struct lines_count_t
	{
		Score l5 = 0;
		Score l4 = 0;
		Score l3 = 0;
		Score l2 = 0;

		lines_count_t() = default;
		lines_count_t(Score cnt);

		void operator+=(const lines_count_t& rhs);
		void operator-=(const lines_count_t& rhs);
		Score score() const;

		void log_statistic() const;
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

		lines_count_t field_krestik;
		lines_count_t field_nolik;

		snapshot_t() = default;
		snapshot_t(const step_t& _st, const matrix<lines5_t>& lines_field, const matrix<score_t>& scores_field, 
			const lines_count_t& _field_krestik, const lines_count_t& _field_nolik) : 
			st(_st),
			central_s(scores_field.get(_st)),
			field_krestik(_field_krestik),
			field_nolik(_field_nolik)
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
	
	class field5_t
	{
	public:
		using line_visitor = std::function<void (const point& line_point,const line5_t& line, int dx, int dy)>;

		snapshot_t make_snapshot(const step_t& st) const { return snapshot_t(st, lines_field, scores_field, field_krestik, field_nolik); }
		void apply_shapshot(const snapshot_t& snapshot);
		
		void change_state(const field_t& field);
		void set_steps(const field_t::steps_t& steps);
		inline const matrix<score_t>& get_scores_field() const {return scores_field;}
		inline const points_set_t& get_empty_points() const {return empty_points;}
		inline const lines_count_t& get_field_krestik() const {return field_krestik;}
		inline const lines_count_t& get_field_nolik() const {return field_nolik;}

		void iterate_involved_lines(const point& pt, const line_visitor& visitor);
		
		Score get_score(const point& p, Step move_color) const;
	private:
		matrix<lines5_t> lines_field;
		matrix<score_t> scores_field;
		points_set_t empty_points;
		lines_count_t field_krestik;
		lines_count_t field_nolik;
		
		void change_state(const field_t& field, int dx, int dy);
		void change_line(const field_t& field,Step color, const point& line_point, int dx, int dy);
		void change_score(const field_t& field,const line5_t& old_line, const line5_t& new_line, const point& pt);

		void set_score(const point& pt, const score_t& new_scr);
		
		void iterate_involved_lines(const point& pt, const line_visitor& visitor, int dx, int dy);
	};

	
	struct score_pr
	{
		const field5_t& field;
		const Step move_color;
		const int k;

		score_pr(const field5_t& _field, Step _move_color) :
			field(_field),
			move_color(_move_color),
			k(move_color==st_krestik? 1:-1)
		{
		}

		inline bool operator()(const point& pa, const point& pb) const
		{
			Score sa = field.get_score(pa, move_color);
			Score sb = field.get_score(pb, move_color);
			return k*sa > k*sb;
		}
	};

	struct fscore_pr : near_point_pr
	{
		const int k;
		
		fscore_pr(Step move_color) : near_point_pr(point(0,0)), k(move_color==st_krestik? 1:-1){}
		inline bool operator()(const ipoint& pa, const ipoint& pb) const
		{
			if(pa.i != pb.i)
				return pa.i*k > pb.i*k;

			return near_point_pr::operator()(pa,pb);
		}
	};

	struct max_step_pr
	{
		const matrix<score_t>& scores_field;
		const Step move_color;
		const Step oposite_color;
		const int k;

		max_step_pr(const matrix<score_t>& _scores_field, Step _move_color) :
			scores_field(_scores_field),
			move_color(_move_color),
			oposite_color(other_color(_move_color)),
			k(move_color==st_krestik? 1:-1)
		{
		}

		bool operator()(const score_t& sa, const score_t& sb) const;
		
		inline bool operator()(const point& pa, const point& pb) const
		{
			return operator()(scores_field.get(pa), scores_field.get(pb));
		}

		inline bool operator()(const score_t& sa, const point& pb) const
		{
			return operator()(sa, scores_field.get(pb));
		}

		inline bool operator()(const point& pa, const score_t& sb) const
		{
			return operator()(scores_field.get(pa), sb);
		}
	};

} }//namespace gomoku


#endif
