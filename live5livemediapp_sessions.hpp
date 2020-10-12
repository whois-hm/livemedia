#pragma once

class livemediapp_proxy_serversession :  public ProxyServerMediaSession
{
	const live5livemediapp_server_session_parameters::session_parameters_pair &_parameter;
	class livemediapp_proxy_mediatranscodingtable : public MediaTranscodingTable
	{
		friend class livemediapp_proxy_serversession;
		const live5livemediapp_server_session_parameters::session_parameters_pair &_parameter;
		livemediapp_proxy_mediatranscodingtable(UsageEnvironment& env,
				const live5livemediapp_server_session_parameters::session_parameters_pair &parameter) :
			MediaTranscodingTable(env),
			_parameter(parameter){ }
		virtual ~livemediapp_proxy_mediatranscodingtable() { }
	  virtual FramedFilter*
	  lookupTranscoder(MediaSubsession& in,
			   char*& outputCodecName)
	  {
			live5livemediapp_serversession_source_rtspclient *ourfilter =
					(live5livemediapp_serversession_source_rtspclient *)session_in_par(_parameter)._make_sessionsource(session_in_par(_parameter)._target);
			/*
			 	 media type && codecid check previous in 'allowProxyingForSubsession'.
			 	 we just create this mediasubsession 's filter
			 */
			FramedFilter *f = ourfilter->create_filter(envir(),
					in.readSource(),/*it client session initiated source*/
					media_type_string_support_map()(in.mediumName()),
					codec_type_string_support_map()(in.codecName()),
					in.fmtp_spropparametersets());
			delete ourfilter;
			outputCodecName = strDup(in.codecName());
		return f;
	  }
	  virtual Boolean weWillTranscode(char const* /*mediumName*/, char const* /*codecName*/)
	  {
	    return True;
	  }

		static MediaTranscodingTable *createnew(UsageEnvironment& env,
				const live5livemediapp_server_session_parameters::session_parameters_pair &parameter)
		{
			return new livemediapp_proxy_mediatranscodingtable(env, parameter);
		}
	};
public:
	livemediapp_proxy_serversession(UsageEnvironment& env,
				const live5livemediapp_server_session_parameters::session_parameters_pair &parameter,
				GenericMediaServer* ourMediaServer,
			  char const* inputStreamURL,
			  char const* streamName = nullptr,
			  char const* username = nullptr,
			  char const* password = nullptr,
			  portNumBits tunnelOverHTTPPortNum = 0,
			  int verbosityLevel = 10,
			  int socketNumToServer = -1,
			  createNewProxyRTSPClientFunc* ourCreateNewProxyRTSPClientFunc
			  = defaultCreateNewProxyRTSPClientFunc,
			  portNumBits initialPortNum = 6970,
			  Boolean multiplexRTCPWithRTP = False) :
		ProxyServerMediaSession(env,
				ourMediaServer,
				inputStreamURL,
				streamName,
				username,
				password,
				tunnelOverHTTPPortNum,
				verbosityLevel,
				socketNumToServer,
				livemediapp_proxy_mediatranscodingtable::createnew(env,
						parameter),
				ourCreateNewProxyRTSPClientFunc,
				initialPortNum,
				multiplexRTCPWithRTP),
				_parameter(parameter) { }
	virtual ~livemediapp_proxy_serversession(){}
	virtual Boolean allowProxyingForSubsession(MediaSubsession const& mss)
	{
		live5livemediapp_serversession_source_rtspclient *ourfilter =
				(live5livemediapp_serversession_source_rtspclient *)session_in_par(_parameter)._make_sessionsource(session_in_par(_parameter)._target);
		enum AVMediaType ssmediatype = media_type_string_support_map()(mss.mediumName());

		Boolean res = False;
		do
		{
			if(ssmediatype == AVMEDIA_TYPE_VIDEO)
			{
				if((ourfilter->videocodec() != AV_CODEC_ID_NONE) &&
						ourfilter->videocodec() ==
						codec_type_string_support_map()(mss.codecName()))
				{
					res = True;
					break;
				}
			}
			if(ssmediatype == AVMEDIA_TYPE_AUDIO)
			{
				if((ourfilter->audiocodec() != AV_CODEC_ID_NONE) &&
						ourfilter->audiocodec() ==
						codec_type_string_support_map()(mss.codecName()))
				{
					res = True;
					break;
				}
			}
		}while(0);
		delete ourfilter;
		return res;
	}
};
/*--------------------------------------------------------------------------------
 our server session
 --------------------------------------------------------------------------------*/
