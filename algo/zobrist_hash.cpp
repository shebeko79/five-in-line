#include <random>
#include "zobrist_hash.h"

namespace Gomoku
{

zobrist_hash::val_t::val_t() :
	krestik(get_random_uint64()),
	nolik(get_random_uint64())
{
}

uint64_t zobrist_hash::get_random_uint64()
{
	static std::random_device rd;
	static std::mt19937_64 gen(rd());
	static std::uniform_int_distribution<uint64_t> dis;

	return dis(gen);
}

}//namespace
