#pragma once
#include <alsa/asoundlib.h>
#include <alsa/control.h>



class audiospec : public pcm
{
	friend class soundcardctl;
	unsigned _period;
	unsigned _periodsize;
	/*for init soundcard*/
	audiospec() : pcm(),
			_period(0),
			_periodsize(0){}
	/*for running soundcard*/
	audiospec(int channel,
			int samplingrate,
			int samplesize,
			enum AVSampleFormat fmt,
			unsigned datasize,
			unsigned period,
			unsigned periodsize) :
				pcm(datasize),
				_period(period),
				_periodsize(periodsize){
		set(channel, samplingrate, samplesize, fmt);
	}
public:
	/*for user request*/
	audiospec(int channel,
			int samplingrate,
			int samplesize,
			enum AVSampleFormat fmt) :
		pcm() { set(channel, samplingrate, samplesize, fmt); }
	virtual ~audiospec(){}
	audiospec &operator =
			(const audiospec &rhs)
	{
		pcm::operator =
				(dynamic_cast<const pcm &>(rhs));
		return *this;
	}
};
class soundcard
{
public:
	using _cardid = unsigned;
	using _deviceid = unsigned;
	using soundcardid = std::pair<_cardid, _deviceid>;
protected:

	using soundcard_infos = std::tuple<snd_ctl_t *,
			snd_ctl_card_info_t *,
			std::list<snd_pcm_info_t *>>;

	 std::list<soundcard_infos>_cardlists[SND_PCM_STREAM_LAST+1];

	void free_soundcard(std::list<soundcard_infos> &cardlists)
	{

		for(auto &it : cardlists)
		{
			std::list<snd_pcm_info_t *>::iterator pit;
			pit = std::get<2>(it).begin();
			while(pit != std::get<2>(it).end())
			{
				snd_pcm_info_free(*pit);
				pit++;
			}
			std::get<2>(it).clear();
			snd_ctl_card_info_free(std::get<1>(it));
			snd_ctl_close(std::get<0>(it));
		}
		cardlists.clear();
	}
	void alloc_soundcard(std::list<soundcard_infos> &cardlists, snd_pcm_stream_t stream)
	{
		snd_ctl_t *h = nullptr;
		snd_ctl_card_info_t *i = nullptr;
		int card = -1;
		int dev = -1;

		while(snd_card_next(&card)>= 0 &&
				card >= 0)
		{
			char name[32] = {0, };
			sprintf(name, "hw:%d", card);
			snd_ctl_open(&h, name, 0);

			snd_ctl_card_info_malloc(&i);
			snd_ctl_card_info(h, i);


			std::list<snd_pcm_info_t *> infos;
			while(snd_ctl_pcm_next_device(h, &dev) >= 0 &&
					dev >= 0)
			{
				unsigned count = -1;
				snd_pcm_info_t *p = nullptr;
				snd_pcm_info_malloc(&p);
				snd_pcm_info_set_device(p, dev);
				snd_pcm_info_set_subdevice(p, 0);
				snd_pcm_info_set_stream(p, stream);
				if(snd_ctl_pcm_info(h, p) < 0)
				{
					snd_pcm_info_free(p);
					continue;
				}
				infos.push_back(p);
			}

			cardlists.push_back(std::make_tuple(h, i, infos));
		}
	}
	soundcard()
	{

	}
	virtual ~soundcard()
	{
		free_soundcard(_cardlists[SND_PCM_STREAM_CAPTURE]);
		free_soundcard(_cardlists[SND_PCM_STREAM_PLAYBACK]);
	}

	virtual std::list<soundcardid> list(snd_pcm_stream_t stream)
	{
		std::list<soundcardid> list;
		for(auto &it : _cardlists[stream])
		{
			for(auto &dit : std::get<2>(it))
			{
				list.push_back(std::make_pair(snd_ctl_card_info_get_card(std::get<1>(it)),
						snd_pcm_info_get_device(dit)));
			}
		}
		return list;
	}

