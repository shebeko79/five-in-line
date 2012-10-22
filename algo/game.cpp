
#include "game_xml.h"
#include "gomoku_exceptions.h"
#include "field_xml.h"
#ifdef USE_XML
#  include <cppexpat/xml_service.h>
#  include <cppexpat/pack_library.h>
#endif

namespace Gomoku
{

game_t::game_t() : fieldp(new field_t)
{
}


void game_t::begin_play()
{
	field().clear();
	field().add(point(0,0),st_krestik);

	continue_play();
}

void game_t::continue_play()
{
	krestik->init(*this,st_krestik);
	nolik->init(*this,st_nolik);

	krestik->begin_game();
	nolik->begin_game();
	if(field().size()%2)nolik->delegate_step();
	else krestik->delegate_step();
}

bool game_t::is_game_over() const
{
	return !field().empty()&&field().check_five(field().back());
}

void game_t::make_step(const iplayer_t& pl,const point& pt)
{
	if(&pl!=krestik.get()&&&pl!=nolik.get())throw e_invalid_step(pl.color());
	if((field().size()%2==0) != (&pl==krestik.get()))throw e_invalid_step(pl.color());

	field().add(pt,pl.color());
#ifndef WITHOUT_EXTERNAL_LIBS
	OnNextStep(*this);
#endif
	if(is_game_over()) return;

	if(field().size()%2) nolik->delegate_step();
	else krestik->delegate_step();
}

void game_t::end_play()
{
	field().clear();
}

bool game_t::is_play() const
{
	return !field().empty()&&krestik!=0&&nolik!=0&&!is_game_over();
}

}//namespace Gomoku


#ifdef USE_XML
namespace Xpat
{
	void xmlpack(ipacker_t& root_node,const Gomoku::game_t& val)
	{
		XML_NPACK("field",val.field());
		XML_NPACK("krestik",val.get_krestik());
		XML_NPACK("nolik",val.get_nolik());
	}
	
	void xmlunpack(const ipacker_t& root_node,Gomoku::game_t& val)
	{
		XML_NUNPACK("field",val.field());
		Gomoku::player_ptr krestik,nolik;
		XML_MUNPACK(krestik);
		XML_MUNPACK(nolik);
		val.set_krestik(krestik);
		val.set_nolik(nolik);
	}

}//namespace
#endif