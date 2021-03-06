#ifndef solution_treeH
#define solution_treeH
#include <stdio.h>
#include "ibin_index.h"
#include "solution_tree_utils.h"
#include "../algo/field.h"
#include "../extern/object_progress.hpp"

namespace Gomoku
{
	
	struct sol_state_t
	{
        //wins for next step from state key
        unsigned long long wins_count;
        //fails for next step from state key
        unsigned long long fails_count;
		steps_t key;

        points_t neitrals;
        //step from state key is a win
		npoints_t solved_wins;
        //step from state key is a fail
		npoints_t solved_fails;
        //step from state key is a win
		npoints_t tree_wins;
        //step from state key is a fail
		npoints_t tree_fails;

		sol_state_t()
		{
            wins_count = 0;
            fails_count = 0;
		}

		void pack(data_t& bin) const;
		void unpack(const data_t& bin);

		bool is_win() const{return !solved_wins.empty()||!tree_wins.empty();}
        bool is_completed() const{return is_win() || neitrals.empty();}
		unsigned min_win_chain() const;
		unsigned max_fail_chain() const;

        inline double get_win_rate() const{return static_cast<double>(wins_count+1)/(fails_count+1);}
        inline bool is_win_fail_stat_empty() const{return wins_count==0&&fails_count==0;}
	public:
		static void pack(const points_t& pts,data_t& bin);
		static void unpack(const data_t& bin,points_t& pts,size_t& from);
		static void pack(const steps_t& pts,data_t& bin);
		static void unpack(const data_t& bin,steps_t& pts,size_t& from);
		static void pack(const npoints_t& pts,data_t& bin);
		static void unpack(const data_t& bin,npoints_t& pts,size_t& from);
	};

	struct deep_solve_t
	{
		steps_t key;
		std::vector<points_t> neitrals;

		void pack(data_t& bin) const;
		void unpack(const data_t& bin);

		inline bool empty() const{return key.empty();}

		steps_t get_sorted_key() const
		{
			steps_t ret=key;
			sort_steps(ret);
			return ret;
		}

		const points_t& get_key_neitrals(size_t cur_key_size) const;
		void trunc_to_key_size(size_t cur_key_size);
		inline size_t get_root_key_size() const{return key.size()-neitrals.size();}
	};

    typedef std::pair<point,steps_t*> state_ref_t;
    typedef std::vector<state_ref_t> state_refs_t;

    struct sol_state_width_pr
    {
        virtual ~sol_state_width_pr(){}

        //return true if val changed
        virtual bool on_enter_node(sol_state_t& val){return false;}

        virtual bool is_canceled(){return false;}
    };

    struct sol_state_visitor_pr
    {
        virtual ~sol_state_visitor_pr(){}

        //return true if val changed
        virtual bool on_enter_node(sol_state_t& val){return false;}

        //return true if val changed
        virtual bool on_exit_node(sol_state_t& val){return false;}

        virtual bool should_scan_neitrals(const sol_state_t& val){return true;}
        virtual bool should_scan_tree_fails(const sol_state_t& val){return true;}
        virtual bool should_scan_tree_wins(const sol_state_t& val){return true;}

        virtual bool is_canceled(){return false;}
    };

	class isolution_tree_base_t
	{
	public:
		virtual ~isolution_tree_base_t(){}

		virtual bool get(sol_state_t& res) const = 0;
		virtual void set(const sol_state_t& val) = 0;
		virtual steps_t get_root_key() const=0;
        virtual void width_first_search_from_bottom_to_top(sol_state_width_pr& pr) = 0;
	};

	typedef boost::shared_ptr<isolution_tree_base_t> isolution_tree_base_ptr;

    class solution_tree_t
	{
	private:
		std::string base_dir;
		isolution_tree_base_ptr db_ptr;
		isolution_tree_base_t& db;
		mutable deep_solve_t first_solving;
		mutable deep_solve_t last_solving;

		void load_solve(deep_solve_t& val,const std::string& _file_name) const;
		void save_solve(const deep_solve_t& val,const std::string& _file_name) const;

        bool rewind_to_not_solved(bool first_rewind,deep_solve_t& key);

        void scan_already_solved_neitrals(sol_state_t& base_st);
        void update_base_wins_and_fails(const sol_state_t& child_st,unsigned long long delta_wins,unsigned long long delta_fails);
		
		bool get_root_first_deep(deep_solve_t& _val);
		bool get_first_deep(deep_solve_t& val,unsigned max_key_size);

        template<typename T>
        static void check_really_unique(const steps_t& key,const std::vector<T>& vals,const std::string& vals_name);

        bool get_ant_job(const steps_t& base_st_key,const npoints_t& wins_hint,steps_t& result_key);
        void load_all_childs_neitrals(const sol_state_t base_st,std::vector<steps_t>& childs,state_refs_t& refs);
        void load_all_fails_its_wins(const sol_state_t base_st,npoints_t& wins);

	public:
		static const char* first_solving_file_name;
		static const char* last_solving_file_name;
		static unsigned win_neitrals;
		
		solution_tree_t(const isolution_tree_base_ptr _db_ptr) : db_ptr(_db_ptr),db(*_db_ptr){}

		void init(const std::string& _base_dir);
		void create_init_tree();

		bool get_job(steps_t& key);
		bool get_ant_job(steps_t& key);
        bool get_ant_job(const steps_t& root_key,steps_t& key);
		void save_job(const steps_t& key,const points_t& neitrals,const npoints_t& win,const npoints_t& fails);
		static void trunc_neitrals(const steps_t& key,points_t& neitrals,const npoints_t& win,const npoints_t& fails);

		bool get(sol_state_t& res) const;
		void set(const sol_state_t& val);

		steps_t get_root_key() const;
		void relax(const sol_state_t& child_st);

        void depth_first_search(sol_state_visitor_pr& pr);
        void depth_first_search(const steps_t& key,sol_state_visitor_pr& pr);

        void width_first_search_from_bottom_to_top(sol_state_width_pr& pr);
	};

}//namespace

#endif
