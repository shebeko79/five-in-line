#ifndef gomoku_field_stateH
#define gomoku_field_stateH

#include <functional>
#include "field.h"
#include <array>
#include "../extern/binary_find.h"

namespace Gomoku { namespace State5
{
	constexpr unsigned kScore2 = (1u<<6);
	constexpr unsigned kScore3 = (1u<<12);
	constexpr unsigned kScore4 = (1u<<18);
	constexpr unsigned kScore5 = (1u<<24);


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
			//this is probably wrong function. 
			// *2 overlaps with with oposite color
			return move_color == st_krestik? krestik_score*2+nolik_score : nolik_score*2 + krestik_score;
		}

		inline unsigned score(Step color) const {return color == st_krestik? krestik_score : nolik_score;} 
	};

	unsigned max_step(unsigned score);

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
		points_t p4h;
		points_t p4l;
		points_set_t p3;
		points_set_t p2;

		sorted_scores();
		
		void update(const point& pt, unsigned old_scr, unsigned new_scr);

		bool p5_exists(const point& p) const {return std::find(p5.begin(),p5.end(),p)!=p5.end();}
		bool p4h_exists(const point& p) const {return std::find(p4h.begin(),p4h.end(),p)!=p4h.end();}
		bool p4l_exists(const point& p) const {return binary_find(p4l.begin(),p4l.end(),p,less_point_pr())!=p4l.end();}
		bool p3_exists(const point& p) const {return p3.find(p) != p3.end();}
		bool p2_exists(const point& p) const {return p2.find(p) != p2.end();}

		inline auto p5_pr() const { return [this] (const point& p){return p5_exists(p);}; }
		inline auto p4_pr() const { return [this] (const point& p){return p5_exists(p) || p4h_exists(p) || p4l_exists(p);}; }
		inline auto p3_pr() const { return [this] (const point& p){return p5_exists(p) || p4h_exists(p) || p4l_exists(p) || p3_exists(p);}; }
		inline auto p2_pr() const { return [this] (const point& p){return p5_exists(p) || p4h_exists(p) || p4l_exists(p) || p3_exists(p) || p2_exists(p);}; }

		void log_statistic() const;
		unsigned size() const {return p5.size() + p4h.size() + p4l.size() + p3.size() + p2.size();}
	private:
		void add(const point& pt, unsigned step, unsigned scr);
		void remove(const point& pt, unsigned step, unsigned scr);
	};


	class field5_t
	{
	public:
		using line_visitor = std::function<void (const point& line_point,const line5_t& line, int dx, int dy)>;

		snapshot_t make_snapshot(const step_t& st) const { return snapshot_t(st, lines_field, scores_field); }
		void apply_shapshot(const snapshot_t& snapshot);
		
		void change_state(const field_t& field);
		void set_steps(const field_t::steps_t& steps);
		inline const sorted_scores& get_sorted(Step st) const {return st==st_krestik? sorted_krestik: sorted_nolik;};
		inline const matrix<score_t>& get_scores_field() const {return scores_field;}

		void iterate_involved_lines(const point& pt, const line_visitor& visitor);
	private:
		matrix<lines5_t> lines_field;
		matrix<score_t> scores_field;
		sorted_scores sorted_krestik;
		sorted_scores sorted_nolik;


		void change_state(const field_t& field, int dx, int dy);
		void change_line(const field_t& field,Step color, const point& line_point, int dx, int dy);
		void change_score(const field_t& field,const line5_t& old_line, const line5_t& new_line, const point& pt);

		void set_score(const point& pt, const score_t& new_scr);
		
		void iterate_involved_lines(const point& pt, const line_visitor& visitor, int dx, int dy);
	};

	
	struct score_pr
	{
		const matrix<score_t>& scores_field;
		const Step move_color;

		score_pr(const matrix<score_t>& _scores_field, Step _move_color) :
			scores_field(_scores_field),
			move_color(_move_color)
		{
		}

		inline bool operator()(const point& pa, const point& pb) const
		{
			score_t sa = scores_field.get(pa);
			score_t sb = scores_field.get(pb);
			return sa.total(move_color) > sb.total(move_color);
		}
	};

} }//namespace gomoku


#endif
