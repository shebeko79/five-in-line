#ifndef field_state_playerH
#define field_state_playerH
#include "game.h"
#include "field_state.h"

namespace Gomoku { namespace State5
{
	class node_t;

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


	class node_t : public step_t
	{
	public:
		node_t(field_state_player_t& _player, const step_t& st);

		void process();

		point get_next_step() const;

		inline const points_t& get_neitrals() const {return neitrals;};
		inline const points_t& get_wins() const {return wins;};
		inline const points_t& get_fails() const {return fails;};
	private:
		field_state_player_t& player;

		points_t neitrals; 
		points_t wins;
		points_t fails;
	};


} }//namespace Gomoku

#endif

