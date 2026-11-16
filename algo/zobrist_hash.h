#ifndef ZOBRIST_HASH_H
#define ZOBRIST_HASH_H

#include <cstdint>
#include "field.h"

namespace Gomoku
{
	class zobrist_hash
	{
	public:
		uint64_t get(const step_t& st) {return m_rnd.get_ref(st).get(st.step);} 

	private:
		struct val_t
		{
			uint64_t krestik;
			uint64_t nolik;

			val_t();
			inline uint64_t get(Step color) const {return color==st_krestik? krestik : nolik;}
		};

		matrix<val_t> m_rnd;
		static uint64_t get_random_uint64();
	};

}//namespace

#endif