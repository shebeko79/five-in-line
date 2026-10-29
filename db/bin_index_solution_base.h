#ifndef bin_index_solution_baseH
#define bin_index_solution_baseH
#include <set>
#include "solution_tree.h"
#include "bin_index.h"

namespace Gomoku
{
	class bin_index_solution_base_t :  public isolution_tree_base_t
	{
		struct mark_pr
		{
			less_point_pr pr;

			bool operator()(const sol_mark_t& a, const sol_mark_t& b) const;
		};

		using sol_marks_t = std::set<sol_mark_t, mark_pr>;
	
		enum MarkState
		{
			ms_unloaded,
			ms_loaded,
			ms_dirty
		};

	private:
		mutable bin_indexes_t indexes;
		mutable sol_marks_t marks;
		mutable MarkState mark_state = ms_unloaded;
		std::string marks_filename;

        void width_first_search_from_bottom_to_top(sol_state_width_pr& pr,ibin_index_t& idx);
        size_t retreive_max_level() const;

		void load_marks() const;
		void save_marks();
	public:
		bin_index_solution_base_t(const std::string& base_dir);
		~bin_index_solution_base_t();

		bool get(sol_state_t& res) const override;
		void set(const sol_state_t& val) override;
		steps_t get_root_key() const override;
        void width_first_search_from_bottom_to_top(sol_state_width_pr& pr) override;

		bool get_mark(sol_mark_t& mrk) const override;
		void mark(const sol_mark_t& mrk) override;
		void visit_marks(mark_visitor_pr& pr) override;
	};

}//namespace

#endif
