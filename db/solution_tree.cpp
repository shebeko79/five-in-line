#include "solution_tree.h"
#include <functional>
#include <numeric>
#include <stdexcept>
#include "../extern/binary_find.h"
#include <boost/lexical_cast.hpp>
#include "../extern/pair_comparator.h"

namespace fs=std::filesystem;

namespace Gomoku
{
	const char* solution_tree_t::first_solving_file_name="first_solving";
	const char* solution_tree_t::last_solving_file_name="last_solving";

	void solution_tree_t::init(const std::string& _base_dir)
	{
		base_dir=_base_dir;

		load_solve(first_solving,first_solving_file_name);
		load_solve(last_solving,last_solving_file_name);
	}

	void solution_tree_t::load_solve(deep_solve_t& val,const std::string& _file_name) const
	{
		val=deep_solve_t();

		fs::path file_name=fs::path(base_dir)/_file_name;
		if(!fs::exists(file_name))
			return;

        
		data_t bin;
		load_file(file_name,bin);
		val.unpack(bin);
	}

	void solution_tree_t::save_solve(const deep_solve_t& val,const std::string& _file_name) const
	{
		fs::path file_name=fs::path(base_dir)/_file_name;

		data_t bin;
		val.pack(bin);
		save_file(file_name,bin);
	}

	void solution_tree_t::create_init_tree()
	{
		sol_state_t s;
		s.key.push_back(step_t(st_krestik,0,0));

		s.neutrals.push_back({ point(1,0),0 });
		s.neutrals.push_back({ point(1,1),0 });
		s.neutrals.push_back({ point(2,0),0 });
		s.neutrals.push_back({ point(2,1),0 });
		s.neutrals.push_back({ point(2,2),0 });

		save_job(s.key,s.neutrals,s.solved_wins,s.solved_fails);
	}

	bool solution_tree_t::get(sol_state_t& res) const
	{
		return db.get(res);
	}

	void solution_tree_t::set(const sol_state_t& val)
	{
		db.set(val);
	}

	steps_t solution_tree_t::get_root_key() const
	{
		return db.get_root_key();
	}

	bool solution_tree_t::get_root_first_deep(deep_solve_t& _val)
	{
		deep_solve_t val;
		val.key=get_root_key();
		if(val.key.empty())return false;
		
		if(!get_first_deep(val,(unsigned)-1))return false;
		_val=val;
		return true;
	}
	
	bool solution_tree_t::get_first_deep(deep_solve_t& val,unsigned max_key_size)
	{
		sol_state_t ss;
		ss.key=val.key;
		
		if(ss.key.size()>=max_key_size)
			return false;

		if(!get(ss))
			return true;

		if(ss.is_completed())
			return false;

		Step move_step=next_color(val.key.size());
		
		val.key.push_back(step_t(move_step,0,0));
		val.neutrals.push_back(ss.neutrals);
		
		for(size_t i=0;i<ss.neutrals.size();i++)		
		{
			const point& p=ss.neutrals[i];
			static_cast<point&>(val.key.back())=p;
			
			if(get_first_deep(val,max_key_size))
				return true;
		}
		
		val.key.pop_back();
		val.neutrals.pop_back();
		
		return false;
	}

	bool solution_tree_t::get_job(steps_t& key)
	{
		if(last_solving.empty())
		{
			if(!get_root_first_deep(last_solving))return false;
			first_solving=last_solving;
			key=last_solving.key;

			save_solve(first_solving,first_solving_file_name);
			save_solve(last_solving,last_solving_file_name);

			return true;
		}

		if(rewind_to_not_solved(true,last_solving))
		{
			key=last_solving.key;
			save_solve(last_solving,last_solving_file_name);
			return true;
		}

		last_solving=first_solving;

		if(rewind_to_not_solved(false,last_solving))
		{
			key=last_solving.key;
			save_solve(last_solving,last_solving_file_name);
			return true;
		}

		if(!get_root_first_deep(last_solving))return false;
		first_solving=last_solving;
		key=last_solving.key;

		save_solve(first_solving,first_solving_file_name);
		save_solve(last_solving,last_solving_file_name);

		return true;
	}
	
	bool solution_tree_t::rewind_to_not_solved(bool first_rewind,deep_solve_t& key)
	{
		if(!first_rewind)
		{
			sol_state_t st;
			st.key=key.key;
			if(!get(st))
				return true;
		}

		steps_t p;

		for(steps_t k=key.key;k.size()>key.get_root_key_size();k=p)
		{
			p=steps_t(k.begin(),k.end()-1);

			const auto& n=key.get_key_neutrals(p.size());

			auto it=std::find(n.begin(),n.end(),k.back());
			if(it==n.end())throw std::runtime_error("rewind_to_not_solved(): neutral does not exist: "+print_steps(k)+" of "+print_steps(key.key));

            for(++it;it!=n.end();++it)
			{
				deep_solve_t child=key;
				child.trunc_to_key_size(k.size());
				child.key=p;
				
				Step move_step=last_color(k.size());
				child.key.push_back(step_t(move_step,it->x,it->y));
                

				if(get_first_deep(child,key.key.size()))
				{
					key=child;
					return true;
				}
			}
		}
        
		return false;
	}

