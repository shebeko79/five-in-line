#ifndef symmetryH
#define symmetryH
#include "field.h"
#include <memory>

namespace Gomoku { namespace Symmetry
{
	struct itrans;
	using itrans_ptr = std::shared_ptr<itrans>; 

	struct tr_agregate;

	
	tr_agregate minimal(const steps_t& steps);

	template<typename T>
	inline void transform(std::vector<T>& points, itrans& tr)
	{
		for(auto& p : points)
			tr.transform(p);
	}


	struct itrans
	{
		virtual ~itrans() {}
		virtual void transform(point& p) const = 0;
		virtual itrans_ptr invert() const = 0;
	};

	struct tr_agregate : public itrans
	{
		std::vector<itrans_ptr> sub;
		void transform(point& p) const override;
		itrans_ptr invert() const override;
	};
	
	struct tr_flip_x : public itrans
	{
		void transform(point& p) const override {p.x = -p.x;}
		itrans_ptr invert() const override {return std::make_shared<tr_flip_x>();}
	};

	struct tr_flip_y : public itrans
	{
		void transform(point& p) const override {p.y = -p.y;}
		itrans_ptr invert() const override {return std::make_shared<tr_flip_y>();}
	};

	struct tr_translate : public itrans
	{
		const point dp;
		tr_translate(const point& _dp) : dp(_dp) {}
		void transform(point& p) const override {p += dp;}
		itrans_ptr invert() const override {return std::make_shared<tr_translate>(point(-dp.x,-dp.y));}
	};

	struct tr_90_cw : public itrans
	{
		void transform(point& p) const override {p = point(-p.y,p.x);}
		itrans_ptr invert() const override;
	};

	struct tr_90_ccw : public itrans
	{
		void transform(point& p) const override {p = point(p.y,-p.x);}
		itrans_ptr invert() const override {return std::make_shared<tr_90_cw>();}
	};

	class binary_field
	{
	public:
		binary_field(const steps_t& steps, tr_agregate& tr);
		bool operator<(const binary_field& rhs) const;
	
	private:
		using chunk = unsigned long long;
		std::vector<chunk> chunks;
		unsigned row_width = 1;
	};


} }//namespace

#endif
