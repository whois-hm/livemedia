#pragma once
/*
 	 interface class AVPacket of FFMpeg
 */
class avpacket_class
{
	enum AVMediaType _type;
public:
	void settype(enum AVMediaType type)
	{
		_type = type;
	}
	enum AVMediaType gettype()
	{
		return _type;
	}
	avpacket_class( ) :
		_type(AVMEDIA_TYPE_UNKNOWN)
	{
		av_init_packet(&_pkt);
	}
	avpacket_class(   const AVPacket &pkt ) :
		_type(AVMEDIA_TYPE_UNKNOWN)
	{
		av_init_packet(&_pkt);

		av_packet_ref(&_pkt, &const_cast<AVPacket &>(pkt));
	}
	avpacket_class(   AVPacket &&pkt ) :
		_type(AVMEDIA_TYPE_UNKNOWN)
	{
		av_init_packet(&_pkt);

		av_packet_move_ref(&_pkt, &const_cast<AVPacket &>(pkt));
	}
	avpacket_class(   const avpacket_class &pkt ) :
		_type(pkt._type)
	{
		av_init_packet(&_pkt);
		av_packet_ref(&_pkt, &const_cast<AVPacket &>(pkt._pkt));
	}
	avpacket_class(   avpacket_class &&pkt ) :
		_type(pkt._type)
	{
		av_init_packet(&_pkt);
		av_packet_move_ref(&_pkt, &const_cast<AVPacket &>(pkt._pkt));
	}
	virtual ~avpacket_class()
	{
		unref();
	}
	avpacket_class &operator = (const  avpacket_class &pkt)
	{
		unref();
		av_packet_ref(&_pkt, &const_cast<AVPacket &>(pkt._pkt));
		_type = pkt._type;
		return *this;
	}
	avpacket_class &operator = (avpacket_class &&pkt)
	{
		unref();
		av_packet_move_ref(&_pkt, &const_cast<AVPacket &>(pkt._pkt));
		_type = pkt._type;
		return *this;
	}
	avpacket_class &operator = (const  AVPacket &pkt)
	{
		unref();
		av_packet_ref(&_pkt, &const_cast<AVPacket &>(pkt));
		return *this;
	}
	avpacket_class &operator = (AVPacket &&pkt)
	{
		unref();
		av_packet_move_ref(&_pkt, &const_cast<AVPacket &>(pkt));
		return *this;
	}
	AVPacket *raw()
	{
		return &_pkt;
	}
	void unref()
	{
		av_packet_unref(&_pkt);
	}
	avpacket_class move()
	{
		avpacket_class dump;
		av_packet_move_ref(&dump._pkt, &_pkt);

		return dump;
	}
	void fromdata(uint8_t *data, int size, enum AVMediaType t)
	{
		fromdata(data, size);
		settype(t);
	}
	void fromdata(uint8_t *data, int size)
	{
		if(data && size > 0)
		{
			unref();
			av_new_packet(&_pkt, size);
			memcpy(_pkt.data, data, size);
			//av_packet_from_data(&_pkt, data, size);
		}
	}
protected:
	AVPacket _pkt;

};

	class rtsppacket : public avpacket_class
	{
		/*
		 	 redefiniton packet class
		 */
	public:
		/*
		 	 saved live5 values
		 */
		unsigned _truncatedbyte;
		struct timeval _presentationtime;
		unsigned _durationinmicroseconds;
		double _normalplaytime;
	public:
		rtsppacket(unsigned char *data,  unsigned size, unsigned truncatedbyte,
				  struct timeval presentationtime, unsigned durationinmicroseconds,
				  double normalplaytime) :
					  avpacket_class(),
					  _truncatedbyte (truncatedbyte),
					  _presentationtime(presentationtime),
					  _durationinmicroseconds(durationinmicroseconds),
					  _normalplaytime(normalplaytime)
		{
			DECLARE_THROW(av_new_packet(&_pkt, size), "can't av_new_packet");
			memcpy(_pkt.data, data, size);
		}
		virtual ~rtsppacket()
		{ }
		rtsppacket(rtsppacket const &rhs) :
			avpacket_class(dynamic_cast<avpacket_class const &>(rhs)),
			_truncatedbyte(rhs._truncatedbyte),
			_presentationtime(rhs._presentationtime),
			_durationinmicroseconds(rhs._durationinmicroseconds),
			_normalplaytime(rhs._normalplaytime){ }
		rtsppacket const &operator = (rtsppacket const &rhs)
		{
			avpacket_class::operator =(dynamic_cast<avpacket_class const &>(rhs));
			_truncatedbyte = rhs._truncatedbyte;
			_presentationtime = rhs._presentationtime;
			_durationinmicroseconds = rhs._durationinmicroseconds;
			_normalplaytime = rhs._normalplaytime;
			return *this;
		}
		rtsppacket *clone() const
		{
			return new rtsppacket(*this);
		}
	};
