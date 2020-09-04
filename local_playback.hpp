#pragma once


/*
 	 playback local file

 	 have functional class  in 'local_playback' class, like inner class,
 	 because we'll not visible functional class to outside,  only using in playback local file
 */
class local_playback :
		public playback_inst
{
	enum  local_playback_state
	{
		local_playback_state_open = 0,
		local_playback_state_pause,
		local_playback_state_run,
		local_playback_state_close
	};




	struct framescheduler :
			public avframescheduler
			<pixelframe_presentationtime, pcmframe_presentationtime>
	{
			AVRational _videorational;
			AVRational _audiorational;
            int64_t _delta;
            int64_t _startdelta;
			framescheduler(const avattr &attr) :
				avframescheduler(attr),
				_videorational(AVRational{0, 0}),
                _audiorational(AVRational{0, 0}),
            _delta(0),
            _startdelta(0){ }
			virtual ~framescheduler(){}
		void set_rational(enum AVMediaType type, const AVRational &rational)
		{
			if(type == AVMEDIA_TYPE_VIDEO)
				set_video_rational(rational);
			else if(type == AVMEDIA_TYPE_AUDIO)
				set_audio_rational(rational);
		}
        virtual int64_t global_clock()
        {
            return avframescheduler
                        <pixelframe_presentationtime, pcmframe_presentationtime>::global_clock() ;
        }
        void start_delta()
        {
            _startdelta = global_clock();
        }
        void end_delta()
        {
            if(_startdelta)
            {
                _delta = global_clock() - _startdelta;

                _video[_video_type::clock_base_frame_pts_timer] += (double)(_delta / 1000000.0);

            }

        }
        void clear_delta()
        {
            _startdelta = _delta = 0;
        }

		void set_video_rational(const AVRational &rational)
		{ _videorational = rational; }
		void set_audio_rational(const AVRational &rational)
		{ _audiorational = rational; }
		virtual void usingframe( pixelframe_presentationtime &rf)
                { rf(); }
		
		virtual void usingframe( pcmframe_presentationtime &rf)
                { rf();}
	};



	class streamer;
	typedef std::pair<streamer *, std::mutex *> pair_streamer;
	/*
	 	 decoder functor
	 */
	struct stream :
			public ::decoder
	{
		/*
		 	 	 make frame functors
		 */
		struct functor_makeframe_video
		{
			void operator () (const avpacket_class & pkt,
					avframe_class &frm,
					void *puser)
			{
				/*
				 	 	 make frame
				 */

				pixelframe_presentationtime _pixelframe(*frm.raw(),
						((local_playback *)puser)->_framescheduler._videorational);

				/*
				 	 	 convert if different
				 */
				swxcontext_class (dynamic_cast<pixelframe &>(_pixelframe),
						((local_playback *)puser)->_attr);
				/*
				 	 put this
				 */
				autolock a(*((local_playback *)puser)->_streamers[AVMEDIA_TYPE_VIDEO].second);
				((local_playback *)puser)->_framescheduler << _pixelframe;

			}
		};
		struct functor_makeframe_audio
		{
			void operator () (const avpacket_class & pkt,
					avframe_class &frm,
					void *puser)
			{
				/*
				 	 	 make frame
				 */


				pcmframe_presentationtime _pcmframe(*frm.raw(),
						((local_playback *)puser)->_framescheduler._audiorational);

				/*
				 	 	 convert if different
				 */



				swxcontext_class (dynamic_cast<pcmframe &>(_pcmframe),
						((local_playback *)puser)->_attr);
				 
				/*
				 	 put frame
				 */
				autolock a(*((local_playback *)puser)->_streamers[AVMEDIA_TYPE_AUDIO].second);
				((local_playback *)puser)->_framescheduler << _pcmframe;

			}
		};
		enum functor_event { functor_event_reqeust_frame, 
			functor_event_waitdump, /*for confirm stream queue return time*/
			functor_event_close };
		union fucntor_par { enum functor_event e; };

		stream() :
			decoder(nullptr),_ptr(nullptr), _type(AVMEDIA_TYPE_UNKNOWN){ }
		stream (local_playback *ptr, const AVCodecParameters *codec) :
			decoder(codec), _ptr(ptr), _type(codec->codec_type){}
		virtual ~stream(){}

		bool operator ()
				(_dword r, void *par, _dword size, _dword *qtime)
		{
			if(r != WQOK)
			{
				return false;
			}
			union fucntor_par *_par =  (union fucntor_par *)par;
			if(_par->e == functor_event_waitdump)
			{
				/*confirm queue return time*/
				return true;
				
			}
			if(_par->e == functor_event_reqeust_frame)
			{
				bool makeframe = false;
				while(!makeframe)
				{
					avpacket_class  pkt;
					{
						std::lock_guard<std::mutex> a(this->_ptr->_mediacontainerlock);
						if(!_ptr->_mediacontainer.read_packet(_type, pkt))

						{
							break;
						}
					}
					if(_type == AVMEDIA_TYPE_VIDEO)
                                        {
						makeframe = decoder::operator()(pkt, functor_makeframe_video(), this->_ptr) > 0;

                                        }


					if(_type == AVMEDIA_TYPE_AUDIO)
						makeframe =  decoder::operator()(pkt, functor_makeframe_audio(), this->_ptr)  > 0;

						//usleep(1000 * 5);


				}
				return true;
			}
			/*
			 	 	 'functor_event_close'
			 */
			return false;
		}
	private:
		local_playback *_ptr;
		enum AVMediaType _type;

	};
	/*
	 	 stream manage class
	 */
	struct streamer :
			public wthread<stream>
	{
		streamer( streamer &&rhs):
                        wthread(dynamic_cast<wthread &&>(rhs))
		{
				/*
				 	 	 warnning !
				 	 	 no control mutex
				 	 	 we create first time copy constructor  and =
				 */
		}
		streamer(char const *name) :
                        wthread(10, sizeof (stream::fucntor_par), name){}
		virtual ~streamer()
		{
			stream::fucntor_par par;
			par.e = stream::functor_event_close;
			sendto(&par, sizeof(stream::fucntor_par));
		}

		void lock()
		{
                    _access.lock();

		}
		void unlock()
		{
                    _access.unlock();
		}
		void request_1frame()
		{
			stream::fucntor_par par;
			par.e = stream::functor_event_reqeust_frame;
			sendto(&par, sizeof(stream::fucntor_par));
		}
		void wait_dump()
		{
            stream::fucntor_par par;
			par.e = stream::functor_event_waitdump;
			sendto_wait(&par, sizeof(stream::fucntor_par));
		}

		void operator = (streamer &&rhs) = delete;

	private:
		std::mutex _access;/*access resume / puase*/
	};


	friend class stream;
	friend class streamer;
private:
	mediacontainer _mediacontainer;
	framescheduler _framescheduler;
	enum  local_playback_state _state;

	std::mutex _mediacontainerlock;
	std::map<enum AVMediaType, pair_streamer> _streamers;


public:
	local_playback(const avattr &attr, char const *name) :
		playback_inst(attr),
		_mediacontainer(name),
		_framescheduler(attr),
		_state(local_playback_state_open)
	{


		struct
		{ bool has; const AVStream *stream;char const *name; }
		open_test[AVMEDIA_TYPE_NB] =
		{
				/*
				 	 	 we can control 'audio' or 'video' current
				 */
				{!_attr.notfound(avattr::frame_video),
						_mediacontainer.find_stream(AVMEDIA_TYPE_VIDEO),
						"local playback video"},
				{!_attr.notfound(avattr::frame_audio),
						_mediacontainer.find_stream (AVMEDIA_TYPE_AUDIO),
						"local playback audio"},
				{false,
						nullptr,
						""},
				{false,
						nullptr,
						""},
				{false,
						nullptr,
						""}
		};
		for(unsigned i = 0; i < AVMEDIA_TYPE_NB; i++)
		{
			if(open_test[i].has &&
					open_test[i].stream)
			{
				_framescheduler.set_rational((enum AVMediaType)i,open_test[i].stream->time_base );
				_framescheduler.set_clock_master((enum AVMediaType)i);

				/*
				 	 start stream thread and ready to 1frame
				 */
				_streamers.insert(
						std::make_pair((enum AVMediaType )i,
								std::make_pair(new streamer(open_test[i].name),
										new std::mutex())));/*framescheduler's data lock*/

				_streamers[(enum AVMediaType )i].first->start(INFINITE, this, open_test[i].stream->codecpar);
				_streamers[(enum AVMediaType )i].first->request_1frame();
			}
		}
                DECLARE_THROW(_streamers.empty(), "can not found stream");

		pause();


	}
	virtual ~local_playback()
	{
		resume();		
		for(auto &it : _streamers)
		{
			delete it.second.first;
			delete it.second.second;
		}
		_streamers.clear();
	}
	enum AVMediaType get_master_clock()
	{
		return _framescheduler.get_clock_master();
	}
	void record(char const *file)
	{

	}
	bool pause()
	{
		/*
			we manage constructor has always 'pause'
		*/
		if(_state == local_playback_state_run || 
			_state == local_playback_state_open)
		{

			_state =  local_playback_state_pause;
               _framescheduler.start_delta();
			for(auto &it : _streamers)
			{
				it.second.first->lock();
			}
			return true;
		}		
		return false;
	}
	bool resume(bool closing = false)
	{
		if(_state == local_playback_state_pause)
		{
			if(!closing)_state = local_playback_state_run;
			else _state = local_playback_state_close;
            _framescheduler.end_delta();
			for(auto &it : _streamers)
			{
				it.second.first->unlock();
			}
			return true;
		}
		return false;
	}
    void resolution(int w, int h)
    {
        if(w > 0 &&
                h > 0)
        {
            /*
                 syncronize 'swscontext' to 'frame'.
                so.. user don't request when our resolution value changed
            */
            pause();

            for(auto &it : _streamers)
            {
                it.second.first->wait_dump();
            }

            /*if want change resolution */
            _attr.reset(avattr::width, avattr::width, w, (double)w);
            _attr.reset(avattr::height, avattr::height, h, (double)w);
            resume();
        }

    }
	bool seek(double incr)
	{
		double master_pts;
		enum AVMediaType master_type;
		/*first user 'take' blockking  */
		enum  local_playback_state prev_state = _state;

		pause();


		/*second wait for finish streamers queue's piled all event*/
		for(auto &it : _streamers)
		{
			it.second.first->wait_dump();
		}
		master_type	= _framescheduler.get_clock_master(&master_pts);
		/*exist data clear*/
		_framescheduler.data_clear();
		/* seeking */

		{
			std::lock_guard<std::mutex> a(_mediacontainerlock);
			/*not access 'this->get_master_clock()' because master pts value has not visible*/
			_mediacontainer.seek(master_type,incr, master_pts);
		}


		/*ready next frame*/
		for(auto &it : _streamers)
		{
			it.second.first->request_1frame();
		}

		/*if user state running then resume*/
		if(prev_state == local_playback_state_run ||
				prev_state == local_playback_state_open)
		{
			resume();
		}
        _framescheduler.clear_delta();
        return true;
	}

	bool has(avattr::avattr_type_string &&key)
	{
		return !_attr.notfound(key.c_str());
	}
	void play()
	{
		resume();
	}
	bool isplaying()
	{
		if(_state == local_playback_state_run)
		{
			return true;
		}
		return false;
	}
    duration_div duration()
    {
        bool has = false;
        return _mediacontainer.duration(has);
    }
	virtual int take(const std::string &title, pixel &output)
	{
                int res = -1;
		streamer *_targetstreamer = nullptr;
		std::mutex *_targetstream_lock = nullptr;

		if(!take(_targetstreamer,
				_targetstream_lock,
				AVMEDIA_TYPE_VIDEO))
		{
                        return res;
		}



		_targetstreamer->lock();
		if(_state == local_playback_state_run)
		{
			_targetstreamer->request_1frame();
			_targetstream_lock->lock();
			_framescheduler >> output;
			_targetstream_lock->unlock();
		}
        
		_targetstreamer->unlock();
		
        res = output ? 1 : 0;
        if(res == 0)
        {
            std::lock_guard<std::mutex> a(_mediacontainerlock);
            if(_mediacontainer.eof_stream(AVMEDIA_TYPE_VIDEO))
            {
                res = -1;
            }
        }
        return res;
	}
	virtual int take(const std::string &title, pcm_require &output)
	{
                int res = -1;
		streamer *_targetstreamer = nullptr;
		std::mutex *_targetstream_lock = nullptr;
		if(!take(_targetstreamer,
				_targetstream_lock,
				AVMEDIA_TYPE_AUDIO))
		{
                        return res;
		}

		/*
			access resume / pause 
		*/
		_targetstreamer->lock();
		if(_state == local_playback_state_run)
		{

			_targetstreamer->request_1frame();

			/*
			 	access data
			*/
			_targetstream_lock->lock();

			_framescheduler >> output;

			_targetstream_lock->unlock();
		}
        _targetstreamer->unlock();

        res = output.first ? 1 : 0;
        if(res == 0)
        {
            std::lock_guard<std::mutex> a(_mediacontainerlock);
            if(_mediacontainer.eof_stream(AVMEDIA_TYPE_AUDIO))
            {
                    res = -1;
            }
        }

        return res;
	}
private:
	bool take(streamer *&_targetstreamer,
			std::mutex *&_targetstream_lock,
			enum AVMediaType type)
	{
		auto it = std::find_if(_streamers.begin(), _streamers.end(),
				[&type](const std::pair<enum AVMediaType, pair_streamer> &p)->
				bool {
						return p.first == type;
		});

		if(it == _streamers.end())
		{
			return false;
		}

		_targetstreamer = (*(it)).second.first;
		_targetstream_lock = (*(it)).second.second;

		return true;
	}
};