	void solution_tree_t::save_job(const steps_t& key,const ipoints_t& neutrals,const npoints_t& win,const npoints_t& fails)
	{
        check_really_unique(key,neutrals,"neutrals");
        check_really_unique(key,win,"win");
        check_really_unique(key,fails,"fails");

		sol_state_t st;
		st.key=key;
		
		if(get(st))
			return;

		st.neutrals=neutrals;
		st.solved_wins=win;
		st.solved_fails=fails;

        scan_already_solved_neutrals(st);

		set(st);

		if(first_solving.get_sorted_key()==st.key)
		{
			if(!rewind_to_not_solved(true,first_solving))
				first_solving=last_solving;
			save_solve(first_solving,first_solving_file_name);
		}

		relax(st);
	}

	void solution_tree_t::relax(const sol_state_t& child_st)
	{
		Step cur_step=last_color(child_st.key.size());
		
		sol_state_t prev_st;
		steps_t& key=prev_st.key;

		unsigned n = 0;
		int best_score = 0;

		if (child_st.is_completed())
		{
			if (child_st.is_win())n = child_st.min_win_chain() + 1;
			else n = child_st.max_fail_chain() + 1;
		}
		else
		{
			best_score = child_st.best_neutral_score();
		}

		for(size_t i=0;i<child_st.key.size();i++)
		{
			const step_t& st=child_st.key[i];
			if(st.step!=cur_step)continue;

			key=child_st.key;
			key.erase(key.begin()+i);

			if(!get(prev_st))continue;

			if(prev_st.is_completed())
				continue;

			if (child_st.is_completed())
			{
				prev_st.neutrals.erase(
					std::remove(prev_st.neutrals.begin(), prev_st.neutrals.end(), st), prev_st.neutrals.end());

				npoints_t& solved = child_st.is_win() ? prev_st.tree_fails : prev_st.tree_wins;

				npoint p(st);
				p.n = n;

				npoints_t::iterator it = std::find(solved.begin(), solved.end(), st);
				if (it == solved.end())solved.push_back(p);
				else *it = p;
			}
			else
			{
				auto it = std::find(prev_st.neutrals.begin(), prev_st.neutrals.end(), st);
				if (it != prev_st.neutrals.end())
					it->i = best_score;
			}

			set(prev_st);

			if(prev_st.key.size()>1)
				relax(prev_st);
		}
	}

    void solution_tree_t::scan_already_solved_neutrals(sol_state_t& base_st)
    {
		Step next_step=next_color(base_st.key.size());
        
        sol_state_t st;
		steps_t& key=st.key;

		for(size_t ii=base_st.neutrals.size();ii>0;ii--)
		{
            size_t i=ii-1;
			auto& p=base_st.neutrals[i];
			
			key=base_st.key;
			key.push_back(step_t(next_step,p.x,p.y));

            if(!get(st))
                continue;

            if(st.is_win())
            {
                unsigned n=st.min_win_chain()+1;
                base_st.tree_fails.push_back(npoint(p,n));
                base_st.neutrals.erase(base_st.neutrals.begin()+i);
            }
            else if(st.neutrals.empty())
            {
                unsigned n=st.max_fail_chain()+1;
                base_st.tree_wins.push_back(npoint(p,n));
                base_st.neutrals.erase(base_st.neutrals.begin()+i);
            }
			else
			{
				p.i = st.best_neutral_score();
			}
		}
    }

    template<typename T>
    void solution_tree_t::check_really_unique(const steps_t& key,const std::vector<T>& vals,const std::string& vals_name)
    {
        std::vector<T> cp(vals);
        make_unique(cp);
        if(cp.size() != vals.size())
            throw std::runtime_error("check_really_unique() failed: key="+print_steps(key)+" "+vals_name+"="+print_points(vals));
    }

	bool solution_tree_t::get_ant_job(steps_t& key)
	{
        return get_ant_job(get_root_key(),key);
	}
    
    bool solution_tree_t::get_ant_job(const steps_t& base_st_key, steps_t& result_key)
    {
		sol_state_t base_st;
		base_st.key=base_st_key;

		if(!get(base_st))
		{
            result_key=base_st.key;
			return true;
		}

		if(base_st.neutrals.empty())
			return false;

		auto it = std::min_element(base_st.neutrals.begin(), base_st.neutrals.end(),
			[](const ipoint& pa, const ipoint& pb)
			{
				//if next step is st_krestik we value strongest moves first
				//if next step is st_nolik we value weakest moves first to keep whole tree as narrow as possible
				return pa.i > pb.i;
			});

		Step next_cl = next_color(base_st_key.size());
		steps_t child_st = base_st_key;
		child_st.push_back(step_t(next_cl, *it));

		return get_ant_job(child_st, result_key);
    }
    
