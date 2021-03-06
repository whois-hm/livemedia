/*
 	 audio frame
 */

class pcmframe :
		public avframe_class_type
		<pcm>
{
protected:
	void throw_if_not_audio()
	{
		DECLARE_THROW(type() != AVMEDIA_TYPE_AUDIO,
			"pcm frame type no match");
	}
	virtual void field_attr_value(pcm &t)
	{
		t.set(std::tuple<int,
			int,
			int,
			enum AVSampleFormat>(raw()->channels,
					raw()->sample_rate,
					raw()->nb_samples,
					(enum AVSampleFormat)raw()->format));
	}
public:
	pcmframe() = delete;
	pcmframe(const  AVFrame &frm) :
		avframe_class_type<pcm>(frm)
	{
		throw_if_not_audio();
	}
	pcmframe(const pcmframe &_class) :
		avframe_class_type<pcm>(dynamic_cast<const avframe_class_type &>(_class))
	{
		throw_if_not_audio();
	}
	pcmframe(int channel,
			int samplingrate,
			int samplesize,
			enum AVSampleFormat fmt,
			uint8_t *data,
			int datasize) :
				avframe_class_type<pcm>()
	{
		raw()->channels = channel;
		raw()->sample_rate = samplingrate;
		raw()->format = (int)fmt;
		raw()->nb_samples = samplesize;
		raw()->channel_layout = av_get_default_channel_layout(raw()->channels);

		av_samples_alloc_array_and_samples(&raw()->extended_data, raw()->linesize, channel,
		                     samplesize, fmt, 1);
		av_samples_copy(raw()->extended_data, (uint8_t * const*)&data, 0,
		                    0, samplesize, channel,
		                    fmt);

	}
	virtual ~pcmframe()
	{

	}

	pcmframe &operator = (const  AVFrame &frm)
	{
		avframe_class::operator =(frm);
		throw_if_not_audio();
		return *this;
	}
	pcmframe &operator = (const pcmframe &_class)
	{
		avframe_class_type::operator =
				(dynamic_cast<const avframe_class_type <pcm>&>(_class));
		return *this;
	}
	virtual int len()
	{
		/*
		 	 For audio, only linesize[0] may be set. For planar audio, each channel plane must be the same size.
		 	 perhaps return value == first argument * raw()->channels
		 */
		/*
		 	 	      * For planar audio, each channel has a separate data pointer, and
		 	 	      * linesize[0] contains the size of each channel buffer.
		 	 	      * For packed audio, there is just one data pointer, and linesize[0]
		 	 	      * contains the total size of the buffer for all channels.
		 */
		int len = av_samples_get_buffer_size(NULL,
				 raw()->channels,
				 raw()->nb_samples,
		         (enum AVSampleFormat)raw()->format,
				 1);
			return len;
	}
	virtual void data_copy(uint8_t *ptr, int length)
	{
		DECLARE_THROW(!ptr || length < len(), "pixelframe invalid parameter");
		/*
		 	 	 we operation pcm 1array style, then
		 	 	 no use 'av_samples_copy' function
		 */
		memcpy(ptr, raw()->extended_data[0], length);

	}
	virtual pcm data_alloc()
	{

		int length = len();

		if(length > 0)
		{

			pcm d(length);
			memcpy(d.take<raw_media_data::type_ptr>(), raw()->data[0], length);
			return d;
		}
		return pcm();
	}
};


class pcmframe_presentationtime : public pcmframe
{
protected:
	double _pts;
	AVRational _rational;

public:
	pcmframe_presentationtime &operator =
			(const pcmframe_presentationtime &_class)
	{
		pcmframe::operator =
						(dynamic_cast<const pcmframe &>(_class));
		_pts = _class._pts;
				return *this;
	}
	virtual void field_attr_value(pcm &t)
	{
		pcmframe::field_attr_value(t);

		t = _pts;
	}
	pcmframe_presentationtime (const pcmframe_presentationtime &frame) :
		pcmframe(dynamic_cast<const pcmframe &>(frame)),
		_pts(frame._pts)
	{
	}
	pcmframe_presentationtime(const pcmframe &f,
			const AVRational rational) :
				pcmframe(f),
				_pts(0.0),
				_rational(rational){}
	pcmframe_presentationtime(int channel,
			int samplingrate,
			int samplesize,
			enum AVSampleFormat fmt,
			uint8_t *data,
			int datasize,
			double pts = 0.0) :
				pcmframe(channel,
						samplingrate,
						samplesize,
						fmt,
						data,
						datasize), _pts(pts)
	{_rational.den = 0; _rational.num = 0; }
	double operator()()
	{
		if(_pts != 0.0)
		{
			return _pts;
		}
		if(raw()->pkt_dts != AV_NOPTS_VALUE)
		{
			_pts = av_q2d(_rational) * raw()->pkt_pts;
		}
		return _pts;
	}
};

