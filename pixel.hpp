#pragma once

/*
 	 video raw data class
 */
class pixel :
		public raw_media_data
{
private:
	std::tuple<int, int, enum AVPixelFormat> _val;
public:

	pixel() : raw_media_data(AVMEDIA_TYPE_VIDEO, 0),
			_val(std::make_tuple(0, 0, AV_PIX_FMT_NONE)){}
	pixel(raw_media_data::type_size s) : raw_media_data(AVMEDIA_TYPE_VIDEO, s),
			_val(std::make_tuple(0, 0, AV_PIX_FMT_NONE)){}
	pixel(const pixel &rhs) :
		raw_media_data(dynamic_cast<const raw_media_data &>(rhs)),
		_val(rhs._val){}
	pixel(pixel &&rhs) :
			raw_media_data(std::move(rhs)),
			_val(rhs._val){}
	virtual ~pixel(){}
	pixel &operator =
			(const pixel &rhs)
	{
		raw_media_data::operator =
				(dynamic_cast<const raw_media_data &>(rhs));
		_val = rhs._val;
		return *this;
	}
	pixel &operator =
			(pixel &&rhs)
	{
		raw_media_data::operator =
				(std::move(rhs));
		_val = rhs._val;
		return *this;
	}
	pixel &operator =  ( const std::pair<raw_media_data::type_ptr,
			raw_media_data::type_size> &ds)
	{
		raw_media_data::operator =(ds);
		return *this;
	}
	pixel &operator =  (raw_media_data::type_pts p)
	{
		raw_media_data::operator =(p);
		return *this;
	}
	pixel &operator =  (raw_media_data::type_media t)
	{
		raw_media_data::operator =(AVMEDIA_TYPE_VIDEO);
		return *this;
	}
	pixel &operator =  (const std::tuple<raw_media_data::type_ptr,
			raw_media_data::type_size,
			raw_media_data::type_media,
			raw_media_data::type_pts>& v)
	{
		(*this) =  std::make_pair(std::get<0>(v), std::get<1>(v));
		(*this) =  std::get<2>(v);
		(*this) =  std::get<3>(v);
		return *this;
	}

	void set(int width, int
			height,
			enum AVPixelFormat format)
	{
		_val = std::make_tuple(width, height, format);
	}
	void set(std::tuple<int,
			int,
			enum AVPixelFormat> &&val)
	{
		_val = val;
	}

	int width() const
	{ return std::get<0>(_val); }
	int height() const
	{ return std::get<1>(_val); }
	enum AVPixelFormat format() const
	{ return std::get<2>(_val); }
	virtual operator bool () const
	{
		return  std::get<0>(_val) > 0 &&
				std::get<1>(_val) > 0 &&
				std::get<2>(_val) != AV_PIX_FMT_NONE &&
				raw_media_data::operator bool();
	}
};