    void solution_tree_t::depth_first_search(sol_state_visitor_pr& pr)
    {
        return depth_first_search(get_root_key(),pr);
    }

    void solution_tree_t::depth_first_search(const steps_t& key,sol_state_visitor_pr& pr)
    {
        if(pr.is_canceled())
            throw e_cancel();

        sol_state_t st;
        st.key=key;

        if(!get(st))
            return;

        if(pr.on_enter_node(st))
        {
            set(st);
        }

        steps_t child_key(key);
        child_key.push_back(step_t(next_color(key.size()),0,0));

        if(pr.should_scan_neutrals(st))
        {
            for(size_t i=0;i<st.neutrals.size();i++)
            {
                static_cast<point&>(child_key.back())=st.neutrals[i];
                depth_first_search(child_key,pr);
            }
        }

        if(pr.should_scan_tree_fails(st))
        {
            for(size_t i=0;i<st.tree_fails.size();i++)
            {
                static_cast<point&>(child_key.back())=st.tree_fails[i];
                depth_first_search(child_key,pr);
            }
        }

        if(pr.should_scan_tree_wins(st))
        {
            for(size_t i=0;i<st.tree_wins.size();i++)
            {
                static_cast<point&>(child_key.back())=st.tree_wins[i];
                depth_first_search(child_key,pr);
            }
        }

        if(pr.on_exit_node(st))
        {
            set(st);
        }
    }

    void solution_tree_t::width_first_search_from_bottom_to_top(sol_state_width_pr& pr)
    {
		return db.width_first_search_from_bottom_to_top(pr);
    }

/////////////////////////////////////////////////////////////////////////////////
//
	void sol_state_t::pack(data_t& bin) const
	{
		bin.resize(0);

		pack(key,bin);
		pack(neutrals,bin);
		pack(solved_wins,bin);
		pack(solved_fails,bin);
		pack(tree_wins,bin);
		pack(tree_fails,bin);
	}

	void sol_state_t::unpack(const data_t& bin)
	{
		size_t from=0;

		unpack(bin,key,from);
		unpack(bin,neutrals,from);
		unpack(bin,solved_wins,from);
		unpack(bin,solved_fails,from);
		unpack(bin,tree_wins,from);
		unpack(bin,tree_fails,from);
	}

	void sol_state_t::pack(const points_t& pts,data_t& bin)
	{
		data_t d;
		points2bin(pts,d);
		size_t sz=d.size();
		const unsigned char* psz=reinterpret_cast<const unsigned char*>(&sz);
		bin.insert(bin.end(),psz,psz+sizeof(size_t));
		bin.insert(bin.end(),d.begin(),d.end());
	}

	void sol_state_t::unpack(const data_t& bin,points_t& pts,size_t& from)
	{
		if(from+sizeof(size_t)>bin.size())
			throw std::runtime_error("sol_state_t::unpack(): unpack size failed");

		size_t sz=*reinterpret_cast<const size_t*>(&bin[from]);
		from+=sizeof(size_t);

		if(from+sz>bin.size())
			throw std::runtime_error("sol_state_t::unpack(): unpack failed");

		data_t d(bin.begin()+from,bin.begin()+from+sz);
		bin2points(d,pts);
		from+=sz;
	}

	void sol_state_t::pack(const steps_t& pts,data_t& bin)
	{
		data_t d;
		points2bin(pts,d);
		size_t sz=d.size();
		const unsigned char* psz=reinterpret_cast<const unsigned char*>(&sz);
		bin.insert(bin.end(),psz,psz+sizeof(size_t));
		bin.insert(bin.end(),d.begin(),d.end());
	}

	void sol_state_t::unpack(const data_t& bin,steps_t& pts,size_t& from)
	{
		if(from+sizeof(size_t)>bin.size())
			throw std::runtime_error("sol_state_t::unpack(): unpack size failed");

		size_t sz=*reinterpret_cast<const size_t*>(&bin[from]);
		from+=sizeof(size_t);

		if(from+sz>bin.size())
			throw std::runtime_error("sol_state_t::unpack(): unpack failed");

		data_t d(bin.begin()+from,bin.begin()+from+sz);
		bin2points(d,pts);
		from+=sz;
	}

	void sol_state_t::pack(const npoints_t& pts,data_t& bin)
	{
		data_t d;
		points2bin(pts,d);
		size_t sz=d.size();
		const unsigned char* psz=reinterpret_cast<const unsigned char*>(&sz);
		bin.insert(bin.end(),psz,psz+sizeof(size_t));
		bin.insert(bin.end(),d.begin(),d.end());
	}

