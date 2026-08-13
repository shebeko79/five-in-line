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

        POLIMVAR_IMPLEMENT_CLONE( check_player_t )
	};
}
#endif
