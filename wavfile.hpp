#pragma once

class wavfile
{
private:
	struct riff
	{
		unsigned char chunkid[4];
		unsigned chunksize;
		unsigned char format[4];
	};
	struct fmt
	{
		unsigned char chunkid[4];
		unsigned chunksize;
		unsigned short audioformat;
		unsigned short channel;
		unsigned samplingrate;
		unsigned byterate;
		unsigned short blockalign;
		unsigned short bps;
	};
	struct data
	{
		unsigned char chunkid[4];
		unsigned chunksize;
	};

	struct _header
	{
		struct riff _riff;
		struct fmt _fmt;
		struct data _data;
	};
	int _m;/*mode. if set 0 read mode, other write mode*/
	std::string _t;
	FILE *_f;
	struct _header _hdr;

	FILE* parse()
	{
		memset(&_hdr, 0, sizeof(struct _header));
		FILE *f = fopen(_t.c_str(), "rb");
		do
		{
			if(!f)
			{
				break;
			}
			if(fread((void *)&_hdr,
					sizeof(char),
					sizeof(struct _header),
					f) != sizeof(struct _header))
			{
				break;
			}
			if(memcmp(_hdr._riff.chunkid,
					"RIFF",
					sizeof(_hdr._riff.chunkid)))
			{
				break;
			}
			if(memcmp(_hdr._riff.format,
					"WAVE",
					sizeof(_hdr._riff.format)))
			{
				break;
			}
			if(memcmp(_hdr._fmt.chunkid,
								"fmt ",
								sizeof(_hdr._fmt.chunkid)))
			{
				break;
			}
			if(_hdr._fmt.chunksize != (sizeof(_hdr._fmt) -
					(sizeof(_hdr._fmt.chunkid) + sizeof(_hdr._fmt.chunksize))))
			{
				break;
			}
			if(_hdr._fmt.audioformat != 1)
			{
				break;
			}
			if(memcmp(_hdr._data.chunkid,
					"data",
					sizeof(_hdr._data.chunkid)))
			{
				break;
			}
			if(_hdr._data.chunksize <= 0)
			{
				break;
			}
			_f = f;/*for rewind()*/
			rewind();
			return f;
		}while(0);
		if(f)
		{
			fclose(f);
		}
		return nullptr;
	}

	FILE *create_open(const std::string &t,
			int channel,
			int samplingrate,
			enum AVSampleFormat format)
	{
		FILE *f = nullptr;
		/* header riff*/
		memset(&_hdr, 0, sizeof(struct _header));
		memcpy(_hdr._riff.chunkid, "RIFF", sizeof(_hdr._riff.chunkid));
		_hdr._riff.chunksize += (sizeof(struct _header) -
				(sizeof(_hdr._riff.chunkid) +
						sizeof(_hdr._riff.chunksize)));
		memcpy(_hdr._riff.format, "WAVE", sizeof(_hdr._riff.format));


		/* header format */
		memcpy(_hdr._fmt.chunkid, "fmt ", sizeof(_hdr._fmt.chunkid));
		_hdr._fmt.chunksize += (sizeof(struct fmt) - (sizeof(_hdr._fmt.chunksize) - sizeof(_hdr._fmt.chunkid)));
		_hdr._fmt.audioformat = 1;
		_hdr._fmt.channel = channel;
		_hdr._fmt.samplingrate = samplingrate;
		_hdr._fmt.byterate = _hdr._fmt.samplingrate * _hdr._fmt.channel * av_get_bytes_per_sample(format);
		_hdr._fmt.blockalign = _hdr._fmt.channel * av_get_bytes_per_sample(format);
		_hdr._fmt.bps = av_get_bytes_per_sample(format) * 8;

		/*header data*/
		memcpy(_hdr._data.chunkid, "data", sizeof(_hdr._data.chunkid));
		_hdr._data.chunksize += 0;

		/*first empty data file make*/

		f = fopen(t.c_str(), "wb");
		if(f)
		{
			if(fwrite(&_hdr, sizeof(char), sizeof(struct _header), f) == sizeof(struct _header))
			{
				return f;
			}
			fclose(f);
		}

		return nullptr;
	}
	bool is_readmode()
	{
		return _m == 0;
	}
public :
	wavfile(const std::string &t) :
		_m(0),
		_t(t),
		_f(parse()) { }
	wavfile(const std::string &t,
			int channel,
			int samplingrate,
			enum AVSampleFormat format) :
				_m(1),
				_t(t),
				_f(create_open(t, channel, samplingrate, format)) { }


