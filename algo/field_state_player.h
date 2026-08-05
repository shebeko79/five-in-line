#ifndef field_state_playerH
#define field_state_playerH
#include "game.h"
#include "field_state.h"

namespace Gomoku { namespace State5
{
	class field_state_player_t : public iplayer_t
	{
	private:
		field_t field;
		field5_t states;

        int thinking = 0;

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


} }//namespace Gomoku

#endif

