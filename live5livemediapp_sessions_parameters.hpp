#pragma once
	class live5livemediapp_serversession_source;
	/*make session function you should be return class 'livemediapp_serversession_source'*/
		typedef std::function<live5livemediapp_serversession_source *(const std::string &target)> make_sessionsource_functor;
enum source_type
{
	source_type_uvc,
	source_type_file,
	source_type_rtspclient
};
struct live5livemediapp_server_session_parameters_in
/*
 	 parameter for rtsp server using value
 */
{
	enum source_type _type;
	std::string _target;
	make_sessionsource_functor _make_sessionsource;
	/*this session name for '_make_session()'*/
	std::string _session_name;
	/*shared source to client*/
	Boolean _reuse_source;
	std::string _sessioninfo;
   std::string _sessiondescription;
   Boolean _isssm;
   std::string _miscsdplines;

   live5livemediapp_server_session_parameters_in(enum source_type type,
		   char const *target,
		   make_sessionsource_functor &&make_sessionsource,
			std::string &&session_name,
			Boolean reuse_source = True,
			char const * sessioninfo = nullptr,
		   char const * sessiondescription = nullptr,
		   Boolean isssm = False,
		   char const *miscsdplines = nullptr) :
			   _type(type),
			   _target(target),
			   _make_sessionsource(make_sessionsource),
				_session_name(session_name),
				_reuse_source(reuse_source),
				_sessioninfo(sessioninfo ? sessioninfo : std::string("")),
				_sessiondescription(sessiondescription ? sessiondescription : std::string("")),
				_isssm(isssm),
				_miscsdplines(miscsdplines ? miscsdplines : std::string("")){}
	virtual ~live5livemediapp_server_session_parameters_in(){}
};
struct live5livemediapp_server_session_parameters_out
/*
 	 parameter for rtsp server output value
 */
{


	/* our rtsp server output datas*/
	typedef std::function<void (unsigned clientsessionid,
			enum AVMediaType mediatype,
			enum AVCodecID codecid,
			const unsigned char *streamdata,
			unsigned streamdata_size)> streamclient_functor;


 	streamclient_functor _streamclient;


 	live5livemediapp_server_session_parameters_out( streamclient_functor streamclient) :
			_streamclient(streamclient){}
	virtual ~live5livemediapp_server_session_parameters_out(){}
};
struct live5livemediapp_server_session_parameters
/*
 	 packing the parameters
 */
{
	typedef std::pair<live5livemediapp_server_session_parameters_in,
			live5livemediapp_server_session_parameters_out> session_parameters_pair;


	/*our server port*/
	Port _port;
	 Port &port()
	{
		return _port;
	}
	/*server's authentication if need*/
	std::string _authentication_id;
	 std::string &auth_id()
	{
		return _authentication_id;
	}
	/*server's authentication if need*/
	std::string _authentication_password;
	 std::string &auth_passwd()
	{
		return _authentication_password;
	}
	/*infinite service to client*/
	unsigned _reclamationSeconds;

	 unsigned &reclamationseconds()
	{
		return _reclamationSeconds;
	}
	/* client has incoming ip */
	typedef std::function<void (std::string clientip,
			unsigned clientsessionid)> playingclient_functor;

	/* client has teardown our server*/
	typedef std::function<void (std::string clientip,
			unsigned clientsessionid)> teardownclient_functor;
	playingclient_functor _playingclient;
 	teardownclient_functor _teardownclient;

	std::list<session_parameters_pair> _sessionparameters_pairs;

	live5livemediapp_server_session_parameters(const std::list<session_parameters_pair> &sessionparameters_pairs,
			playingclient_functor playingclient,
			teardownclient_functor teardownclient,
			Port port = 554,
			char const *authentication_id = nullptr,
			char const *authentication_password = nullptr,
			unsigned reclamationseconds = 65) :
				_port(port),
				_authentication_id(authentication_id ? authentication_id : std::string("")),
				_authentication_password(authentication_password ? authentication_password : std::string("")),
				_reclamationSeconds(reclamationseconds),
				_playingclient(playingclient),
				_teardownclient(teardownclient),
				_sessionparameters_pairs(sessionparameters_pairs){}
	virtual ~live5livemediapp_server_session_parameters(){}

};
inline const live5livemediapp_server_session_parameters_in &session_in_par(
		const live5livemediapp_server_session_parameters::session_parameters_pair &pair)
{
	return pair.first;
}
inline const live5livemediapp_server_session_parameters_out &session_out_par(
		const live5livemediapp_server_session_parameters::session_parameters_pair &pair)
{
	return pair.second;
}
