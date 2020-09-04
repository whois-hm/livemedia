#pragma once


class live5livemediapp_serversession_source
{
protected:
	unsigned _unique_id;
	unsigned _refcount;/*subsession reference count*/
	memio _bank[AVMEDIA_TYPE_NB];

protected:

	void bank_in(enum AVMediaType type, unsigned char *from, unsigned size)
	{
		_bank[type].write(from, size);
	}
	int bank_drain(enum AVMediaType type, unsigned char *to, unsigned size)
	{
		return _bank[type].read(to, size);
	}


	live5livemediapp_serversession_source() :
		_unique_id(0),
		_refcount(0) {}




public:
	virtual ~live5livemediapp_serversession_source()
	{
	}
	virtual int doGetNextFrame(enum AVMediaType type,
				unsigned char*to,
				unsigned size,
				unsigned *numtruncatedbyte,
				struct timeval *presentationtime,
				unsigned *durationinmicroseconds) = 0;
		virtual void seekFrame(char*& absStart, char*& absEnd) = 0;
		virtual void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes) = 0;
		virtual enum AVCodecID videocodec() = 0;
		virtual enum AVCodecID audiocodec() = 0;
		virtual void doStopGettingFrames(enum AVMediaType type) = 0;
	void set_unique(unsigned v)
	{
		_unique_id = v;
	}
	unsigned get_unique()
	{
		return _unique_id;
	}
	unsigned reference()
	{
		return _refcount;
	}
	void reference_increase()
	{
		_refcount++;
	}
	void reference_decrease()
	{
		if(_refcount > 0)
		{
			_refcount--;
		}
	}
	virtual const AVCodecContext *codec(enum AVMediaType type)
	{
		return NULL;
	}
};







/*
 	 uvc source
 */
#if defined have_uvc
class live5livemediapp_serversession_source_uvc :
		public live5livemediapp_serversession_source
{
public:
	class uvc_filter :
			public filter
			<pixelframe, avattr>
	{
	public:
		uvc_filter(avattr *p) : filter(p){}

		virtual void operator >>(pixelframe &pf/*uvc return frame*/)
		{
			/*default resize avattr base*/
			swxcontext_class (pf, *((avattr *)ptr()));
		}
	};
	live5livemediapp_serversession_source_uvc(const char *src,
			const avattr &codecinfo) :
				live5livemediapp_serversession_source(),
			_attr(codecinfo),
			_uvc(new uvc(src)),
			_enc(new encoder(_attr)),
			_th(nullptr),/*create when 'dogetnextframe'*/
			_filter(new uvc_filter(&_attr)) { }
	live5livemediapp_serversession_source_uvc(const char *src,
			const avattr codecinfo,
			uvc_filter *custom_filter) :
				live5livemediapp_serversession_source(),
			_attr(codecinfo),
			_uvc(new uvc(src)),
			_enc(new encoder(_attr)),
			_th(nullptr),/*create when 'dogetnextframe'*/
			_filter(custom_filter) { }
	virtual ~live5livemediapp_serversession_source_uvc()
	{
		if(_th)
		{
			_filter->disable();
			_th->join();
			delete _th;
			_th = nullptr;
		}
		if(_filter) delete _filter;
		if(_enc) delete _enc;
		if(_uvc) delete _uvc;
	}
private:
	avattr _attr;
	uvc *_uvc;
	encoder *_enc;
	std::thread *_th;
	uvc_filter *_filter;
	std::mutex _lock;


	virtual int doGetNextFrame(enum AVMediaType type,
			unsigned char*to,
			unsigned size,
			unsigned *numtruncatedbyte,
			struct timeval *presentationtime,
			unsigned *durationinmicroseconds)
	{
		*numtruncatedbyte = 0;
		int readsize = 0;

		readsize = bank_drain(type, to, size);
		if(readsize)
		{
			return readsize;
		}


		_filter->enable();
		int res = _uvc->waitframe(-1);/*tell to uvc 'has frame'*/
		DECLARE_THROW(res < 0, "source uvc has broken");
		if(res > 0)/* res == 0	noframe*/
		{
			_uvc->get_videoframe([&](pixelframe &pix)->void {
					(*_filter) << pix;/*filtering*/
					_enc->encoding(pix,[&](avframe_class &frm,
							avpacket_class &pkt,
							void * )->void{
					{
						bank_in(AVMEDIA_TYPE_VIDEO, pkt.raw()->data, pkt.raw()->size);
						readsize = bank_drain(type, to, size);
						gettimeofday(presentationtime, nullptr);
					}
				});
			});
		}


		return readsize;
	}
	virtual void doStopGettingFrames(enum AVMediaType type)
	{
		/*no condition.. just wait */
	}
	virtual void seekFrame(char*& absStart, char*& absEnd)
	{
		/*can't seek uvc*/
	}
	virtual void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes)
	{
		/*can't seek uvc*/
	}

	virtual enum AVCodecID videocodec()
	{

		if(_enc && _uvc)
		{
			return (enum AVCodecID)_attr.get<avattr::avattr_type_int>(avattr::video_encoderid);
		}
		return AV_CODEC_ID_NONE;
	}
	virtual enum AVCodecID audiocodec()
	{
		/*uvc current has no audio*/
		return AV_CODEC_ID_NONE;
	}
	virtual const AVCodecContext *codec(enum AVMediaType type)
	{
		if(type == AVMEDIA_TYPE_VIDEO)
		{
			return _enc->raw();
		}
		return nullptr;
	}
};