class livemediapp_serversession : public ServerMediaSession
{
private:
		class mediafiltered_source : public FramedSource
		{
		/*--------------------------------------------------------------------------------
		 filter source interface : source and serversession interface
		 --------------------------------------------------------------------------------*/
			livemediapp_serversession &_ourserversession;/*just reference*/
			enum AVMediaType _mediaid;
			TaskToken _misstoken;/*we reading retry using 'timer'*/
			unsigned _clientid;
		public:
			mediafiltered_source(
					livemediapp_serversession &ourserversession,
					enum AVMediaType mediaid,
					unsigned clientid) :
					FramedSource(ourserversession.envir()),
					_ourserversession(ourserversession),
					_mediaid(mediaid),
					_misstoken(nullptr),
					_clientid(clientid){ }
			virtual ~mediafiltered_source()
			{
					stop_misstoken();
			}
			unsigned clientid()
			{
				return _clientid;
			}
			void stop_misstoken()
			{
				if(_misstoken)
				{
					envir().taskScheduler().unscheduleDelayedTask(_misstoken);
					_misstoken = nullptr;
				}
			}
			void retry(unsigned nextime/*us*/)
			{
				stop_misstoken();
				_misstoken = envir().taskScheduler().scheduleDelayedTask(nextime,
						[](void* c)->void{((mediafiltered_source *)c)->doGetNextFrame();},
						this);
			}
			virtual void doGetNextFrame()
			{
				do
				{
					if(!isCurrentlyAwaitingData())
					{
						/*if our server session scheduler so fast*/
						break;
					}
					fFrameSize = _ourserversession.doGetNextFrame(_clientid,
						_mediaid,
							fTo,
							fMaxSize,
							&fNumTruncatedBytes,
							&fPresentationTime,
							&fDurationInMicroseconds);
					if(fFrameSize <= 0)
					{
						/*if our server session buffer too slow*/
						break;
					}
					/*
						now our source can be deliver to live555 scheduler
					*/
					FramedSource::afterGetting(this);
					return;
				}while(0);
				/*
					 do retry if can't reading from source
				 */
				retry(0);
			}
			virtual void doStopGettingFrames()
			{
				stop_misstoken();
				_ourserversession.doStopGettingFrames(_clientid,
					_mediaid);
			}
			void seekFrame(char*& absStart, char*& absEnd)
			{
				_ourserversession.seekFrame(_clientid,
					absStart,
					absEnd);
			}
			void seekFrame(double& seekNPT, double streamDuration, u_int64_t& numBytes)
			{
				_ourserversession.seekFrame(_clientid,
					seekNPT,
					streamDuration, 
					numBytes);
			}
				
		};
		

	/*--------------------------------------------------------------------------------
	 our server media subsessions
	 --------------------------------------------------------------------------------*/

