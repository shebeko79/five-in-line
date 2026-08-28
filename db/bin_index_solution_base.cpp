#include "bin_index_solution_base.h"
#include "../algo/symmetry.h"

namespace Gomoku
{

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


}//namespace