	virtual std::string cardid(const soundcardid &id, snd_pcm_stream_t stream)
	{
		std::string _id;
		for(auto &it : _cardlists[stream])
		{
			if(snd_ctl_card_info_get_card(std::get<1>(it)) == id.first)
			{
				_id = snd_ctl_card_info_get_id(std::get<1>(it));
				break;
			}
		}
		return _id;
	}
	virtual std::string cardname(const soundcardid &id, snd_pcm_stream_t stream)
	{
		std::string device;
		for(auto &it : _cardlists[stream])
		{
			if(snd_ctl_card_info_get_card(std::get<1>(it)) == id.first)
			{
				device = snd_ctl_card_info_get_name(std::get<1>(it));
				break;
			}
		}
		return device;
	}
	virtual std::string deviceid(const soundcardid &id, snd_pcm_stream_t stream)
	{
		std::string deviceid;
		for(auto &it : _cardlists[stream])
		{
			if(snd_ctl_card_info_get_card(std::get<1>(it)) == id.first)
			{
				for(auto &dit : std::get<2>(it))
				{
					if(snd_pcm_info_get_device(dit) == id.second)
					{
						deviceid = snd_pcm_info_get_id(dit);
						break;
					}
				}
				break;
			}
		}
		return deviceid;
	}
	virtual std::string devicename(const soundcardid &id, snd_pcm_stream_t stream)
	{
		std::string deviceid;
		for(auto &it : _cardlists[stream])
		{
			if(snd_ctl_card_info_get_card(std::get<1>(it)) == id.first)
			{
				for(auto &dit : std::get<2>(it))
				{
					if(snd_pcm_info_get_device(dit) == id.second)
					{
						deviceid = snd_pcm_info_get_name(dit);
						break;
					}
				}
				break;
			}
		}
		return deviceid;
	}
};

//==============================================================================================
//
//==============================================================================================


