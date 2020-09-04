/*
 	 audio frame
 */

class pcmframe :
		public avframe_class_type
		<pcm>
{
private:
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
					0,/*TODO calc samplesize*/
					(enum AVSampleFormat)raw()->format));
	}
public:
	pcmframe() = delete;
	pcmframe(const  AVFrame &frm) :
		avframe_class_type(frm)
	{
		throw_if_not_audio();
	}
	pcmframe(const pcmframe &_class) :
		avframe_class_type(dynamic_cast<const avframe_class_type &>(_class))
	{
		throw_if_not_audio();
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
				(dynamic_cast<const avframe_class_type &>(_class));
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
			return av_samples_get_buffer_size(NULL,
						 raw()->channels,
						 raw()->nb_samples,
				         (enum AVSampleFormat)raw()->format,
						 1);
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
	virtual raw_media_data data_alloc()
	{

		int length = len();

		if(length > 0)
		{
			raw_media_data d(AVMEDIA_TYPE_AUDIO, length, 0);
			memcpy(d.take<raw_media_data::type_ptr>(), raw()->extended_data[0], length);
			return d;
		}
		return raw_media_data();
	}
};


class pcmframe_presentationtime : public pcmframe
{
protected:
	double _pts;
	AVRational _rational;

public:
	pcmframe_presentationtime(const pcmframe &f,
			const AVRational rational) :
				pcmframe(f),
				_pts(0.0),
				_rational(rational){}
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

