#pragma once
#if defined have_uvc
class uvc_playback :
		public playback_inst
{

	struct avscheduler : public
	avframescheduler<pixelframe_presentationtime,
		pcmframe_presentationtime>
	{
	public:
		avscheduler(const avattr &attr) :
			avframescheduler<pixelframe_presentationtime,
					pcmframe_presentationtime>(attr)
					{

					}
		virtual void usingframe( pixelframe_presentationtime &rf){}
			virtual void usingframe( pcmframe_presentationtime &rf){}
	};


	uvc *_uvc;
	soundcardcapture *_soundcard;
	std::thread *_vth;
	std::thread *_ath;
	bool _b;
	bool _p;
	avscheduler _scheduler;
	std::mutex _vlock;
	std::mutex _alock;
public:
	static  void available_devices(std::list<std::string> &list)
	{
		std::string root = "/dev";
		std::string _root = root + std::string("/");
		DIR *dir = nullptr;
		struct dirent *dir_entry = nullptr;

		dir = opendir(_root.c_str());
		if(!dir)
		{
			return;
		}
		while((dir_entry = readdir(dir)) != nullptr)
		{
			struct stat buf;
			lstat((_root + std::string(dir_entry->d_name)).c_str(), &buf);
			if(S_ISDIR(buf.st_mode))
			{
				continue;
			}
			else
			{
				if(contain_string(dir_entry->d_name, "video"))
				{
					list.push_back(_root + std::string(dir_entry->d_name));
				}
			}
		}
		closedir(dir);
	}
	uvc_playback(const avattr &attr, char const *name) :
		playback_inst(attr),
		_uvc(nullptr),
		_soundcard(nullptr),
		_vth(nullptr),
		_ath(nullptr),
		_b(true),
		_p(true),
		_scheduler(attr)
	{
		DECLARE_THROW(attr.notfound(avattr::frame_video) &&
				attr.notfound(avattr::frame_audio), "uvc playback can't found video or audio frame");

		uvc dump(name);
		if(!attr.notfound(avattr::frame_audio))
		{
			soundcardcapture snd;
			std::list<soundcard::soundcardid> list = snd.list();
			for(auto &it : list)
			{
				if(contain_string(dump.name(), snd.cardid(it)) ||
						contain_string(dump.name(), snd.cardname(it)))
				{
					audiospec s(attr.get<avattr::avattr_type_int>(avattr::channel),
							attr.get<avattr::avattr_type_int>(avattr::samplerate),
							512,
							(enum AVSampleFormat)attr.get<avattr::avattr_type_int>(avattr::pcm_format));
					_soundcard = new soundcardcapture();
					if(_soundcard->open_plughw(it, s) < 0)
					{
						delete _soundcard;
						_soundcard = nullptr;
					}
					break;
				}
			}
		}
		if(!attr.notfound(avattr::frame_video))
		{
			_uvc = new uvc(std::move(dump));
		}

		pause();

		if(_uvc)
		{
			_scheduler.set_clock_master(AVMEDIA_TYPE_VIDEO);
			_vth = new std::thread([&]()->void{

				while(_b)
				{
					autolock a(_vlock);
					if(_uvc->waitframe(-1) > 0)
					{
						pixelframe_presentationtime f = _uvc->get_videoframe();
						swxcontext_class (f, (_attr));
						_scheduler << (f);
					}
				}

			});
		}

		if(_soundcard)
		{
			_scheduler.set_clock_master(AVMEDIA_TYPE_AUDIO);
			_ath = new std::thread([&]()->void{

				while(_b)
				{
					autolock a(_alock);
					pcmframe_presentationtime f = _soundcard->read();
					_scheduler << (f);
				}
			});
		}
	}
	virtual ~uvc_playback()
	{
		resume(true);
		if(_uvc)
		{
			delete _uvc;
			_uvc = nullptr;
			_vth->join();
			delete _vth;
			_vth = nullptr;
		}
		if(_soundcard)
		{
			_ath->join();
			delete _ath;
			_ath = nullptr;
			delete _soundcard;
			_soundcard = nullptr;
		}

	}
	enum AVMediaType get_master_clock()
	{
		return _scheduler.get_clock_master();
	}
	void record(char const *file)
	{

	}
	bool pause()
	{
		if(_p)
		{
			return true;
		}
		if(_uvc)
		{
			_vlock.lock();
			_uvc->stop();
		}
		if(_soundcard)
		{
			_alock.lock();
		}
		_p = true;
		return true;
	}

	bool isplaying()
	{
		return _p == false ? true : false;

	}

	bool resume(bool closing = false)
	{
		if(_uvc)
		{
			_vlock.unlock();
			_uvc->start();
		}
		if(_soundcard)
		{
			_alock.unlock();
		}
		_b = closing;
		_p = false;
		return true;
	}
	bool seek(double incr)
	{
		/*can't seek camera*/
		return false;
	}

	bool has(avattr::avattr_type_string &&key)
	{
		if(key == avattr::frame_video)
		{
			if(_uvc)
			{
				return true;
			}
		}
		if(key == avattr::frame_audio)
		{
			if(_soundcard)
			{
				return true;
			}
		}
		return false;
	}
	void play()
	{
		resume();
	}
    duration_div duration()
    {
    	/*can't know duration */
    	return duration_div (-1,-1,-1,-1,-1);
    }
	virtual int take(const std::string &title, pixel &output)
	{
		if(!_uvc)
		{
			return -1;
		}
		autolock a(_vlock);
		_scheduler >> (output);
		if(output)
		{
			return 1;
		}
		return 0;
	}
	virtual int take(const std::string &title, pcm_require &output)
	{
		if(!_soundcard)
		{
			return -1;
		}
		autolock a(_alock);
		_scheduler >> (output);
		if(output.first.take<raw_media_data::type_size>())
		{
			return 1;
		}
		return 0;
	}
};


#endif