#endif


/*
 	 mp4 container source
 */
class live5livemediapp_serversession_source_mp4 :
		public live5livemediapp_serversession_source
	{
public:
	class mp4_filter :
			public filter
			<avpacket_class, mediacontainer>
	{
	public:
		mp4_filter(mediacontainer *p) : filter(p){}

		virtual void operator >>(avpacket_class &pf/*container's packet*/)
		{
			/*default return*/
		}
	};
private:
		mediacontainer *_container;
		mp4_filter *_filter;
		avpacket_bsf *_mp4_h264_adts_bsf;
		uint64_t _per_frame;
		void reupload()
		{

			std::string target = _container->filename();
			delete _mp4_h264_adts_bsf;
			delete _container;

			_container = new mediacontainer(target.c_str());
			_mp4_h264_adts_bsf = new avpacket_bsf(_container);

		}
		void audio_presentationtime(struct timeval *presentationtime,
				unsigned *durationinmicroseconds)
		{
			/*
				 our filtered source can't know audio bps.
				 so we caluation next presentaion bps.
			 */
			  if (presentationtime->tv_sec == 0 && presentationtime->tv_usec == 0) {
				// This is the first frame, so use the current time:
				gettimeofday(presentationtime, NULL);
			  } else {
				// Increment by the play time of the previous frame:
				unsigned uSeconds = presentationtime->tv_usec + _per_frame;
				presentationtime->tv_sec += uSeconds/1000000;
				presentationtime->tv_usec = uSeconds%1000000;
			  }

			  *durationinmicroseconds = _per_frame;
		}
		int read_once(struct timeval *presentationtime,
				unsigned *durationinmicroseconds,
				enum AVMediaType type,
				unsigned char*to,
				unsigned size)
		{
			int readsize = 0;
			readsize = bank_drain(type, to, size);
			if(type  == AVMEDIA_TYPE_AUDIO &&
					readsize > 0)
			{
				audio_presentationtime(presentationtime, durationinmicroseconds);
			}
			return readsize;
		}
public:
		live5livemediapp_serversession_source_mp4(const char *src) :
			live5livemediapp_serversession_source(),
					_container(new mediacontainer(src)),
					_filter(new mp4_filter(_container)),
					_mp4_h264_adts_bsf(new avpacket_bsf(_container)),
					_per_frame(0)

		{
			OutPacketBuffer::increaseMaxSizeTo(LIVE5_BUFFER_SIZE);
			if(_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_AUDIO))
			{
				/*
						  codec->frame_size  : The following data should not be initialized.

							  Number of samples per channel in an audio frame.
				 */
				 _per_frame = (_container->find_stream(AVMEDIA_TYPE_AUDIO)->codec->frame_size * 1000000) /
						 _container->find_stream(AVMEDIA_TYPE_AUDIO)->codec->sample_rate;
			}
		}
		live5livemediapp_serversession_source_mp4(mp4_filter *filter) :
					live5livemediapp_serversession_source(),
						_container(_filter->ptr()),
						_filter(filter),
						_mp4_h264_adts_bsf(new avpacket_bsf(_container)),
						_per_frame(0)

		{
			OutPacketBuffer::increaseMaxSizeTo(LIVE5_BUFFER_SIZE);

			if(_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_AUDIO))
			{
				/*
						  codec->frame_size  : The following data should not be initialized.

							  Number of samples per channel in an audio frame.
				 */
				 _per_frame = (_container->find_stream(AVMEDIA_TYPE_AUDIO)->codec->frame_size * 1000000) /
						 _container->find_stream(AVMEDIA_TYPE_AUDIO)->codec->sample_rate;
			}
		}
		virtual ~live5livemediapp_serversession_source_mp4()
		{
			if(_mp4_h264_adts_bsf) delete _mp4_h264_adts_bsf;
			//if(_container) delete _container; so... perhapse filter's descructor
			if(_filter) delete _filter;
		}
		virtual int doGetNextFrame(enum AVMediaType type,
				unsigned char*to,
				unsigned size,
				unsigned * numtruncatedbyte,
				struct timeval *presentationtime,
				unsigned *durationinmicroseconds)
		{
			//TODO MAKE THREAD
			int readsize = 0;

			*numtruncatedbyte = 0;
			readsize = read_once(presentationtime,
					durationinmicroseconds,
					type,
					to,
					size);
			if(readsize > 0)
			{
				return readsize;
			}
			avpacket_class	pkt;
			bool res = _container->read_packet(type, pkt);

			if(!res)
			{
				if(_container->eof_stream())
				{
					reupload();
					return doGetNextFrame(type,
							to,
							size,
							numtruncatedbyte,
							presentationtime,
							durationinmicroseconds);

				}
				else
				{
					/*wait until drain another media*/
					return 0;
				}
			}

			/*
				 filtering to raw encoded data
			 */
			pkt.settype(type);
			(*_filter) << (pkt);
			(*_mp4_h264_adts_bsf)(pkt);

			bank_in(type, pkt.raw()->data, pkt.raw()->size);
			return read_once(presentationtime,
					durationinmicroseconds,
					type, to, size);
		}
		virtual void doStopGettingFrames(enum AVMediaType type)
		{
			/*no condition.. just wait */
		}
		virtual void seekFrame(char*& absStart, char*& absEnd)
		{

		}
		virtual void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes)
		{

		}
		virtual enum AVCodecID videocodec()
		{
			return _mp4_h264_adts_bsf &&
					_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_VIDEO) &&
					AV_CODEC_ID_H264 == _container->find_codec(AVMEDIA_TYPE_VIDEO) ?
					AV_CODEC_ID_H264 : AV_CODEC_ID_NONE;

		}
		virtual enum AVCodecID audiocodec()
		{
			return _mp4_h264_adts_bsf &&
					_mp4_h264_adts_bsf->can_bsf(AVMEDIA_TYPE_AUDIO) &&
					AV_CODEC_ID_AAC == _container->find_codec(AVMEDIA_TYPE_AUDIO) ?
					AV_CODEC_ID_AAC : AV_CODEC_ID_NONE;

		}
		virtual const AVCodecContext *codec(enum AVMediaType type)
		{
			if(_container->find_stream(type))
			{
				return _container->find_stream(type)->codec;
			}

			return nullptr;
		}
	};



