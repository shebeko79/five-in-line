#ifdef _WIN32
#  include <windows.h>
#endif

#include <stdio.h>

#ifndef _WIN32
#  include <signal.h>
#endif

#include <chrono>
#include <boost/algorithm/string.hpp>
#include "solution_tree.h"
#include "solution_tree_fixes.h"
#include "bin_index.h"
#include <stdexcept>
#include "../extern/object_progress.hpp"
#include "../algo/field_state_player.h"
#include "../algo/env_variables.h"
#include "bin_index_solution_base.h"

using namespace Gomoku;
using namespace Gomoku::State5;

ObjectProgress::logout_cerr log_err;
ObjectProgress::logout_file log_file;

namespace fs=std::filesystem;

void print_use()
{
	printf("USE: \n");
	printf("db <root_dir> get_job\n");
	printf("db <root_dir> get_ant_job [root_key]\n");
	printf("db <root_dir> save_job <file_name>\n");
	printf("db <root_dir> get <key>\n");
	printf("db <root_dir> view <printable_steps>\n");
	printf("db <root_dir> view_root\n");
	printf("db <root_dir> solve_level [iteration_count]\n");
	printf("db <root_dir> solve_ant [root_key] [iteration_count]\n");
	printf("db <root_dir> fix_zero_fails\n");
	printf("db <root_dir> relax <key>\n");
    Gomoku::print_enviropment_variables_hint();
}

void self_solve(solution_tree_t& tr,const steps_t& key)
{
	std::string str_key=print_steps(key);
	ObjectProgress::log_generator lg(true);
	lg<<"Solving: "<<str_key;

	steps_t init_state=key;
	reorder_to_proper_last_color(init_state);

	Step last_step=last_color(init_state.size());

	game_t gm;
	gm.field().set_steps(init_state);

	State5::field_state_player_t pl;

	pl.init(gm,other_color(last_step));
	node_t r = pl.solve();

	const npoints_t& wins = r.get_wins();
	const npoints_t& fails = r.get_fails();
	const ipoints_t& neutrals = r.get_neutrals();

	lg<<str_key<<": n="<<neutrals.size()<<" w="<<wins.size()<<" f="<<fails.size();

	tr.save_job(key,neutrals,wins,fails);
}

static bool need_break;

#ifdef _WIN32
BOOL WINAPI CtrlCHandlerRoutine(DWORD dwCtrlType)
{
	if(dwCtrlType!=CTRL_C_EVENT)return FALSE;
	need_break=true;
	return TRUE;
}
#else
void sig_handler(int v)
{
	need_break=true;
}
#endif

struct fix_zero_deep_fails_impls : public fix_zero_deep_fails
{
    fix_zero_deep_fails_impls(solution_tree_t& _tree) : fix_zero_deep_fails(_tree) {}
    virtual bool is_canceled(){return need_break;}
};

void set_ctrl_handler()
{
	need_break=false;
#ifdef _WIN32
	SetConsoleCtrlHandler(CtrlCHandlerRoutine,TRUE);
#else
  signal(SIGINT,sig_handler);
	signal(SIGTSTP,sig_handler);
#endif
}

void self_solve(solution_tree_t& tr,size_t iteration_count,const steps_t& root_key=steps_t(),bool use_ant=false)
{
    scan_enviropment_variables();

	ObjectProgress::log_generator lg(true);
	print_used_enviropment_variables(lg);

    set_ctrl_handler();

	size_t key_len=0;

	for(size_t i=0;;i++)
	{
		if(need_break)
		{
			lg<<"Ctrl-C pressed. Exit";
			printf("Ctrl-C pressed. Exit\n");
			return;
		}

		if(iteration_count>0&&i>=iteration_count)
		{
			printf("solver iteration count reached\n");
			return;
		}

		steps_t key;
        bool r;
        if(use_ant)
        {
            if(!root_key.empty())r=tr.get_ant_job(root_key,key);
            else r=tr.get_ant_job(key);

        }
        else r=tr.get_job(key);

		if(!r)
		{
			printf("no job anymore\n");
			return;
		}

        if(!use_ant)
        {
		    if(key_len==0)key_len=key.size();
		    else if(key.size()!=key_len)
		    {
#ifdef _WIN32
			    printf("level %llu succesefully solved\n",key_len);
#else
			    printf("level %lu succesefully solved\n",key_len);
#endif
			    return;
		    }
        }
		lg<<"";
		lg<<"self_solve(): i="<<i<<" iteration_count="<<iteration_count;
		self_solve(tr,key);
	}
}

