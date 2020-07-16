#pragma once

template <typename types,/*filter parameter type*/
typename T/*saving any pointer type*/
>
class filter
{
	bool _bfiltering;
	T *_ptr;
public:
	bool isenable()
	{
		return _bfiltering;
	}
	T *ptr(){ return _ptr; }
	void enable()
	{ _bfiltering = true; }
	void disable()
	{ _bfiltering = false; }
	void operator << (types &pf)
	{ if(_bfiltering)operator >> (pf); }
	void operator << (types &&pf)
	{ operator << (pf); }

protected:
	filter(T *ptr) : _bfiltering(false) ,_ptr(ptr){ }
	filter() : _bfiltering(false) ,_ptr(nullptr){ }
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
