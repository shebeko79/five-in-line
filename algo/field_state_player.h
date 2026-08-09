#ifndef field_state_playerH
#define field_state_playerH
#include "game.h"
#include "field_state.h"

namespace Gomoku { namespace State5
{
	class node_t;

	constexpr unsigned common_deep = 2;
	constexpr unsigned threat_deep = 5;

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


		template<class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
#ifdef BOOST_SERIALIZATION_NVP
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(iplayer_t);
#endif
        }

        POLIMVAR_IMPLEMENT_CLONE(field_state_player_t )
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

		bool mark_unchecked_make_move(points_t& pts,const matrix<score_t>& scores_field);
		bool make_move_find_win(const points_t& pts);
		void make_move(const point& p);

		bool mark_limit_reached();
	};


} }//namespace Gomoku

#endif

