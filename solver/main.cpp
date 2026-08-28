#include <stdio.h>
#include "../algo/solution_tree_utils.h"
#include "../algo/game.h"
#include "../algo/field_state_player.h"

#include "../extern/object_progress.hpp"
#include "../algo/env_variables.h"


ObjectProgress::logout_cerr log_err;
ObjectProgress::logout_file log_file;

using namespace Gomoku;
using namespace Gomoku::State5;

void print_use()
{
	printf("USE: solver key\n");
    Gomoku::print_enviropment_variables_hint();
}

std::string print_state(const node_t& r,const steps_t& key)
{
	const auto& neutrals = r.get_neutrals();
	const auto& wins = r.get_wins();
	const auto& fails = r.get_fails();

	std::string str;
	data_t bin;

	std::string ret;

	points2bin(key,bin);
	bin2hex(bin,str);
	ret+="k="+str;

	if(!neutrals.empty())
	{
		points2bin(neutrals,bin);
		bin2hex(bin,str);
		ret+="&n="+str;
	}

	if(!wins.empty())
	{
		points2bin(wins,bin);
		bin2hex(bin,str);
		ret+="&w="+str;
	}

	if(!fails.empty())
	{
		points2bin(fails,bin);
		bin2hex(bin,str);
		ret+="&f="+str;
	}

	return ret;
}

int main(int argc,char** argv)
{
	if(argc!=2)
	{
		print_use();
		return 1;
	}

    log_err.open();
        
    log_file.file_name="solver.log";
    log_file.print_timestamp=true;
    log_file.open();

    scan_enviropment_variables();

	ObjectProgress::log_generator lg(true);
    print_used_enviropment_variables(lg);

	try
	{
		steps_t init_state;
		hex_or_str2points(argv[1],init_state);

		if(init_state.empty())
		{
			print_use();
			return 1;
		}
		
		reorder_to_proper_last_color(init_state);
		Step last_step=last_color(init_state.size());

		game_t gm;
		gm.field().set_steps(init_state);

		State5::field_state_player_t pl;

		pl.init(gm,other_color(last_step));
		node_t r = pl.solve();

		std::string ln=print_state(r,init_state);
		printf("%s",ln.c_str());
	}
	catch(std::exception& e)
	{
        lg<<"std::exception: "<<e.what();
		printf("std::exception: %s\n",e.what());
		return 1;
	}
	catch(...)
	{
        lg<<"unknown exception";
		return 1;
	}

	return 0;
}
