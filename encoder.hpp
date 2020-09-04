#pragma once


class encoder
{
	AVCodecContext *_avcontext;
	uint64_t _pts;
	avpacket_class _pkt;	
	std::function<int(AVCodecContext *,
		AVPacket *,
		AVFrame *, 
		int *)>
		encode;
		
private:
	void clear(AVCodecContext *&target)
	{
		if(target)
		{
			avcodec_free_context(&target);
			target = nullptr;
		}
	}
	bool encoder_open_test( const avattr &attr, AVCodecContext *&target)
	{		
		
		enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
		AVCodec *codec = nullptr;
		AVRational fps;
		clear(target);
		
		if(!attr.notfound(avattr::frame_video) &&
			!attr.notfound(avattr::width) &&
			!attr.notfound(avattr::height) &&
			!attr.notfound(avattr::pixel_format)&&
			!attr.notfound(avattr::fps)&&
			!attr.notfound(avattr::bitrate)&&
			!attr.notfound(avattr::gop)&&
			!attr.notfound(avattr::max_bframe) &&
			!attr.notfound(avattr::video_encoderid))
		{
			type = AVMEDIA_TYPE_VIDEO;
		}
		/*
			todo AVMEDIA_TYPE_AUDIO
		*/
		if(type == AVMEDIA_TYPE_UNKNOWN)
		{
			return false;
		}

		do
		{

			if(type == AVMEDIA_TYPE_VIDEO)
			{
				codec = avcodec_find_encoder((enum AVCodecID)attr.get<avattr::avattr_type_int>(avattr::video_encoderid));
				if(!codec)
				{
					break;
				}
				target = avcodec_alloc_context3(codec);
				if(!target)
				{
					break;
				}

					fps.num = 1;
					fps.den = attr.get<avattr::avattr_type_int>(avattr::fps);
					target->width = attr.get<avattr::avattr_type_int>(avattr::width);
					target->height = attr.get<avattr::avattr_type_int>(avattr::height);
					target->bit_rate = attr.get<avattr::avattr_type_int>(avattr::bitrate);
					target->time_base = fps;
					target->gop_size = attr.get<avattr::avattr_type_int>(avattr::gop);
					target->max_b_frames = attr.get<avattr::avattr_type_int>(avattr::max_bframe);
					target->pix_fmt = (enum AVPixelFormat)attr.get<avattr::avattr_type_int>(avattr::pixel_format);
					encode = avcodec_encode_video2;
				

				if(avcodec_open2(target, codec, nullptr))
				{
					break;
				}
			}
			return true;
		}while(0);
		clear(target);
		return false;
	}
public:
	encoder() : 
		_avcontext(nullptr),
		_pts(0),
		_pkt(avpacket_class())
	{
		/*for open test user*/
	}
	encoder(  const avattr &attr) : 
		_avcontext(nullptr),
		_pts(0),
		_pkt(avpacket_class())
	{
		DECLARE_THROW(!encoder_open_test(attr, _avcontext),
				"can't alloc encoder");
	}
	int parameter_open_test( const avattr &attr)
	{
		AVCodecContext *test_avcontext = nullptr;
		bool res = false;
	    res = encoder_open_test(attr, test_avcontext);
		clear(test_avcontext);
		return res ? 0 : -1;
	}
	AVCodecContext *raw()
	{
		return _avcontext;
	}
	virtual ~encoder()
	{
		clear(_avcontext);
	}

	template <typename functor>
	int encoding( avframe_class &frm,
			functor &&_f,
			void *puser = nullptr)
	{
		bool has_encoded = false;
		if(!_avcontext)
		{
			return -1;
		}
		/*
			none check valid parameter in frm.raw(), return error perhaps in encode funtion
		*/
		int got = 0;
		int encoded = 0;

		if(_pts <= 0) _pts = 1;

		if(frm.raw()->pts == AV_NOPTS_VALUE)
		{
			frm.raw()->pts = _pts++;
		}


		encoded = encode(_avcontext, _pkt.raw(), frm.raw(), &got);

		if(encoded >=0&&
				got)
		{
			has_encoded = true;
			_f(frm, _pkt, puser);
			_pkt.unref();
		}

		return has_encoded ? 1 : 0;
	}

};