	virtual ~wavfile()
	{
		if(_f)
		{
			if(!is_readmode())
			{
				flush();
			}
			fclose(_f);
			_f = nullptr;
		}
	}
	 operator bool ()
	{
		return _f != nullptr;
	}
	unsigned channel()
	{
		return _hdr._fmt.channel;
	}
	unsigned samplingrate()
	{
		return _hdr._fmt.samplingrate;
	}
	unsigned bitpersample()
	{
		return _hdr._fmt.bps;
	}
	unsigned playtime()
	{
		int sec = 0;
		double msec = 0.0;
		sec = (datasize() / _hdr._fmt.byterate) * 1000;

		msec = (((datasize() % _hdr._fmt.byterate) / _hdr._fmt.byterate) * 1000);

		//printf("datasize = %d byterate = %d sec = %d msec = %d\n", datasize(), _hdr._fmt.byterate, sec, msec);
		return sec + (int)msec;
	}
	unsigned bytepersample()
	{
		return bitpersample() / 8;
	}
	enum AVSampleFormat format()
	{
		if (bytepersample() == av_get_bytes_per_sample(AV_SAMPLE_FMT_S16))
		{
			return AV_SAMPLE_FMT_S16;
		}
		if(bytepersample() == av_get_bytes_per_sample(AV_SAMPLE_FMT_U8))
		{
			return AV_SAMPLE_FMT_U8;
		}
		return AV_SAMPLE_FMT_NONE;
	}
	unsigned datasize()
	{
		return _hdr._data.chunksize;
	}
	unsigned samplesize()
	{
		return datasize() / (bytepersample() * channel());
	}
	std::string name()
	{
		return _t;
	}
	void rewind()
	{
		if(!*this)
		{
			return;
		}
		if(is_readmode())
		{
			fseek(_f, sizeof(struct _header), SEEK_SET);
		}
	}
	bool eof()
	{
		if(is_readmode())
		{
			return feof(_f) != 0;
		}
		return false;
	}
	void write( pcm &d, bool f = false)
	{
		if(!*this ||
				is_readmode() ||
				!d)
		{
			return ;
		}
		if(d.channel() != _hdr._fmt.channel ||
				d.samplingrate() != _hdr._fmt.samplingrate ||
				av_get_bytes_per_sample(d.format()) != _hdr._fmt.bps)
		{
			return;
		}
		_hdr._data.chunksize += d.take<raw_media_data::type_size>();
		_hdr._riff.chunksize += d.take<raw_media_data::type_size>();
		fwrite(d.take<raw_media_data::type_ptr>(),
				sizeof(char),
				d.take<raw_media_data::type_size>(),
				_f);
		if(f)
		{
			flush();
		}
	}
	void flush()
	{
		if(*this)
		{
			if(!is_readmode())
			{
				fseek(_f, 0, SEEK_SET);
				fwrite(&_hdr, sizeof(char), sizeof(struct _header), _f) == sizeof(struct _header);
				fseek(_f, 0, SEEK_END);
			}
			fflush(_f);
		}
	}
	void read(pcm_require &pcm_re)
	{
		if(!*this ||
				!is_readmode() ||
				feof(_f) )
		{
			pcm_re.first = pcm();
			return;
		}

		uint8_t data[pcm_re.second];

		raw_media_data::type_ptr pdata = data;
		int size = fread(data,
				sizeof(char),
				pcm_re.second,
				_f);
		if(size <= 0)
		{
			pcm_re.first = pcm();
			return;
		}
		printf("wavfile read() = %dbyte\n", size);
		pcm res(size);
		res = std::make_pair(pdata, size);
		res.set(channel(),
				 samplingrate(),
				 (size / (bytepersample() * channel())),
				 bitpersample() == 8 ? AV_SAMPLE_FMT_U8 :
				 				AV_SAMPLE_FMT_S16);
		pcm_re.first = res;
	}
	pcm cache()
	{
		if(!*this ||
				!is_readmode())
		{
			return pcm();
		}
		int cur = ftell(_f);
		fseek(_f, sizeof(struct _header), SEEK_SET);
		pcm res(_hdr._data.chunksize);
		if(fread(res.take<raw_media_data::type_ptr>(),
				sizeof(char),
				res.take<raw_media_data::type_size>() ,
				_f)!= res.take<raw_media_data::type_size>())
		{
			fseek(_f, cur, SEEK_SET);
			return pcm();
		}

		res.set(channel(),
				samplingrate(),
				samplesize(),
				bitpersample() == 8 ? AV_SAMPLE_FMT_U8 :
				AV_SAMPLE_FMT_S16);
		fseek(_f, cur, SEEK_SET);
		return res;
	}

};