	void sol_state_t::unpack(const data_t& bin,npoints_t& pts,size_t& from)
	{
		if(from+sizeof(size_t)>bin.size())
			throw std::runtime_error("sol_state_t::unpack(): unpack size failed");

		size_t sz=*reinterpret_cast<const size_t*>(&bin[from]);
		from+=sizeof(size_t);

		if(from+sz>bin.size())
			throw std::runtime_error("sol_state_t::unpack(): unpack failed");

		data_t d(bin.begin()+from,bin.begin()+from+sz);
		bin2points(d,pts);
		from+=sz;
	}

	void sol_state_t::pack(const ipoints_t& pts,data_t& bin)
	{
		data_t d;
		points2bin(pts,d);
		size_t sz=d.size();
		const unsigned char* psz=reinterpret_cast<const unsigned char*>(&sz);
		bin.insert(bin.end(),psz,psz+sizeof(size_t));
		bin.insert(bin.end(),d.begin(),d.end());
	}

	void sol_state_t::unpack(const data_t& bin,ipoints_t& pts,size_t& from)
	{
		if(from+sizeof(size_t)>bin.size())
			throw std::runtime_error("sol_state_t::unpack(): unpack size failed");

		size_t sz=*reinterpret_cast<const size_t*>(&bin[from]);
		from+=sizeof(size_t);

		if(from+sz>bin.size())
			throw std::runtime_error("sol_state_t::unpack(): unpack failed");

		data_t d(bin.begin()+from,bin.begin()+from+sz);
		bin2points(d,pts);
		from+=sz;
	}

	unsigned get_min_n(const npoints_t& vals)
	{
		unsigned ret=(unsigned)-1;
		for(unsigned i=0;i<vals.size();i++)
			if(vals[i].n<ret)ret=vals[i].n;
		return ret;
	}

	unsigned get_max_n(const npoints_t& vals)
	{
		unsigned ret=0;
		for(unsigned i=0;i<vals.size();i++)
			if(vals[i].n>ret)ret=vals[i].n;
		return ret;
	}

	unsigned sol_state_t::min_win_chain() const
	{
		unsigned a=get_min_n(solved_wins);
		unsigned b=get_min_n(tree_wins);
		if(a<b)return a;
		return b;
	}

	unsigned sol_state_t::max_fail_chain() const
	{
		unsigned a=get_max_n(solved_fails);
		unsigned b=get_max_n(tree_fails);
		if(a>b)return a;
		return b;
	}

	int sol_state_t::best_neutral_score() const
	{
		if(neutrals.empty())
			throw std::runtime_error("best_neutral_score(): neutrals is empty");
		const int k = last_color(key.size()) == st_krestik? 1:0;
		return std::min_element(neutrals.begin(), neutrals.end(), 
			[k](const ipoint& pa, const ipoint& pb){return pa.i*k > pb.i*k;})->i;
	}

	void deep_solve_t::pack(data_t& bin) const
	{
		bin.resize(0);

		sol_state_t::pack(key,bin);

		size_t neutrals_counts=neutrals.size();
        const unsigned char* p=reinterpret_cast<const unsigned char*>(&neutrals_counts);
		bin.insert(bin.end(),p,p+sizeof(neutrals_counts));

		for(size_t i=0;i<neutrals_counts;i++)
			sol_state_t::pack(neutrals[i],bin);
	}

	void deep_solve_t::unpack(const data_t& bin)
	{
		size_t from=0;
		sol_state_t::unpack(bin,key,from);

		size_t neutrals_counts=0;
		if(from+sizeof(neutrals_counts)>bin.size())
			throw std::runtime_error("deep_solve_t::unpack(): unpack neutrals_counts failed");
		neutrals_counts=*(const size_t*)(&bin[from]);
		from+=sizeof(neutrals_counts);

		neutrals.resize(neutrals_counts);

		for(size_t i=0;i<neutrals_counts;i++)
			sol_state_t::unpack(bin,neutrals[i],from);
	}

	const ipoints_t& deep_solve_t::get_key_neutrals(size_t cur_key_size) const
	{
		unsigned idx=neutrals.size()-(key.size()-cur_key_size);
		if(idx>=neutrals.size())throw std::runtime_error("get_key_neutrals(): invalid neutrals index");
		return neutrals[idx];
	}

	void deep_solve_t::trunc_to_key_size(size_t cur_key_size)
	{
		unsigned idx=neutrals.size()-(key.size()-cur_key_size);
		if(idx>neutrals.size())throw std::runtime_error("get_key_neutrals(): invalid neutrals index");
		neutrals.erase(neutrals.begin()+idx,neutrals.end());
	}
}//namespace
