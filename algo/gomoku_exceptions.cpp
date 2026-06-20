
#include "gomoku_exceptions.h"
#include <boost/lexical_cast.hpp>

namespace Gomoku
{

e_invalid_step::e_invalid_step(unsigned val)
{
	mess="Invalid step "+std::to_string(val);
}

e_point_busy::e_point_busy(int x,int y)
{
	mess="Point ("+std::to_string(x)+";"+std::to_string(y)+")";
}

}//namespace Gomoku
