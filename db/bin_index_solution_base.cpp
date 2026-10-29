#include "bin_index_solution_base.h"
#include "../algo/symmetry.h"

namespace fs=std::filesystem;

namespace Gomoku
{

bin_index_solution_base_t::bin_index_solution_base_t(const std::string& base_dir) : 
	indexes(base_dir,3),
	marks_filename((fs::path(base_dir)/"inprogress.bin").string())
{
}

bin_index_solution_base_t::~bin_index_solution_base_t()
{
    try
    {
        save_marks();
    }
	catch(std::exception& e)
	{
		ObjectProgress::log_generator lg(true);
		lg<<__FILE__<<": "<<__LINE__<<" std::exception: "<<e.what();
	}
	catch(...)
	{
		ObjectProgress::log_generator lg(true);
		lg<<__FILE__<<": "<<__LINE__<<" unknown exception";
	}
}

bool bin_index_solution_base_t::get(sol_state_t& res) const
{
	ibin_index_t* idx=indexes.find_index(res.key.size());
    if(!idx)return false;

	steps_t old_key=res.key;
	steps_t key=res.key;
	
	auto tr = Symmetry::minimal(key);
	Symmetry::transform(key, tr);
	sort_steps(key);

	data_t bin_key;
	points2bin(key,bin_key);

	data_t bin;
	if(!idx->get(bin_key,bin))return false;

	res.unpack(bin);

	auto inv_tr = tr.invert();
	res.key = old_key;
	Symmetry::transform(res.neutrals, *inv_tr);
	Symmetry::transform(res.solved_wins, *inv_tr);
	Symmetry::transform(res.solved_fails, *inv_tr);
	Symmetry::transform(res.tree_wins, *inv_tr);
	Symmetry::transform(res.tree_fails, *inv_tr);

	return true;
}

void bin_index_solution_base_t::set(const sol_state_t& _val)
{
	sol_state_t val=_val;
	auto tr = Symmetry::minimal(val.key);
	Symmetry::transform(val.key, tr);
	sort_steps(val.key);
	Symmetry::transform(val.neutrals, tr);
	Symmetry::transform(val.solved_wins, tr);
	Symmetry::transform(val.solved_fails, tr);
	Symmetry::transform(val.tree_wins, tr);
	Symmetry::transform(val.tree_fails, tr);


	ibin_index_t& idx=indexes.get_index(val.key.size());

	data_t bin_key;
	points2bin(val.key,bin_key);

	data_t bin;
	val.pack(bin);

	idx.set(bin_key,bin);
}

steps_t bin_index_solution_base_t::get_root_key() const
{
	unsigned level=indexes.get_root_level();
	if(level==0)return steps_t();
	ibin_index_t& idx=indexes.get_index(level);

	data_t bin_key,bin_val;
	if(!idx.first(bin_key,bin_val))return steps_t();

	steps_t ret;
	bin2points(bin_key,ret);
	
	return ret;
}

void bin_index_solution_base_t::width_first_search_from_bottom_to_top(sol_state_width_pr& pr)
{
    for(size_t l=retreive_max_level();l>0;l--)
    {
        ibin_index_t& idx=indexes.get_index(l);
        width_first_search_from_bottom_to_top(pr,idx);
    }
}

void bin_index_solution_base_t::width_first_search_from_bottom_to_top(sol_state_width_pr& pr,ibin_index_t& idx)
{
	data_t bin_key,bin_data;

    for(bool r=idx.first(bin_key,bin_data);r;r=idx.next(bin_key,bin_data))
    {
        if(pr.is_canceled())
            throw e_cancel();

        sol_state_t st;
        st.unpack(bin_data);

        if(pr.on_enter_node(st))
        {
            set(st);
        }
    }
}

size_t bin_index_solution_base_t::retreive_max_level() const
{
    size_t ret=0;

    while(indexes.find_index(ret+1)!=0){ret++;}
    return ret;
}

bool bin_index_solution_base_t::get_mark(sol_mark_t& mrk) const
{
	load_marks();
	
	Symmetry::normalize(mrk.key);
	auto it = marks.find(mrk);
	if(it == marks.end())
		return false;

	mrk = *it;
	return true;
}

void bin_index_solution_base_t::mark(const sol_mark_t& _mrk)
{
	load_marks();
	
	sol_mark_t mrk(_mrk);
	Symmetry::normalize(mrk.key);
	marks.insert(mrk);
	mark_state = ms_dirty;
}

void bin_index_solution_base_t::unmark(const sol_mark_t& _mrk)
{
	load_marks();
	
	sol_mark_t mrk(_mrk);
	Symmetry::normalize(mrk.key);
	marks.erase(mrk);
	mark_state = ms_dirty;
}


void bin_index_solution_base_t::visit_marks(mark_visitor_pr& pr)
{
	load_marks();

	for (auto it = marks.begin(); it != marks.end();)
	{
		if(pr.is_canceled())
			throw e_cancel();

		if (pr.on_enter_node(*it))
		{
			it = marks.erase(it);
			mark_state = ms_dirty;
		}
		else
			++it;
	}
}

void bin_index_solution_base_t::load_marks() const
{
	if(mark_state != ms_unloaded)
		return;
	
	data_t bin;
	load_file(marks_filename, bin);

	size_t from = 0;
	unpack(marks, bin, from);
	mark_state = ms_loaded;
}

void bin_index_solution_base_t::save_marks()
{
	if(mark_state != ms_dirty)
		return;

	data_t bin;
	pack(marks, bin);
	save_file(marks_filename, bin);

	mark_state = ms_loaded;
}

bool bin_index_solution_base_t::mark_pr::operator()(const sol_mark_t& a, const sol_mark_t& b) const
{
	return std::lexicographical_compare(a.key.begin(),a.key.end(),b.key.begin(),b.key.end(),pr);
}

}//namespace
