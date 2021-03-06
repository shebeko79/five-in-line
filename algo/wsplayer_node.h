#ifndef gomoku_wsplayer_nodeH
#define gomoku_wsplayer_nodeH
#include "wsplayer_common.h"

namespace Gomoku { namespace WsPlayer
{
	class item_t;
	typedef boost::shared_ptr<item_t> item_ptr;
	typedef std::vector<item_ptr> items_t;

	class selected_wins_childs
	{
	private:
		selected_wins_childs(const selected_wins_childs&);
		void operator=(const selected_wins_childs&);
	protected:
		unsigned current_chain_depth;
		items_t vals;
		item_ptr best_val;
	public:
		selected_wins_childs();
		virtual ~selected_wins_childs(){}
		
		virtual void add(const item_ptr& val);
		virtual void clear();
		inline const item_ptr& get_best() const{return best_val;}
		inline bool empty() const{return vals.empty();}
		inline const items_t& get_vals() const{return vals;}
		inline unsigned get_chain_depth() const{return current_chain_depth;}
		inline size_t size() const{return vals.size();}
	};

	class selected_fails_childs : public selected_wins_childs
	{
		npoints_t win_hints;

		void add_wins_hint(const item_ptr& val);
	public:
		virtual void add(const item_ptr& val);
		virtual void clear();
		inline const npoints_t& get_win_hins() const{return win_hints;}
	};


	class existing_npoints_sort_pr
	{
		const npoints_t& ref;
		less_point_pr pr;
	public:
		existing_npoints_sort_pr(const npoints_t& _ref) : ref(_ref){}
		
		bool operator()(const item_ptr& a,const item_ptr& b)const;
	};

	struct win_rate_cmp_pr
	{
		bool operator()(const item_ptr& a,const item_ptr& b)const;
	};

	class item_t : public step_t
	{
		item_t(const item_t&);
		void operator=(const item_t&);
	protected:
		items_t neitrals;
		selected_wins_childs wins;
		selected_fails_childs fails;

        template<class Points>
		void add_neitrals(const Points& pts);

		void add_and_process_neitrals(const npoints_t& pts,unsigned drop_generation);

		void clear();

		void process_predict_treat_sequence(bool need_fill_neitrals);
		void process_predictable_move(bool need_fill_neitrals);
		void process_treat_sequence();
		virtual void process_neitrals(bool need_fill_neitrals,unsigned from=0,const item_t* parent_node=0);
		void process_deep_stored();
        bool is_defence_five_exists() const;
		size_t select_ant_neitral(const item_t* parent_node);

		void process_deep_ant();
		void calculate_deep_wins_fails();
		void solve_ant(const item_t* parent_node=0);

		virtual item_ptr create_neitral_item(const step_t& s);
		item_ptr create_neitral_item(const Gomoku::point& p,Step s){return create_neitral_item(step_t(s,p.x,p.y));}
	public:
		wsplayer_t& player;
		long long deep_wins_count;
		long long deep_fails_count;
		unsigned neitrals_min_deep;

		item_t(wsplayer_t& _player,const step_t& s);
		item_t(wsplayer_t& _player,const Gomoku::point& p,Step s);
		~item_t(){--nodes_count;}

		item_ptr get_next_step() const;
		item_ptr get_win_fail_step() const;
		unsigned get_chain_depth() const;

		void process(bool need_fill_neitrals,const item_t* parent_node=0);
		void process_deep_common();

		inline void add_win(const item_ptr& val){wins.add(val);}
		inline void add_fail(const item_ptr& val){fails.add(val);}
		inline const selected_wins_childs& get_wins() const{return wins;}
		inline const selected_fails_childs& get_fails() const{return fails;}
		inline const items_t& get_neitrals() const{return neitrals;}
        inline double get_win_rate() const{return static_cast<double>(deep_wins_count+1)/(deep_fails_count+1);}

		//next state from current state wins
		inline bool is_win() const{return !wins.empty();}
		inline bool is_fail() const{return !is_win() && !fails.empty() && neitrals.empty();}
		inline bool is_completed() const{return is_win() || is_fail();}
		void calculate_neitrals_min_deep();
	};

	class wide_item_t : public item_t
	{
	protected:
		void process(bool need_fill_neitrals);
		void process_deep_stored();
		virtual void process_neitrals(bool need_fill_neitrals,unsigned from=0,const item_t* parent_node=0);

		virtual item_ptr create_neitral_item(const step_t& s);
	public:
		wide_item_t(wsplayer_t& _player,const step_t& s) : item_t(_player,s){}
		wide_item_t(wsplayer_t& _player,const Gomoku::point& p,Step s) : item_t(_player,p,s){}

		void process_deep_common();
	};


	std::string print_chain(item_ptr root);
	void items2points(const items_t& items,points_t& res);
	void items2depth_npoints(const items_t& items,npoints_t& res);
} }//namespace Gomoku

#endif