bool show_state(solution_tree_t& tr,steps_t req)
{
	sol_state_t st;
	data_t bin;
	std::string str;

	st.key=req;

	if(!tr.get(st))
	{
		printf("%s does not exist\n",print_steps(req).c_str());
		return false;
	}
				
	data_t bin_key;
	points2bin(st.key,bin_key);

	std::string h;
	bin2hex(bin_key,h);

	sort(st.neutrals,fscore_pr(next_color(st.key.size())));

	sort(st.solved_wins,less_n_pr());
	sort(st.solved_fails,greater_n_pr());
	sort(st.tree_wins,less_n_pr());
	sort(st.tree_fails,greater_n_pr());

	std::string k=print_steps(st.key);
	std::string n=print_points(st.neutrals);
	std::string sw=print_points(st.solved_wins);
	std::string sf=print_points(st.solved_fails);
	std::string tw=print_points(st.tree_wins);
	std::string tf=print_points(st.tree_fails);
	std::string field_str=print_field(req);

	printf("key: %s\nhex_key: %s\nneutrals: %s\nsolved wins: %s\ntree wins: %s\nsolved fails: %s\ntree fails: %s\nfield:\n%s",
        k.c_str(),h.c_str(),
		n.c_str(),
		sw.c_str(),tw.c_str(),
		sf.c_str(),tf.c_str(),
		field_str.c_str());

	return true;
}

void save_job(solution_tree_t& tr,const std::string& file_name)
{
	data_t file_content;
	Gomoku::load_file(file_name,file_content);

	std::string content(file_content.begin(),file_content.end());

    std::vector< std::string > lines;
	boost::split( lines, content, boost::is_any_of("\n"));

	for(size_t i=0;i<lines.size();i++)
	{
		std::string& s=lines[i];
		boost::trim(s);

		if(s.empty())
			continue;
		
		std::vector< std::string > parts;
		boost::split( parts, s, boost::is_any_of(";"));

		if(parts.size()!=4)
			throw std::runtime_error("Couldn't parse line: '"+s+"'");

		for(size_t i=0;i<parts.size();i++)
			boost::trim(parts[i]);

		data_t bin;

		steps_t key;
		ipoints_t neutrals;
		npoints_t win;
		npoints_t fails;
		
		hex2bin(parts[0],bin);
		bin2points(bin,key);
		
		hex2bin(parts[1],bin);
		bin2points(bin,neutrals);
		
		hex2bin(parts[2],bin);
		bin2points(bin,win);
		
		hex2bin(parts[3],bin);
		bin2points(bin,fails);
		
		tr.save_job(key,neutrals,win,fails);
	}
}

