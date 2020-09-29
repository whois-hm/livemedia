#pragma once

namespace raw_media_data_class
{
typedef uint8_t type_data;
typedef std::shared_ptr<type_data> type_shared_ptr;
class raw_media_data :
		public type_shared_ptr
{
public:
	typedef 		type_data* 	type_ptr;
	typedef 		unsigned 	type_size;
	typedef 		enum AVMediaType type_media;
	typedef 		double 		type_pts;
protected:
	type_size _s;/*size of data*/
	type_media _t; /*type of this media*/
	type_pts _p;/*pts of this data*/
public:
	static type_ptr make_new(unsigned s,
			type_ptr d = nullptr/*if not null copy that, other dump new only*/)
	{
		type_ptr _n_d = nullptr;
		do
		{
			if(s <= 0) { break; }

			_n_d = new type_data[s];
			if(!d) { break; }

			memcpy(_n_d, d, s);
		}while(0);

		return _n_d;
	}
	static void delete_new(type_ptr d)
	{
		_delete_array<type_data>()(d);
	}
	raw_media_data() : /*create templete*/
		type_shared_ptr(nullptr, _delete_array<type_data>()),
		_s(0),
		_t(AVMEDIA_TYPE_UNKNOWN),
		_p(0.0){}
	raw_media_data(const raw_media_data &rhs) : /*copy const*/
		type_shared_ptr(dynamic_cast<const type_shared_ptr&>(rhs)),
		_s(rhs._s),
		_t(rhs._t),
		_p(rhs._p){}
	raw_media_data(raw_media_data &&rhs) : /*move*/
		type_shared_ptr(std::move(rhs)),
		_s(rhs._s),
		_t(rhs._t),
		_p(rhs._p){}
	raw_media_data(enum AVMediaType t,
			unsigned s,
			double p = 0.0) :	/*create empty data*/
				type_shared_ptr(raw_media_data::make_new(s),
		_delete_array<type_data>()),
		_s(s),
		_t(s ? t : AVMEDIA_TYPE_UNKNOWN/*if data was not create */),
		_p(p) { }
	raw_media_data(enum AVMediaType t,
			uint8_t *d,
			unsigned s,
			double p = 0.0) :	/*create raw data*/
				type_shared_ptr(raw_media_data::make_new(s, d),
		_delete_array<type_data>()),
		_s(s),
		_t(s ? t : AVMEDIA_TYPE_UNKNOWN/*if data was not create */),
		_p(p) { }
	template <typename T>
	T take()
	{
		T t;
		return t;
	}
	virtual operator bool()		const
	{
		return 	(_s) &&
					(_t != AVMEDIA_TYPE_UNKNOWN) &&
					(std::__shared_ptr<type_data>::
							operator bool());
	}
	raw_media_data &operator =  ( const std::pair<type_ptr,
			type_size> &ds)
	{
		type_shared_ptr::reset();
		type_shared_ptr::reset(raw_media_data::make_new(ds.second, ds.first),
				_delete_array<type_data>());
		_s = ds.second;
		return *this;
	}
	raw_media_data &operator =  (type_pts p)
	{
		_p = p;
		return *this;
	}
	raw_media_data &operator =  (type_media t)
	{
		_t = t;
		return *this;
	}
	raw_media_data &operator =  (const std::tuple<type_ptr,
			type_size,
			type_media,
			type_pts>& v)
	{
		(*this) = (std::make_pair(std::get<0>(v), std::get<1>(v)));
		(*this) = std::get<2>(v);
		(*this) = std::get<3>(v);
		return *this;
	}
	raw_media_data &operator =  (const raw_media_data &rhs)
	{
		type_shared_ptr::reset();
		type_shared_ptr::operator =(rhs);
		_s = rhs._s;
		_t = rhs._t;
		_p = rhs._p;
		return *this;
	}
	raw_media_data &operator = (raw_media_data &&rhs)
	{
		type_shared_ptr::reset();
		type_shared_ptr::operator =
				(std::move(dynamic_cast<type_shared_ptr &>(rhs)));
		_s = rhs._s;
		_t = rhs._t;
		_p = rhs._p;
		rhs._s = 0;
		rhs._t = AVMEDIA_TYPE_UNKNOWN;
		rhs._p = 0.0;
		return *this;
	}
	/* no use parent this */
	void reset() noexcept 	= delete;
	void swap() noexcept 		= delete;
	void get() 					= delete;
};

template<> inline  raw_media_data::type_ptr raw_media_data::take()		{ return type_shared_ptr::get(); }
template<> inline  raw_media_data::type_size raw_media_data::take()		{ return _s; 	}
template<> inline  raw_media_data::type_media raw_media_data::take()		{ return _t; 	}
template<> inline  raw_media_data::type_pts raw_media_data::take()		{ return _p; 	}
}
using namespace raw_media_data_class;
