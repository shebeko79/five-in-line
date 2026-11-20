#ifndef ZOBRIST_HASH_H
#define ZOBRIST_HASH_H

#include "field.h"

namespace Gomoku
{
	using hash_t = unsigned long long;

	class zobrist_hash
	{
	public:
		hash_t get(const step_t& st) {return m_rnd.get_ref(st).get(st.step);} 

	private:
		struct val_t
		{
			hash_t krestik;
			hash_t nolik;

			val_t();
			inline hash_t get(Step color) const {return color==st_krestik? krestik : nolik;}
		};

		matrix<val_t> m_rnd;
		static hash_t get_random_uint64();
	};

}//namespace

#endif