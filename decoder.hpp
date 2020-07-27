#pragma once

/*
 	 audio, video, decoder
 */
class decoder
{
private:
	void clear()
	{
		if(_avcontext)avcodec_free_context(&_avcontext);
		_avcontext = nullptr;
		_decode = nullptr;
	}
public:
	std::string name()
	{
		return av_type_string_map()(_avcontext->codec_id);
	}
	enum AVCodecID id()
	{
		return _avcontext->codec_id;
	}

	template <typename functor>
	int operator () ( avpacket_class &packet,
			functor &&_f,
			void *puser = nullptr)
	{
		int org_size = packet.raw()->size;
		unsigned char *org_ptr = packet.raw()->data;
		int ret = 0;
		int got = 0;
		int make_frame_number = 0;


		while(packet.raw()->size > 0)
		{
			ret = 0;
			got = 0;
			ret = _decode(_avcontext, _frame.raw(), &got, packet.raw());
			if(ret >= 0 &&
					got)
			{
				/*
				  	  put to virtual function
				 */
				int cn = packet.raw()->size;
				unsigned char *cp = packet.raw()->data;
				packet.raw()->size = org_size;
				packet.raw()->data = org_ptr;

				_f(packet, _frame, puser);
				_frame.unref();

				make_frame_number++;
				packet.raw()->size = cn;
				packet.raw()->data = cp;
			}
			if(ret < 0)
			{
				/*
				  	 throw the frame and break
				 */

				//_frame.unref();
				break;
			}

			packet.raw()->size -= ret;
			packet.raw()->data += ret;
		}
		packet.raw()->size = org_size;
		packet.raw()->data = org_ptr;
		return make_frame_number;
	}
	bool opentest(const avattr &attr,
			char const *codec)
	/*
	 	 test from codec string
	 */
	{
		clear();
		return decoder_open_test(attr, codec, nullptr, nullptr);
	}
	bool opentest(const avattr &attr,
			enum AVCodecID codec)
	/*
	 	 test from codec id
	 */
	{
		clear();
		return decoder_open_test(attr, nullptr, &codec, nullptr);
	}
	bool opentest(const AVCodecParameters *codec)
	/*
	 	 test from any codec parameter
	 */
	{
		clear();
		return decoder_open_test(avattr(), nullptr, nullptr, codec);
	}
	/*
	 	 create decoder by users parameter
	 */
	decoder()  :
			_decode(nullptr),
			_avcontext(nullptr),
			_frame(avframe_class())
	{
		/*
		 	 useful opentest or copy
		 */
	}
	decoder(const avattr &attr,
			char const *codec) :
					_decode(nullptr),
					_avcontext(nullptr),
					_frame(avframe_class())
	{
		DECLARE_THROW(!decoder_open_test(attr,
				codec,
				nullptr,
				nullptr),
				"can't alloc decoder");
	}
	/*
	 	 create decoder by users parameter
	 */
	decoder(const avattr &attr,
			enum AVCodecID codec) :
					_decode(nullptr),
					_avcontext(nullptr),
					_frame(avframe_class())
	{
		DECLARE_THROW(!decoder_open_test(attr,
				nullptr,
				&codec,
				nullptr),
				"can't alloc decoder");
	}
	/*
	 	 create decoder by codecs parameter
	 */
	decoder( const AVCodecParameters *codec) :_decode(nullptr),
			_avcontext(nullptr),
			_frame(avframe_class())
	{
		DECLARE_THROW(!decoder_open_test(avattr(),
				nullptr,
				nullptr,
				codec),
				"can't alloc decoder");
	}
	decoder(const decoder &rhs) :
		_decode(nullptr),
		_avcontext(nullptr),
		_frame(avframe_class())

	{
		avattr dump_attr;
		AVCodecParameters *codec_par = avcodec_parameters_alloc();
		DECLARE_THROW(!codec_par, "decoder create fail");

		DECLARE_THROW(0 < avcodec_parameters_from_context(codec_par, rhs._avcontext), "decoder create fail");

		DECLARE_THROW(!decoder_open_test(dump_attr/*when test by codec parameter, this attr parmeter is none used*/,
				nullptr,
				nullptr,
				codec_par),
				"can't alloc decoder");
		avcodec_parameters_free(&codec_par);
	}
	decoder(decoder &&rhs) :
		_decode(nullptr),
		_avcontext(nullptr),
		_frame(avframe_class())
	{
		clear();
		this->_avcontext = rhs._avcontext;
		this->_decode = rhs._decode;
		this->_frame = rhs._frame;
		rhs._avcontext = nullptr;
	}

