#pragma once
/*
 	 	 audio raw data class
 */
class pcm :
		public raw_media_data
{
private:
	std::tuple <int,/*channel*/
	int,/*samplingrate*/
	int,/*samplesize*/
	enum AVSampleFormat/*sample format*/
	> _val;
public:
	/*
	 	 because reqeust size always > 0
	 */
	pcm() : raw_media_data(AVMEDIA_TYPE_AUDIO, 0),
	_val(std::make_tuple(0, 0, 0, AV_SAMPLE_FMT_NONE)) { }
	pcm(raw_media_data::type_size s) : raw_media_data(AVMEDIA_TYPE_AUDIO, s),
	_val(std::make_tuple(0, 0, 0, AV_SAMPLE_FMT_NONE)) { }
	pcm(const pcm &rhs) :
		raw_media_data (dynamic_cast<const raw_media_data &>(rhs)),
		_val(rhs._val) { }
	pcm(pcm &&rhs) :
		raw_media_data (std::move(rhs)),
		_val(rhs._val) { }
	virtual ~pcm(){}
	pcm &operator =
			(const pcm &rhs)
	{
		raw_media_data::operator =
				(dynamic_cast<const raw_media_data &>(rhs));
		_val = rhs._val;
		return *this;
	}
	pcm &operator =
			(pcm &&rhs)
	{
		raw_media_data::operator =
				(std::move(rhs));
		_val = rhs._val;
		return *this;
	}

	pcm &operator =  ( const std::pair<raw_media_data::type_ptr,
			raw_media_data::type_size> &ds)
	{
		raw_media_data::operator =(ds);
		return *this;
	}
	pcm &operator =  (raw_media_data::type_pts p)
	{
		raw_media_data::operator =(p);
		return *this;
	}
	pcm &operator =  (raw_media_data::type_media t)
	{
		raw_media_data::operator =(AVMEDIA_TYPE_AUDIO);
		return *this;
	}
	pcm &operator =  (const std::tuple<raw_media_data::type_ptr,
			raw_media_data::type_size,
			raw_media_data::type_media,
			raw_media_data::type_pts>& v)
	{
		(*this) =  (std::make_pair(std::get<0>(v), std::get<1>(v)));
		(*this) =  std::get<2>(v);
		(*this) =  std::get<3>(v);
		return *this;
	}

	pcm &operator -=(unsigned s)
	{
		raw_media_data::type_ptr p = nullptr;
		if(_s)
		{
			unsigned reduce = _s - std::min(s, _s);
			if(reduce)
			{
				type_data d[reduce];
				memcpy(d, take<raw_media_data::type_ptr>(), reduce);
				p = d;
			}

			dynamic_cast<raw_media_data &>(*this) =
					std::make_pair(p, reduce);
		}

		return *this;
	}
	pcm &operator +=(const std::pair<raw_media_data::type_ptr,
			raw_media_data::type_size>&v)
	{
		raw_media_data::type_size t_s = _s + v.second;
		type_data d[t_s];
		raw_media_data::type_ptr p = d;
		memcpy(p, take<raw_media_data::type_ptr>(), _s);
		memcpy(p + _s, v.first, v.second);
		dynamic_cast<raw_media_data &>(*this) =
				std::make_pair(p, t_s);
		return *this;
	}

	void set(int channel,
			int samplingrate,
			int samplesize,
			enum AVSampleFormat format)
	{
		_val = std::make_tuple(channel,
				samplingrate,
				samplesize,
				format);
	}
	void set(std::tuple<int,
			int,
			int,
			enum AVSampleFormat> &&val)
	{
		_val = val;
	}

	int channel() const
	{ return std::get<0>(_val); }
	int samplingrate() const
	{ return std::get<1>(_val); }
	int samplesize() const
	{ return std::get<2>(_val); }
	enum AVSampleFormat format() const
	{ return std::get<3>(_val); }
    virtual operator bool ()const
	{
		/*
		 	 responsor filled field
		 */
		return std::get<0>(_val) > 0 &&
				std::get<1>(_val) > 0 &&
				std::get<2>(_val) > 0 &&
				std::get<3>(_val) != AV_SAMPLE_FMT_NONE &&
				raw_media_data::operator bool();
	}
};