/*
 	 rtspclient source
 */
class live5livemediapp_serversession_source_rtspclient :
		public live5livemediapp_serversession_source
{
public:
	class rtspclient_filter : public FramedFilter
	{
	protected:
		enum AVMediaType _mediatype;
		enum AVCodecID _codecid;
		virtual void transcode(unsigned char *&fto,
				unsigned &ftosize) { /*default no implement (just bypass)*/}
	public:
		rtspclient_filter(UsageEnvironment& env,
				enum AVMediaType mediatype,
				enum AVCodecID codecid,
				FramedSource* inputSource) :
			FramedFilter(env, inputSource),
			_mediatype (mediatype),
			_codecid(codecid){}
		virtual ~rtspclient_filter(){}

		static void afterGettingFrame(void* clientData, unsigned frameSize,
			     unsigned numTruncatedBytes,
			     struct timeval presentationTime,
			     unsigned durationInMicroseconds)
		{
			rtspclient_filter *filter = (rtspclient_filter *)clientData;
			filter->afterGettringFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
		}
		void afterGettringFrame(unsigned frameSize,
			     unsigned numTruncatedBytes,
			     struct timeval presentationTime,
			     unsigned durationInMicroseconds)
		{
			  fFrameSize = frameSize;
			  fNumTruncatedBytes = numTruncatedBytes;
			  fPresentationTime = presentationTime;
			  fDurationInMicroseconds = durationInMicroseconds;
			  /*now filter run*/
			  transcode(fTo, fFrameSize);
			  FramedSource::afterGetting(this);
		}
		virtual void doGetNextFrame()
		{

			fInputSource->getNextFrame(fTo, fMaxSize,
					rtspclient_filter::afterGettingFrame,
					this,
					FramedSource::handleClosure,
					this);
		}
	};
	virtual rtspclient_filter *create_filter(UsageEnvironment& env,
			FramedSource* inputSource,
			enum AVMediaType mediatype,
			enum AVCodecID codecid,
			char const *fmtp = nullptr)
	{
		return new rtspclient_filter(env, mediatype, codecid, inputSource);
	}
	live5livemediapp_serversession_source_rtspclient() :
				live5livemediapp_serversession_source() { }
	virtual ~live5livemediapp_serversession_source_rtspclient() { }
	int doGetNextFrame(enum AVMediaType type,
			unsigned char*to,
			unsigned size,
			unsigned * numtruncatedbyte,
			struct timeval *presentationtime,
			unsigned *durationinmicroseconds) { return 0; /*serversession implement*/}
	void doStopGettingFrames(enum AVMediaType type) { /*serversession implement*/}
	 void seekFrame(char*& absStart, char*& absEnd) { /*serversession implement*/}
	void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes) { /*serversession implement*/}
	enum AVCodecID videocodec() { return AV_CODEC_ID_NONE; /*support video*/}
	enum AVCodecID audiocodec() { return AV_CODEC_ID_NONE; /**support audio*/}
	const AVCodecContext *codec(enum AVMediaType type) { return nullptr; /*not need */}
};