	class livemediapp_servermediasubession :
			public OnDemandServerMediaSubsession
	{
			/*based subsession*/
	protected:
		livemediapp_serversession &_serversession;
		enum AVMediaType _type;/*for debug print*/


		livemediapp_servermediasubession(livemediapp_serversession &serversession,
			enum AVMediaType type,
			Boolean bresuse) :
			OnDemandServerMediaSubsession(serversession.envir(), bresuse),
				_serversession(serversession),
				_type(type) {}
		virtual ~livemediapp_servermediasubession()
		{
	
		}
		virtual void closeStreamSource(FramedSource *inputSource) = 0;
		virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
							  unsigned& estBitrate) = 0;
		virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
						unsigned char rtpPayloadTypeIfDynamic,
						FramedSource* inputSource) = 0;
		  virtual void startStream(unsigned clientSessionId, void* streamToken,
					   TaskFunc* rtcpRRHandler,
					   void* rtcpRRHandlerClientData,
					   unsigned short& rtpSeqNum,
								   unsigned& rtpTimestamp,
					   ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
								   void* serverRequestAlternativeByteHandlerClientData)
		  {
			  OnDemandServerMediaSubsession::startStream(clientSessionId,
					  streamToken,
					  rtcpRRHandler,
					  rtcpRRHandlerClientData,
					  rtpSeqNum,
					  rtpTimestamp,
					  serverRequestAlternativeByteHandler,
					  serverRequestAlternativeByteHandlerClientData);
		  }
		virtual void pauseStream(unsigned clientSessionId, void *streamToken)
		{
			/*
				we have nothing to do. because the sink has no request source's data.
			*/

			OnDemandServerMediaSubsession::pauseStream(clientSessionId, streamToken);
		}
		virtual void seekStreamSource(FramedSource* inputSource, double& seekNPT, double streamDuration, u_int64_t& numBytes)
		{


				/*
					OnDemandServerMediaSubsession has no imple
				*/
			OnDemandServerMediaSubsession ::seekStreamSource (inputSource, seekNPT, streamDuration, numBytes);
			mediafiltered_source *_filtersource = dynamic_cast<mediafiltered_source *>(inputSource);
			if(_filtersource)/*reference for client id*/
			{
				_filtersource->seekFrame(seekNPT, streamDuration, numBytes);
			}

		}

	  virtual void seekStreamSource(FramedSource* inputSource, char*& absStart, char*& absEnd)
		{

				/*
					OnDemandServerMediaSubsession has no imple
				*/
			OnDemandServerMediaSubsession::seekStreamSource(inputSource, absStart, absEnd);
			mediafiltered_source *_filtersource = dynamic_cast<mediafiltered_source *>(inputSource);
			if(_filtersource)/*reference for client id*/
			{
				_filtersource->seekFrame(absStart, absEnd);
			}
		}

	  virtual void setStreamSourceScale(FramedSource* inputSource, float scale)
		{
		OnDemandServerMediaSubsession::setStreamSourceScale(inputSource, scale);
		}
	  virtual void setStreamSourceDuration(FramedSource* inputSource, double streamDuration, u_int64_t& numBytes)
		{
			OnDemandServerMediaSubsession::setStreamSourceDuration(inputSource, streamDuration, numBytes);
		}
	};
		/*--------------------------------------------------------------------------------
		 server media subsession for h264 frame
		 --------------------------------------------------------------------------------*/
		class livemediapp_h264_servermediasubsession : public livemediapp_servermediasubession
		{
		public:
			livemediapp_h264_servermediasubsession(livemediapp_serversession &serversession, Boolean breuse) :
				livemediapp_servermediasubession(serversession, AVMEDIA_TYPE_VIDEO, breuse){}
			virtual ~livemediapp_h264_servermediasubsession(){}
			virtual void closeStreamSource(FramedSource *inputSource)
			{

				H264VideoStreamFramer *source = (H264VideoStreamFramer *)inputSource;
				mediafiltered_source *filter = (mediafiltered_source*)source->inputSource();
				unsigned clientsessionid = filter->clientid();
				/*
					close source for delete serversession's source dependency
				*/
				Medium::close(source);


				if(clientsessionid) _serversession.closeStreamSource(clientsessionid);
			}
			virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
									  unsigned& estBitrate)
			{
			/*
				ondemand create dummy sink for sdp using 'clinetSessionId = 0'
				so we do not create last source for speed with memory
			*/
				if(clientSessionId) _serversession.createNewStreamSource(clientSessionId);
				return H264VideoStreamFramer::createNew(envir(),new mediafiltered_source(_serversession,
					AVMEDIA_TYPE_VIDEO,
					clientSessionId), False);
			}
			virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
							unsigned char rtpPayloadTypeIfDynamic,
							FramedSource* inputSource)
			{
				 return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
			}
		};
		/*--------------------------------------------------------------------------------
		 server media subsession for aac (adts)format
		 --------------------------------------------------------------------------------*/
		class livemediapp_adts_audio_servermediasubsession : public livemediapp_servermediasubession
		{
			int profile;
			int frequency;
			int channel;
		public:
			livemediapp_adts_audio_servermediasubsession(livemediapp_serversession &serversession, const AVCodecContext *codecinfo, Boolean breuse) :
						livemediapp_servermediasubession(serversession, AVMEDIA_TYPE_AUDIO, breuse),
			 profile(codecinfo->profile),
			 frequency(codecinfo->sample_rate),
			 channel(codecinfo->channels) { 	}
			virtual ~livemediapp_adts_audio_servermediasubsession(){}
			virtual void closeStreamSource(FramedSource *inputSource)
			{
				unsigned clientid = 0;
				mediafiltered_source *filter = (mediafiltered_source*)inputSource;
				clientid = filter->clientid();
				/*
					close source for delete serversession's source dependency
				*/
				Medium::close(filter);

				if(clientid)_serversession.closeStreamSource(clientid);
			}
			virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
									  unsigned& estBitrate)
			{
				/*
				ondemand create dummy sink for sdp using 'clinetSessionId = 0'
				so we do not create last source for speed with memory
			*/

			 if(clientSessionId) _serversession.createNewStreamSource(clientSessionId);
				return new mediafiltered_source(_serversession,
					AVMEDIA_TYPE_AUDIO,
					clientSessionId);
			}
			virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
							unsigned char rtpPayloadTypeIfDynamic,
							FramedSource* inputSource)
			{
				  // Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
				int frequencyindex = 0;
				if(96000 == frequency) frequencyindex = 0;
				else if(88200 == frequency) frequencyindex = 1;
				else if(64000 == frequency) frequencyindex = 2;
				else if(48000 == frequency) frequencyindex = 3;
				else if(44100 == frequency) frequencyindex = 4;
				else if(32000 == frequency) frequencyindex = 5;
				else if(24000 == frequency) frequencyindex = 6;
				else if(22050 == frequency) frequencyindex = 7;
				else if(16000 == frequency) frequencyindex = 8;
				else if(12000 == frequency) frequencyindex = 9;
				else if(11025 == frequency) frequencyindex = 10;
				else if(8000 == frequency) frequencyindex = 11;
				else if(7350 == frequency) frequencyindex = 12;
				else if(0 == frequency) frequencyindex = 13;
				else if(0 == frequency) frequencyindex = 14;
				else if(0 == frequency) frequencyindex = 15;

				 unsigned char audioSpecificConfig[2];
				 char fConfigStr[5];
				 u_int8_t const audioObjectType = profile + 1;
				 audioSpecificConfig[0] = (audioObjectType<<3) | (frequencyindex>>1);
				 audioSpecificConfig[1] = (frequencyindex<<7) | (channel<<3);
				 sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

				return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, frequency, "audio", "AAC-hbr", fConfigStr, channel);
			}
		};

	virtual void closeStreamSource(unsigned clientSessionId)
	{  
		/*calling subsession*/
		std::list<live5livemediapp_serversession_source *>::iterator it;

		it = _sources.begin();
		while(it != _sources.end())
		{
			if((*it)->get_unique() == clientSessionId)
			{
				(*it)->reference_decrease();
				if((*it)->reference() <= 0)
				{
					live5livemediapp_serversession_source *delsource = *it;
					_sources.erase(it);/*broken 'it' but next just break.*/
					delete delsource;
					
				}
				break;
			}
			else ++it;
		}

	}
	
	virtual  void createNewStreamSource(unsigned clientSessionId)
	{
	/*calling subsession*/
		live5livemediapp_serversession_source *fsource = nullptr;
		/*
			duplicated check
		*/
		for(auto &it : _sources)
		{
			if(it->get_unique() == clientSessionId)
			{
				fsource = it;
				break;
			}
		}
		/*
			create source if first
		*/
		if(!fsource)
		{
			fsource = session_in_par(_parameter)._make_sessionsource(session_in_par(_parameter)._target);
			/*
				_source pointer has always true , 
				because we check when 'create_subsessions()' function called 
			*/
			fsource->set_unique(clientSessionId);
			fsource->reference_increase();
			_sources.push_back(fsource);
		}
		/*
			reference add if exist source
		*/
		else
		{
			fsource->reference_increase();
		}

	}

	int doGetNextFrame(unsigned clientsessionid, 
		enum AVMediaType type,
		unsigned char*to,
		unsigned size,
		unsigned *numtruncatedbyte,
		struct timeval *presentationtime,
		unsigned *durationinmicroseconds)
	{
	/*calling filtered source*/
		int readsize = 0;
		find_and(clientsessionid,
			[&](live5livemediapp_serversession_source *t)->void{

			readsize =  t->doGetNextFrame(type,
				to,
				size,
				numtruncatedbyte,
				presentationtime,
				durationinmicroseconds);

			if(session_out_par(_parameter)._streamclient)
			{
				session_out_par(_parameter)._streamclient(clientsessionid,
						type,
						type == AVMEDIA_TYPE_AUDIO ? t->audiocodec() : t->videocodec(),
						to,
						readsize);
			}
		});
		return readsize;
	}
	virtual void doStopGettingFrames(unsigned clientsessionid, 
		enum AVMediaType type)
	{
	/*calling filtered source*/
		find_and(clientsessionid,
			[&](live5livemediapp_serversession_source *t)->void{
			t->doStopGettingFrames(type);
		});
	}
	void seekFrame(unsigned clientsessionid, 
		char*& absStart, 
		char*& absEnd)
	{
	/*calling filtered source*/
		find_and(clientsessionid,
			[&](live5livemediapp_serversession_source *t)->void{
			t->seekFrame(absStart, absEnd);
		});
		
	}
	void seekFrame(unsigned clientsessionid, 
		double& seekNPT, 
		double streamDuration, 
		u_int64_t& numBytes)
	{
	/*calling filtered source*/
		find_and(clientsessionid,
			[&](live5livemediapp_serversession_source *t)->void{
			t->seekFrame(seekNPT, streamDuration, numBytes);
		});
	}
	template <typename functor>
	void find_and(unsigned clientsessionid,
		functor && fn)
	{
		for(auto &it : _sources)
		{
			if(it->get_unique() == clientsessionid)
			{
				fn(it);
				break;
			}
		}
	}
    bool create_subsessions()
	{
		/*tell subsession*/
    	live5livemediapp_serversession_source * session_source = session_in_par(_parameter)._make_sessionsource(session_in_par(_parameter)._target);
    	if(session_source &&
    			session_source->videocodec() == AV_CODEC_ID_H264)
		{
    		this->addSubsession(new livemediapp_h264_servermediasubsession(*this,
    				session_in_par(_parameter)._type == source_type_uvc ? true :
    						session_in_par(_parameter)._reuse_source));
		}
    	if(session_source &&
    			session_source->audiocodec() == AV_CODEC_ID_AAC)
    	{
			this->addSubsession(new livemediapp_adts_audio_servermediasubsession(*this,
					session_source->codec(AVMEDIA_TYPE_AUDIO),/*get parameter audio spec*/
					session_in_par(_parameter)._type == source_type_uvc ? true :
				    						session_in_par(_parameter)._reuse_source));
    	}
    	if(session_source)
    	{
    		/*just tell to create*/
    		delete session_source;
    	}
		return numSubsessions() > 0;
	}
    live5livemediapp_server_session_parameters::session_parameters_pair _parameter;
	std::list<live5livemediapp_serversession_source *>_sources;
