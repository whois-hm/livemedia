#pragma once

/*
 	 video frame
 */

class pixelframe :
		public avframe_class_type
		<pixel>
{
private:
	virtual void field_attr_value(pixel &t)
	{
		t.set(std::tuple<int, int, enum AVPixelFormat>(
				raw()->width,
				raw()->height,
				(enum AVPixelFormat)raw()->format));
	}
	void throw_if_not_video()
	{DECLARE_THROW(type() != AVMEDIA_TYPE_VIDEO,
			"pixel frame type no match");}
public:
	pixelframe() = delete;
	pixelframe(pixel &rhs) :
		avframe_class_type<pixel>()
	{
		if(rhs)
		{

			raw()->width = rhs.width();
			raw()->height = rhs.height();
			raw()->format = rhs.format();
			av_frame_get_buffer(raw(), 0);

			av_image_fill_arrays(raw()->data,
					raw()->linesize,
					rhs.take<uint8_t *>(),
					rhs.format(),
					rhs.width(),
					rhs.height(),
					1);
		}
	}

	/*
		setup self
	*/
	pixelframe (unsigned width, 
		unsigned height,
		enum AVPixelFormat fmt,
		void *p) : 
		avframe_class_type<pixel>()
	{
	DECLARE_THROW(width <= 0, "pixelframe invalid width");
	DECLARE_THROW(height <= 0, "pixelframe invalid height");
	DECLARE_THROW(fmt < 0, "pixelframe invalid format");
	DECLARE_THROW(!p, "pixelframe invalid data pointer");
	
	
		raw()->width = width;
		raw()->height = height;
		raw()->format = fmt;
		av_frame_get_buffer(raw(), 0);
		
		av_image_fill_arrays(raw()->data,
				raw()->linesize,
				(uint8_t *)p,
				fmt,
				width,
				height,
				1);
	}
	pixelframe(unsigned width,
			unsigned height,
			enum AVPixelFormat fmt):
				avframe_class_type<pixel>()
	{
			DECLARE_THROW(width <= 0, "pixelframe invalid width");
			DECLARE_THROW(height <= 0, "pixelframe invalid height");
			DECLARE_THROW(fmt < 0, "pixelframe invalid format");


			raw()->width = width;
			raw()->height = height;
			raw()->format = fmt;
			av_frame_get_buffer(raw(), 0);

			uint8_t black[len()];
			memset(black, 0, len());
			av_image_fill_arrays(raw()->data,
					raw()->linesize,
					black,
					fmt,
					width,
					height,
					1);
	}

	pixelframe(const  AVFrame &frm) :
		avframe_class_type(frm)
	{
		throw_if_not_video();
	}
	pixelframe(const pixelframe &_class) :
		avframe_class_type(dynamic_cast<const avframe_class_type &>(_class))
	{
		throw_if_not_video();
	}
	virtual ~pixelframe()
	{

	}

	pixelframe &operator = (const  AVFrame &frm)
	{
		avframe_class_type::operator =(frm);
		throw_if_not_video();
		return *this;
	}
	pixelframe &operator = (const pixelframe &_class)
	{
		avframe_class_type::operator =
				(dynamic_cast<const avframe_class_type &>(_class));

		return *this;
	}
	int width()
	{
		return raw()->width;
	}
	int height()
	{
		return raw()->height;
	}
	enum AVPixelFormat format()
	{
		return (enum AVPixelFormat)raw()->format;
	}
	virtual int len()
	{
		enum AVPixelFormat fmt= (enum AVPixelFormat)raw()->format;
		int width = raw()->width;
		int height = raw()->height;
		/*
			 fill in the AVPicture fields, always assume a linesize alignment of 1.
		 */
		int align = 1;
		return av_image_get_buffer_size(fmt,
				width,
				height,
				align);
	}
	virtual void data_copy(uint8_t *ptr, int length)
	{
		DECLARE_THROW(!ptr || length < len(), "pixelframe invalid parameter");

		av_image_copy_to_buffer(ptr,
				length,
							(uint8_t **)raw()->data,
							raw()->linesize,
							(enum AVPixelFormat)raw()->format,
							raw()->width,
							raw()->height,
							1) ;
	}
	virtual pixel data_alloc()
	{

		int length = len();

		if(length > 0)
		{
			pixel d(length);

			av_image_copy_to_buffer(d.take<raw_media_data::type_ptr>(),
					length,
					(uint8_t **)raw()->data,
					raw()->linesize,
					(enum AVPixelFormat)raw()->format,
					raw()->width,
					raw()->height,
					1) ;
			return d;
		}
		return pixel();
	}
};

class pixelframe_presentationtime : public pixelframe
{
protected:
	double _pts;
	AVRational _rational;
public:
	pixelframe_presentationtime(const pixelframe_presentationtime &f):pixelframe(dynamic_cast<const pixelframe &>(f)) {
		_pts = f._pts;
		_rational = f._rational;
		 }
	pixelframe_presentationtime(const pixelframe &f,
			const AVRational rational) : pixelframe(f),
					_pts(0.0),
					_rational(rational){}
	pixelframe_presentationtime (unsigned width,
		unsigned height,
		enum AVPixelFormat fmt,
		void *p, double pts = 0.0) :
			pixelframe(width, height, fmt, p),
			_pts(pts) { }
	pixelframe_presentationtime (unsigned width,
			unsigned height,
			enum AVPixelFormat fmt, double pts = 0.0) :
				pixelframe(width, height, fmt),
				_pts(pts) { }
	virtual ~pixelframe_presentationtime(){}
	double operator()()
	{
		if(_pts != 0.0)
		{
			/*return cache for speed*/
			return _pts;
		}
		/*
		 	 first. calc pts from rational
		 */
       if(raw()->pkt_dts != AV_NOPTS_VALUE)
        {
            _pts = av_frame_get_best_effort_timestamp(raw());
            if(_pts != AV_NOPTS_VALUE)
            {
                _pts = av_q2d(_rational) * _pts;
            }
        }
       return _pts;
	}
};
