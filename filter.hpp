#pragma once

template <typename types,/*filter parameter type*/
typename T/*saving any pointer type*/
>
class filter
{
	bool _bfiltering;/*flag for using filter*/
	std::shared_ptr<T> _ptr;
public:
	bool isenable() const
	/*check enable flag*/
	{
		return _bfiltering;
	}
	T *ptr()/*get the user ptr*/
	{ return _ptr.get(); }
	void enable()
	{ _bfiltering = true; }
	void disable()
	{ _bfiltering = false; }
	void operator << (types &pf)
	{ if(_bfiltering)operator >> (pf); }
	void operator << (types &&pf)
	{ operator << (pf); }

protected:
	filter(T *ptr) : _bfiltering(false) ,
		_ptr(ptr, _delete_ref<T>()) { }
	filter() : _bfiltering(false),
		_ptr(nullptr, _delete_ref<T>()) { }
	filter(const filter &rhs) : _bfiltering(rhs._bfiltering),
			_ptr(rhs.ptr()){}
	filter(filter &&rhs) : _bfiltering(rhs._bfiltering),
			_ptr(rhs.ptr()){rhs._bfiltering = false; rhs._ptr = nullptr;}
	virtual ~filter(){}
	virtual void operator >>(types &pf)
	{ }
};

/*if no use type 'T'*/
template <typename types>
class filter_default :
		public filter<types, void>
{
public:
	filter_default() :
		filter<types, void>(){}
	virtual ~filter_default(){}
};