public:
	static ServerMediaSession *createnew( UsageEnvironment& env,
				const live5livemediapp_server_session_parameters::session_parameters_pair &parameter,
				RTSPServer *ourserver)

	{

		ServerMediaSession *newsession = nullptr;
		if(session_in_par(parameter)._type == source_type_uvc)
		{
			newsession = new livemediapp_serversession( env, parameter);
			DECLARE_THROW(!((livemediapp_serversession * )newsession)->create_subsessions(), "server create fail, no found stream");

		}
		if(session_in_par(parameter)._type == source_type_file)
		{
			newsession = new livemediapp_serversession( env, parameter);
			DECLARE_THROW(!((livemediapp_serversession * )newsession)->create_subsessions(), "server create fail, no found stream");
		}
		if(session_in_par(parameter)._type == source_type_rtspclient)
		{
			newsession = new livemediapp_proxy_serversession( env, parameter, ourserver,
					session_in_par(parameter)._target.c_str(),
					session_in_par(parameter)._session_name.c_str());
		}

		return newsession;
	}
	livemediapp_serversession(UsageEnvironment& env,
				const live5livemediapp_server_session_parameters::session_parameters_pair &parameter) :
		    	   ServerMediaSession(env,
		    		session_in_par(parameter)._session_name.c_str(),
					session_in_par(parameter)._sessioninfo.c_str(),
					session_in_par(parameter)._sessiondescription.c_str(),
					session_in_par(parameter)._isssm,
					session_in_par(parameter)._miscsdplines.c_str()),
					_parameter(parameter){ }
	virtual ~livemediapp_serversession() { }
};


