#include "symmetry.h"

namespace Gomoku { namespace Symmetry
{
	void tr_agregate::transform(point& p) const
	{
		for(const auto& t : sub)
			t->transform(p);
	}

	itrans_ptr tr_agregate::invert() const
	{
		auto ret = std::make_shared<tr_agregate>();
		ret->sub.resize(sub.size());

		for(size_t i = 0;i<sub.size();i++)
			ret->sub[sub.size()-1-i]=sub[i]->invert();

		return ret;
	}

	itrans_ptr tr_90_cw::invert() const
	{
		return std::make_shared<tr_90_ccw>();
	}

	binary_field::binary_field(const steps_t& steps, tr_agregate& tr)
	{
		if(steps.empty())
			return;

		rect bbox=rect_inverse_infinity();

		for (step_t p : steps)
		{
			tr.transform(p);
			bbox+=p;
		}

		if (bbox.x1 != 0 || bbox.y1 != 0)
			tr.sub.push_back(std::make_shared<tr_translate>(point(-bbox.x1,-bbox.y1)));

		row_width = ((bbox.width()+1)+63)/64;

		chunks.resize(row_width*(bbox.height()+1),0);

		for (step_t p : steps)
		{
			tr.transform(p);
			unsigned offset = row_width*p.y+p.x/64;
			chunk& ck = chunks[offset];

			chunk v = p.step==st_krestik? 1:2;
			ck |= v<<(p.x%64);
		}
	}

	bool binary_field::operator<(const binary_field& rhs) const
	{
		size_t ha = chunks.size()/row_width;
		size_t hb = rhs.chunks.size()/rhs.row_width;
		
		size_t h = std::min(ha, hb);
		size_t w = std::max(row_width, rhs.row_width);

		for (size_t y = 0; y < h; y++)
		{
			auto ra = chunks.begin() + row_width*y;
			auto rb = rhs.chunks.begin() + rhs.row_width*y;

			for (size_t x = 0; x < w; x++)
			{
				chunk ca = x<row_width? *(ra+x) : 0;
				chunk cb = x<rhs.row_width? *(rb+x) : 0;

				if(ca!=cb)
					return ca<cb;
			}
		}

		return ha < hb;
	}

	tr_agregate flip_x(const steps_t& steps, const tr_agregate& base_tr)
	{
		tr_agregate tr_a = base_tr;
		
		tr_agregate tr_b = base_tr;
		tr_b.sub.push_back(std::make_shared<tr_flip_x>());

		binary_field ba(steps,tr_a);
		binary_field bb(steps,tr_b);
		
		return bb<ba? tr_b : tr_a;
	}

	tr_agregate flip_y(const steps_t& steps, const tr_agregate& base_tr)
	{
		tr_agregate tr_a = flip_x(steps, base_tr);
		
		tr_agregate tr_b = base_tr;
		tr_b.sub.push_back(std::make_shared<tr_flip_y>());

		tr_b = flip_x(steps, tr_b);

		binary_field ba(steps,tr_a);
		binary_field bb(steps,tr_b);
		
		return bb<ba? tr_b : tr_a;
	}

	tr_agregate minimal(const steps_t& steps)
	{
		tr_agregate tr_a;
		
		tr_agregate tr_b;
		tr_b.sub.push_back(std::make_shared<tr_90_cw>());

		tr_a = flip_y(steps, tr_a);
		tr_b = flip_y(steps, tr_b);

		binary_field ba(steps,tr_a);
		binary_field bb(steps,tr_b);
		
		return bb<ba? tr_b : tr_a;
	}

} }//namespace
