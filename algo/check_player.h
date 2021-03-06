#ifndef gomoku_check_playerH
#define gomoku_check_playerH
#include "game.h"

namespace Gomoku
{
	class check_player_t : public iplayer_t
	{
		steps_t steps;
    public:
		void delegate_step();

        bool is_thinking() const;
		void set_steps(const steps_t& val){steps=val;}

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
#ifdef BOOST_SERIALIZATION_NVP
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(iplayer_t);
            ar & BOOST_SERIALIZATION_NVP(steps);
#endif
        }

        POLIMVAR_IMPLEMENT_CLONE( check_player_t )
	};
}
#endif
