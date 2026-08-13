#ifndef field_state_playerH
#define field_state_playerH
#include "game.h"
#include "field_state.h"

namespace Gomoku { namespace State5
{
	class node_t;

	constexpr unsigned common_deep = 3;
	constexpr unsigned threat_deep = 9;

	class field_state_player_t : public iplayer_t
	{
	private:
		field_t field;
		field5_t field5;

        int thinking = 0;

		friend class node_t;
	public:
		void delegate_step() override;
        bool is_thinking() const override{return thinking>0;}


        POLIMVAR_IMPLEMENT_CLONE(field_state_player_t )
	};

	class fork_t
	{
	public:
		fork_t() = default;
		fork_t(const point& p) : active(true),count(1)
		{
			pts[0] = p;
		}

		inline void add(const point& p)
		{
			if(std::find(pts,pts+count,p) != pts+count)
				return;
			if(count == 3)
				throw std::out_of_range("fork_t() exceed space");

			pts[count]=p;
			++count;
		}

		void merge(const fork_t& other)
		{
			if (!active)
			{
				*this = other;
				return;
			}
			
			active = other.active;
			count = std::remove_if(pts, pts + count, [&other](const point& p)
				{
					return std::find(other.pts,other.pts+other.count,p) == other.pts+other.count;
				}) - pts;
		}

		inline bool is_active() const {return active;}
		inline bool is_empty_set() const {return active && count==0;}
		inline bool inside(const point& p) const {return !active || std::find(pts,pts+count,p) != pts+count;}
	private:
		bool active = false;
		unsigned count = 0;
		point pts[3];
	};


	class node_t
	{
	public:
		const step_t prev_step;
		const unsigned deep;
		const Step move_color;

		node_t(field_state_player_t& _player, const step_t& st, unsigned _deep = 0);

		void process();

		const point& get_next_step() const;

		inline const points_t& get_neutrals() const {return neutrals;};
		inline const npoints_t& get_wins() const {return wins;};
		inline const npoints_t& get_fails() const {return fails;};

		const npoint* get_min_win() const;
		const npoint* get_max_fail() const;
	private:
		field_state_player_t& player;

		points_t neutrals; 
		npoints_t wins;
		npoints_t fails;
		
		bool unchecked_exists = false;
		bool deep_limit_reached = false;
		unsigned forced_max_fail = 0;

		fork_t oposite_fork;

		bool mark_unchecked_make_move(points_t& pts,const matrix<score_t>& scores_field);
		bool make_move_find_win(const points_t& pts);
		void make_move(const point& p);

		bool mark_limit_reached();
		void limit_to_p4_fork(const sorted_scores& other_srt);
	};


} }//namespace Gomoku

#endif