int main(int argc,char** argv)
{
	if(argc<3)
	{
		print_use();
		return 1;
	}

    log_err.open();
        
    log_file.file_name="db.log";
    log_file.print_timestamp=true;
    log_file.open();

    ObjectProgress::log_generator lg(true);
	
	unsigned seed = static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	srand(seed);

	try
	{

		fs::path root_dir(argv[1]);
		std::string cmd=argv[2];

		auto db = std::make_shared<bin_index_solution_base_t>(root_dir.string());
		 
		solution_tree_t tr(db);
		tr.init(root_dir.string());

		if(!fs::exists(root_dir))
		{
			fs::create_directory(root_dir);
			self_solve(tr, steps_t{step_t(st_krestik,0,0)});
		}


		if(cmd=="get_job" ||cmd=="get_ant_job")
		{
			steps_t key;
            
            
            bool r;
            if(cmd=="get_job") r=tr.get_job(key);
            else
            {
                if(argc<4)r=tr.get_ant_job(key);
                else
                {
                    steps_t root_key;
                    hex_or_str2points(argv[3],root_key);
                    r=tr.get_ant_job(root_key,key);
                }
            }

			if(!r)
			{
				printf("no job anymore\n");
				return 1;
			}

			data_t bin_key;
			points2bin(key,bin_key);

			std::string ret;
			bin2hex(bin_key,ret);
			printf("%s",ret.c_str());
		}
		else if(cmd=="save_job")
		{
			if(argc!=4)
			{
				print_use();
				return 1;
			}

			save_job(tr,argv[3]);
		}
		else if(cmd=="get")
		{
			if(argc!=4)
			{
				print_use();
				return 1;
			}

			sol_state_t st;
			data_t bin;
			std::string str;

			hex2bin(argv[3],bin);
			bin2points(bin,st.key);

			if(!tr.get(st))
			{
				printf("%s does not exist\n",argv[3]);
				return 1;
			}

			points2bin(st.key,bin);
			bin2hex(bin,str);
			if(str.empty())str="empty";
			printf("%s\n",str.c_str());

			points2bin(st.neutrals,bin);
			bin2hex(bin,str);
			if(str.empty())str="empty";
			printf("%s\n",str.c_str());

			points2bin(st.solved_wins,bin);
			bin2hex(bin,str);
			if(str.empty())str="empty";
			printf("%s\n",str.c_str());

			points2bin(st.solved_fails,bin);
			bin2hex(bin,str);
			if(str.empty())str="empty";
			printf("%s\n",str.c_str());

			points2bin(st.tree_wins,bin);
			bin2hex(bin,str);
			if(str.empty())str="empty";
			printf("%s\n",str.c_str());

			points2bin(st.tree_fails,bin);
			bin2hex(bin,str);
			if(str.empty())str="empty";
			printf("%s\n",str.c_str());
		}
		else if(cmd=="view")
		{
			if(argc!=4)
			{
				print_use();
				return 1;
			}

			steps_t req;
            hex_or_str2points(argv[3],req);
			if(!show_state(tr,req))
				return 1;
		}
		else if(cmd=="view_root")
		{
			steps_t req=tr.get_root_key();
			if(req.empty())
			{
				printf("no root exists\n");
				return 1;
			}

			if(!show_state(tr,req))
				return 1;
		}
		else if (cmd=="solve_level")
		{
			int iter_count=0;

			if(argc>=4)iter_count=atol(argv[3]);
			self_solve(tr,iter_count);
		}
		else if (cmd=="solve_ant")
		{
			int iter_count=0;
            steps_t root_key;
			
            if(argc>=4)hex_or_str2points(argv[3],root_key);
            if(argc>=5)iter_count=atol(argv[4]);

			self_solve(tr,iter_count,root_key,true);
		}
		else if(cmd=="fix_zero_fails")
		{
            set_ctrl_handler();
            fix_zero_deep_fails_impls pr(tr);

            try
            {
                tr.depth_first_search(pr);
            }
            catch(Gomoku::e_cancel& )
            {
                lg<<"fix_zero_fails canceled";
            }

            lg<<"fix_zero_fails: fixed="<<pr.fixed_count;
		}
		else if(cmd=="relax")
		{
			if(argc!=4)
			{
				print_use();
				return 1;
			}

			steps_t req;
            hex_or_str2points(argv[3],req);

			sol_state_t st;
			st.key=req;
			if(!tr.get(st))
				throw std::runtime_error("state not found");

			tr.relax(st);
		}
		else 
		{
			print_use();
			return 1;
		}
	}
	catch(std::exception& e)
	{
        lg<<"std::exception: "<<e.what();
		return 1;
	}
	catch(...)
	{
        lg<<"unknown exception";
		return 1;
	}

	return 0;
}