	virtual ~decoder()
	{
		clear();
	}
	decoder &operator()(decoder &&rhs)
	{
		clear();
		this->_avcontext = rhs._avcontext;
		this->_decode = rhs._decode;
		this->_frame = rhs._frame;
		rhs._avcontext = nullptr;
		return *this;
	}
	decoder &operator()(const decoder &rhs)
	{
		clear();
		avattr dump_attr;
		AVCodecParameters *codec_par = avcodec_parameters_alloc();
		DECLARE_THROW(!codec_par, "decoder create fail");

		DECLARE_THROW(0 < avcodec_parameters_from_context(codec_par, rhs._avcontext), "decoder create fail");

		DECLARE_THROW(!decoder_open_test(dump_attr/*when test by codec parameter, this attr parmeter is none used*/,
				nullptr,
				nullptr,
				codec_par),
				"can't alloc decoder");
		avcodec_parameters_free(&codec_par);
		return *this;
	}

private:
	bool decoder_open_test(const avattr &attr,
			const char *ccodec,
			enum AVCodecID *ncodec,
			const AVCodecParameters *pcodec)
	{
		AVCodec *avcodec = nullptr;
		AVCodecParameters *temppar = nullptr;
		do
		{
			/*
			 	 	 invalid parameter check
			 */
			if(!ccodec &&
					!ncodec &&
					!pcodec)
			{
				break;
			}

			/*
			 	 	 find codec
			 */

			if(ccodec) 				avcodec = avcodec_find_decoder_by_name(ccodec);
			else if(ncodec) 	avcodec = avcodec_find_decoder(*ncodec);
			else if(pcodec)
			{
				temppar = avcodec_parameters_alloc();
				if(temppar)
				{
					if(avcodec_parameters_copy(temppar, pcodec) >= 0)
					{
						avcodec = avcodec_find_decoder(temppar->codec_id);
					}
				}
			}

			if(!avcodec)
			{
				/*
				 	 	 can't find codec
				 */
				break;
			}
			if(avcodec->type != AVMEDIA_TYPE_AUDIO &&
					avcodec->type != AVMEDIA_TYPE_VIDEO)
			{
				//TODO current support codec type  'audio' and 'video'
				break;
			}
			_avcontext = avcodec_alloc_context3(avcodec);
			if(!_avcontext)
			{
				break;
			}

			/*
			 	 filled context
			 */
			/*
			 	 if constructor from codec parameter
			 */
			if(temppar)
			{
				if(avcodec_parameters_to_context(_avcontext, temppar) < 0)
				{
					break;
				}

			}
			/*
			 	 if constructor from codec name
			 */
			else
			{


				if(avcodec->type == AVMEDIA_TYPE_AUDIO)
				{
					if(!attr.has_frame_audio())
					{
						break;
					}

					_avcontext->channels = attr.get_int(avattr_key::channel);
					_avcontext->sample_rate = attr.get_int(avattr_key::samplerate);
					_avcontext->sample_fmt = (enum AVSampleFormat)attr.get_int(avattr_key::pcm_format);;
				}
				else if(avcodec->type == AVMEDIA_TYPE_VIDEO)
				{
					if(!attr.has_frame_video())
					{
						break;
					}

					_avcontext->width = attr.get_int(avattr_key::width);
					_avcontext->height = attr.get_int(avattr_key::height);
					_avcontext->pix_fmt = (enum AVPixelFormat)attr.get_int(avattr_key::pixel_format);
					_avcontext->bit_rate = 400000;
				}
			}

			/*
			 	 	 now open context
			 */
			if(avcodec_open2(_avcontext, avcodec, NULL) < 0)
			{
				break;
			}


			if(_avcontext->codec_type == AVMEDIA_TYPE_AUDIO)
				_decode = avcodec_decode_audio4;
			else if(_avcontext->codec_type == AVMEDIA_TYPE_VIDEO)
				_decode = avcodec_decode_video2;


			return true;
		}while(0);

		if(temppar)
		{


			avcodec_parameters_free(&temppar);
		}
		if(_avcontext)avcodec_free_context(&_avcontext);
		_avcontext = nullptr;

		return false;

	}
	std::function<int(AVCodecContext *avctx, AVFrame *frame,
			                          int *got_frame_ptr, const AVPacket *avpkt)> _decode;
	AVCodecContext *_avcontext;
	avframe_class _frame;
};