class soundcardctl : public soundcard
{
protected:
	snd_pcm_t *_h;
	audiospec _d;
	unsigned _latency;
	snd_pcm_stream_t _s;
	bool _verbose_write;
	bool _verbose_read;
	soundcardctl(snd_pcm_stream_t t) :
		soundcard(),
		_h(nullptr),
		_d(audiospec()),
		_latency(0),
		_s(t),
		_verbose_write(false),
		_verbose_read(false){}
	virtual ~soundcardctl()
	{
		close();
	}
	int hw_par()
	{
		snd_pcm_hw_params_t *params = nullptr;
		snd_pcm_format_t fmt_act = _d.format() == AV_SAMPLE_FMT_U8 ? SND_PCM_FORMAT_U8 :
				_d.format() == AV_SAMPLE_FMT_S16 ? SND_PCM_FORMAT_S16_LE :
						SND_PCM_FORMAT_UNKNOWN;
		unsigned samplingrate_act = _d.samplingrate();
		unsigned channel_act = _d.channel();
		snd_pcm_uframes_t samplesize_act = _d.samplesize();/*period size.*/
		unsigned period = 2;/* fix */
		unsigned buffersize = 0;
		unsigned period_framebyte = 0;



		snd_pcm_hw_params_alloca(&params);
		/*we using nonblock mode*/
		snd_pcm_nonblock(_h, 1);

		/*get hw current*/
		if(snd_pcm_hw_params_any(_h, params) < 0)
		{
			return -1;
		}
		/*
		 	 set 'interleaved'
		 	 we using 'LRLRLRLR' interleaved mode
		 	 not 'LLLRRR LLLRRR' noninterlevaed mode
		 */
		if(snd_pcm_hw_params_set_access(_h, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
		{
			return -2;
		}

		/*
		 	 set format
		 */
		if(snd_pcm_hw_params_set_format(_h, params, fmt_act) < 0)
		{
			return -4;
		}

		/*
		 	 set channel
		 	 if set 'SND_PCM_FORMAT_UNKNOWN' return error?
		 */
		if(snd_pcm_hw_params_set_channels_near(_h, params, &channel_act) < 0)
		{
			return -5;
		}

		/*
			 set samplingrate near hw
		 */
		if(snd_pcm_hw_params_set_rate_near(_h, params, &samplingrate_act, nullptr) < 0)
		{
			return -6;
		}

		/*
		 	 set the period size
		*/
		if(snd_pcm_hw_params_set_period_size_near(_h, params, &samplesize_act, nullptr) < 0)
		{
			return -7;
		}
		/*
		 	 set the period
		 */
		if( snd_pcm_hw_params_set_periods_near(_h, params, &period, nullptr) < 0)
		{

			return -8;
		}
		/*
		 	 calc buffer
		 	 see 'https://www.alsa-project.org/wiki/FramesPeriods'
		 */
		buffersize = samplesize_act * period;
		period_framebyte = (samplesize_act * period) * (channel_act * snd_pcm_format_width(fmt_act) / 8);


		/*
		 	check calced buffer
		 */
		snd_pcm_uframes_t testsize = 0;

		if(snd_pcm_hw_params_get_buffer_size(params, &testsize) < 0)
		{
			return -9;
		}
		if(buffersize != testsize)
		{
			/*TODO retry period size value = testsize / period ? */
			return -10;
		}

		/*now set soundcard by this values*/
		if(snd_pcm_hw_params(_h, params) < 0)
		{
			return -11;
		}

		snd_pcm_uframes_t periodsize = 0;
		snd_pcm_hw_params_get_period_size(params, &periodsize, nullptr);
		snd_pcm_hw_params_get_periods(params, &period, nullptr);
		audiospec dump(channel_act,
				samplingrate_act,
				buffersize,
				_d.format(),
				period_framebyte,
				period,
				periodsize);
		_d = std::move(dump);

		_latency = (buffersize * 1000) / samplesize_act;
		return 0;
	}
	int sw_par()
	{
		snd_pcm_uframes_t start_threshold, stop_threshold;
		int availmin = -1;
		   size_t n;
		snd_pcm_sw_params_t *params = nullptr;
		snd_pcm_sw_params_alloca(&params);
	   if(snd_pcm_sw_params_current(_h, params) < 0)
	    {
	    	return -1;
	    }
	   if(availmin < 0)
	   {

		   n = _d._periodsize;
	   }
	   else
	   {
		 n = (double)_d.samplingrate( ) * availmin / 1000000;
	   }
	   if(snd_pcm_sw_params_set_avail_min(_h, params, n))
	    {
		   return -2;
	    }

	   unsigned startdelay = _s == SND_PCM_STREAM_CAPTURE ? 1 : 0;
	   unsigned stopdelay = 0;

	   if(startdelay <= 0)
	   {
		   start_threshold = _d.samplesize() + (double)_d.samplingrate() * startdelay / 1000000;
	   }
	   else
	   {
		   start_threshold = (double)_d.samplingrate() * startdelay / 1000000;
	   }
	   if(start_threshold < 1)
	   {
		   start_threshold = 1;
	   }
	   if(start_threshold > _d.samplesize())
	   {
		   start_threshold = _d.samplesize();
	   }

		if(snd_pcm_sw_params_set_start_threshold(_h, params, start_threshold))
		{
			return -3;
		}

		if(stopdelay <= 0)
		{
			stop_threshold = _d.samplesize() + (double)_d.samplingrate() * stop_threshold / 1000000;
		}
		else
		{
			stop_threshold = (double)_d.samplingrate() * stop_threshold / 1000000;
		}
		if(snd_pcm_sw_params_set_stop_threshold(_h, params, stop_threshold) < 0)
		{
			return -4;
		}
		if(snd_pcm_sw_params(_h, params) < 0)
		{
			return -5;
		}
		return 0;
	}

	bool xrun()
	{
		snd_pcm_status_t *status = nullptr;
		int res = -1;

		snd_pcm_status_alloca(&status);
		if ((res = snd_pcm_status(_h, status))<0)
		{
			return false;
		}
		if(snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN)
		{
			if(snd_pcm_prepare(_h) < 0)
			{
				return false;
			}
		}
		else if(snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING)
		{
			if(_s == SND_PCM_STREAM_CAPTURE)
			{
				if(snd_pcm_prepare(_h) < 0)
				{
					return false;
				}
			}
		}
		return true;
	}
	bool suspend()
	{
		int res = -1;
		int testloop = 300;
		while((res = snd_pcm_resume(_h)) == -EAGAIN &&
				testloop-- > 0)
		{
			usleep(10 * 1000);
		}
		if(snd_pcm_prepare(_h) < 0)
		{
			return false;
		}
		return true;
	}

	int open(char const *target,
			const audiospec &spec )
	{
		close();

		_d = spec;

		/*
		 	 open !
		 */
		if(snd_pcm_open(&_h,
				target,
				_s,
				0))
		{
			return -2;
		}
		if(hw_par())
		{
			close();
			return -3;
		}

		if(sw_par())
		{
			close();
			return -4;
		}
		return 0;
	}
public:
	int open_hw(const soundcardid &id,
			const audiospec &spec )
	{
		char name[256] = {0, };
		sprintf(name, "hw:%d,%d", id.first, id.second);
		return open(name, spec);
	}
	int open_plughw(const soundcardid &id,
			const audiospec &spec )
	{
		char name[256] = {0, };
		sprintf(name, "plughw:%d,%d", id.first, id.second);
		return open(name, spec);
	}
	int open_plugdmix(const audiospec &spec )
	{
		const char *name = "plug:dmix";
		return open(name, spec);
	}
	int open_default(const audiospec &spec )
	{
		const char *name = "default";
		return open(name, spec);
	}
	bool isopen()
	{
		return (_h != nullptr );
	}
	void close()
	{
		if(isopen())
		{
			flush();
			snd_pcm_close(_h);
			_h = nullptr;
		}
		_d = audiospec();
		_latency = 0;
	}
	int read_size()
	{
		if(!isopen())
		{
			return -1;
		}
		return _d.take<raw_media_data::type_size>();
	}
	pcm read()
	{
		if(!isopen())
		{
			return pcm();
		}

		/*
		 	 we always return fully data
			capturing pcm base samplingrate .
		 */

		snd_pcm_format_t fmt = _d.format() == AV_SAMPLE_FMT_U8 ? SND_PCM_FORMAT_U8 :
				_d.format() == AV_SAMPLE_FMT_S16 ? SND_PCM_FORMAT_S16_LE :
						SND_PCM_FORMAT_UNKNOWN;

		/*alsa using a single channel sample*/
		unsigned request_samplesize = _d.samplingrate() ;
		unsigned readi_samplesize_count = 0;
		pcm res(request_samplesize * av_get_bytes_per_sample(_d.format())* _d.channel());
		res.set(_d.channel(), _d.samplingrate(), request_samplesize, _d.format());


		do
		{

			if(_verbose_read)printf("alsa read sample: pcm buffer start position = %dbyte, reqeust size = %dbyte request sample = %d\n",
					(readi_samplesize_count * (av_get_bytes_per_sample(res.format()))),
					(request_samplesize - readi_samplesize_count) * ( av_get_bytes_per_sample(res.format())),
					request_samplesize - readi_samplesize_count);
			snd_pcm_sframes_t readi_samplesize = 0;
			readi_samplesize = snd_pcm_readi(_h,
					res.take<raw_media_data::type_ptr>() + (readi_samplesize_count * ( av_get_bytes_per_sample(res.format()))* _d.channel()),
							request_samplesize - readi_samplesize_count);

			if(_verbose_read)printf("alsa read sample: readi sample size = %d\n", readi_samplesize);

			if(readi_samplesize == -EAGAIN ||
					(readi_samplesize >= 0 &&
							readi_samplesize < (request_samplesize - readi_samplesize_count)))
			{
				snd_pcm_wait(_h, _latency);
				if(_verbose_read)printf("alsa read sample eagain %d\n", readi_samplesize);
			}
			else if(readi_samplesize == -EPIPE)
			{
				if(!xrun())
				{
					return pcm();
				}
				if(_verbose_read)printf("alsa read sample xrun %d\n", readi_samplesize);
			}
			else if(readi_samplesize == -ESTRPIPE)
			{
				if(!suspend())
				{
					return pcm();
				}
				if(_verbose_read)printf("alsa read sample suspend %d\n", readi_samplesize);
			}
			else if(readi_samplesize < 0)
			{
				return pcm();
				if(_verbose_read)printf("alsa read sample invalid %d\n", readi_samplesize);
			}
			if(readi_samplesize > 0)
			{
				readi_samplesize_count += readi_samplesize;
				if(_verbose_read)printf("alsa readed sample  %d\n", readi_samplesize_count);
			}
		}while(readi_samplesize_count < request_samplesize);


		return res;
	}
	int write_size()
	{
		if(!isopen())
		{
			return -1;
		}
		return _d.take<raw_media_data::type_size>();
	}

	int write(pcm &&d)
	{
		return write(d);
	}
	int write( pcm d)
	{
		if(!d)
		{
			return -2;
		}
		if(!isopen())
		{
			return -1;
		}
		unsigned d_cur = 0;
		unsigned d_max = d.take<raw_media_data::type_size>();
		unsigned once_samplesize = 0;
		unsigned once_size = 0;
		ssize_t writei_size = 0;
		unsigned writei_samplesize = 0;
		if(_verbose_write)printf("alsa write : start!\n");
		do
		{

			if(_verbose_write)printf("alsa write : pcm buffer frame size = %dbyte request pcm buffer frame size = %d\n", write_size(), d_max);
			once_size = std::min(write_size(), (int)d_max);
			/*
				 copy to sample
			 */
			if(_verbose_write)printf("alsa write : request pcm buffer frame copy to pcm buffer from position = %dbyte size = %dbyte\n", d_cur, once_size);
			memcpy(_d.take<raw_media_data::type_ptr>(),
					d.take<raw_media_data::type_ptr>() + d_cur,
					once_size);

			/*alsa using a single channel sample*/

			once_samplesize = once_size /  ((av_get_bytes_per_sample(_d.format()) * _d.channel()));
			if(_verbose_write)printf("alsa write : calculate the request pcm buffer samplesize = %d\n", once_samplesize);

			/*short sample for one period.
			 * so set the silience */
			if(_d.samplesize() > once_samplesize)
			{
				if(_verbose_write)printf("alsa write : pcm sample size is large than reqeust pcm buffer samplesize %d / %d\n", _d.samplesize(), once_samplesize);
				snd_pcm_format_t fmt = _d.format() == AV_SAMPLE_FMT_U8 ? SND_PCM_FORMAT_U8 :
						_d.format() == AV_SAMPLE_FMT_S16 ? SND_PCM_FORMAT_S16_LE :
								SND_PCM_FORMAT_UNKNOWN;

				if(_verbose_write)printf("alsa write : set silence sample = %d\n", (_d.samplesize() - (once_samplesize)));
				snd_pcm_format_set_silence(fmt, _d.take<raw_media_data::type_ptr>(), (_d.samplesize() - (once_samplesize)));
				once_samplesize = _d.samplesize();
				if(_verbose_write)printf("alsa write silence complete total sample size = %d\n", once_samplesize);
			}

			if(_verbose_write)printf("alsa write : start the writting samples\n", once_samplesize);
//			snd_pcm_wait(_h, _latency);
			writei_samplesize = 0;
			do
			{

				if(_verbose_write)printf("alsa write sample : remain request pcm buffer samplesize = %d remain pcm buffer framesize = %dbyte\n", once_samplesize,
						(once_samplesize) * _d.channel() * av_get_bytes_per_sample(_d.format()));
				if(_verbose_write)printf("alsa write sample : pcm buffer from position = %dbyte size = %dbyte sample = %d\n",
						(writei_samplesize * ((_d.channel() * av_get_bytes_per_sample(_d.format())))),
						(once_samplesize) * _d.channel() * av_get_bytes_per_sample(_d.format()),
						(once_samplesize));
				writei_size = snd_pcm_writei(_h,
						_d.take<raw_media_data::type_ptr>() + (writei_samplesize * ((_d.channel() * av_get_bytes_per_sample(_d.format())))),
						once_samplesize);

				if(_verbose_write)printf("alsa writed sample = %d\n", writei_size);

				if(writei_size == -EAGAIN ||
						(writei_size >= 0 &&
								writei_size < (once_samplesize)))
				{
					if(_verbose_write)printf("alsa writed sample eagain %d\n", writei_size);
					snd_pcm_wait(_h, _latency);
				}
				else if(writei_size == -EPIPE)
				{
					if(!xrun())
					{
						return -97;
					}
					if(_verbose_write)printf("alsa writed sample xrun %d\n", writei_size);
				}
				else if(writei_size == -ESTRPIPE)
				{
					if(!suspend())
					{
						return -98;
					}
					if(_verbose_write)printf("alsa writed sample suspend %d\n", writei_size);
				}
				else if(writei_size < 0)
				{
					return -99;
					if(_verbose_write)printf("alsa writed sample invalid %d\n", writei_size);
				}

				if(writei_size > 0)
				{
					once_samplesize -= writei_size;
					writei_samplesize += writei_size;
					if(_verbose_write)printf("alsa writed sample %d\n", writei_size);
				}
			}while(once_samplesize > 0 );
			d_cur += once_size ;
			d_max -= once_size;
		}while(d_max > 0);

		return d_cur;
	}
	void flush()
	{
		if(isopen())
		{
			snd_pcm_drain(_h);
		}
	}
};
class soundcardplayback : public soundcardctl
{
public:
	soundcardplayback() :
		soundcardctl(SND_PCM_STREAM_PLAYBACK)
	{
		alloc_soundcard(_cardlists[_s],
				_s);
	}

	virtual ~soundcardplayback() { }
	std::list<soundcardid> list()
	{
		return soundcard::list(SND_PCM_STREAM_PLAYBACK);
	}
	std::string cardid(const soundcardid &id)
	{
		return soundcard::cardid(id, SND_PCM_STREAM_PLAYBACK);
	}
	std::string cardname(const soundcardid &id)
	{
		return soundcard::cardname(id, SND_PCM_STREAM_PLAYBACK);
	}
	std::string deviceid(const soundcardid &id)
	{
		return soundcard::deviceid(id, SND_PCM_STREAM_PLAYBACK);
	}
	std::string devicename(const soundcardid &id)
	{
		return soundcard::devicename(id, SND_PCM_STREAM_PLAYBACK);
	}
	int read_size() = delete;
	pcm read() = delete;
};

class soundcardcapture : public soundcardctl
{
	std::string _n;
public:
	soundcardcapture() :
		soundcardctl(SND_PCM_STREAM_CAPTURE),
		_n(std::string())
	{
		alloc_soundcard(_cardlists[_s],
				_s);
	}

	virtual ~soundcardcapture() { }
	std::list<soundcardid> list()
	{
		return soundcard::list(SND_PCM_STREAM_CAPTURE);
	}
	std::string cardid(const soundcardid &id)
	{
		return soundcard::cardid(id, SND_PCM_STREAM_CAPTURE);
	}
	std::string cardname(const soundcardid &id)
	{
		return soundcard::cardname(id, SND_PCM_STREAM_CAPTURE);
	}
	std::string deviceid(const soundcardid &id)
	{
		return soundcard::deviceid(id, SND_PCM_STREAM_CAPTURE);
	}
	std::string devicename(const soundcardid &id)
	{
		return soundcard::devicename(id, SND_PCM_STREAM_CAPTURE);
	}
	int write_size() = delete;
	int write( pcm &d) = delete;
};

