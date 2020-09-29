#pragma once

class live5rtspserver : public RTSPServer
{
	/*
 	 	 refernce code 'live555/DynamicRTSPServer/createNewSMS'
	 */
	class live5clientconnection : public RTSPServer::RTSPClientConnection
	{

	public:
		const struct sockaddr_in &get_addr()
		{
			return fClientAddr;
		}
		live5clientconnection(RTSPServer& ourServer, int clientSocket, struct sockaddr_in clientAddr) :
			RTSPServer::RTSPClientConnection(ourServer, clientSocket, clientAddr)
		{
			printf("new client was incoming... (%s)(%d)\n", inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port);
		}
		virtual ~live5clientconnection(){}

	    virtual void handleCmd_OPTIONS()
	    {
	    	printf("\"option\" command recv from (%s)\n", inet_ntoa(fClientAddr.sin_addr));
	    	RTSPServer::RTSPClientConnection::handleCmd_OPTIONS();
	    }
	    virtual void handleCmd_DESCRIBE(char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr)
	    {
	    	printf("\"describe\" command recv from (%s)\n", inet_ntoa(fClientAddr.sin_addr));
	    	RTSPServer::RTSPClientConnection::handleCmd_DESCRIBE(urlPreSuffix,  urlSuffix, fullRequestStr);
	    }

	};
	class live5clientsession : public RTSPServer::RTSPClientSession
	{
	public:
		live5clientsession(RTSPServer& ourServer, u_int32_t sessionId) :
			RTSPServer::RTSPClientSession(ourServer, sessionId) { }
		virtual ~live5clientsession() { }
	   virtual void handleCmd_SETUP(RTSPClientConnection* ourClientConnection,
					 char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr)
	   {
		   live5clientconnection *con = (live5clientconnection *)ourClientConnection;
		   printf("\"setup\" command recv from (%s)\n", inet_ntoa(con->get_addr().sin_addr));
		   RTSPServer::RTSPClientSession::handleCmd_SETUP(ourClientConnection, urlPreSuffix, urlSuffix, fullRequestStr);
	   }
		virtual void handleCmd_TEARDOWN(RTSPClientConnection* ourClientConnection,
						ServerMediaSubsession* subsession)
		{
			live5clientconnection *con = (live5clientconnection *)ourClientConnection;
			printf("\"teardown\" command recv from (%s)\n", inet_ntoa(con->get_addr().sin_addr));
			RTSPServer::RTSPClientSession::handleCmd_TEARDOWN(ourClientConnection, subsession);

			if(((live5rtspserver *)&fOurRTSPServer)->_session_parameters._teardownclient)
			{
				((live5rtspserver *)&fOurRTSPServer)->_session_parameters._teardownclient(
						std::string(inet_ntoa(con->get_addr().sin_addr)),
						fOurSessionId);
			}
		}
		virtual void handleCmd_PLAY(RTSPClientConnection* ourClientConnection,
					ServerMediaSubsession* subsession, char const* fullRequestStr)
		{
			live5clientconnection *con = (live5clientconnection *)ourClientConnection;
			printf("\"play\" command recv from (%s)\n", inet_ntoa(con->get_addr().sin_addr));
			RTSPServer::RTSPClientSession::handleCmd_PLAY(ourClientConnection, subsession, fullRequestStr);
			if(((live5rtspserver *)&fOurRTSPServer)->_session_parameters._playingclient)
			{
				((live5rtspserver *)&fOurRTSPServer)->_session_parameters._playingclient(
						std::string(inet_ntoa(con->get_addr().sin_addr)),
						fOurSessionId);
			}

		}
	    virtual void handleCmd_PAUSE(RTSPClientConnection* ourClientConnection,
					 ServerMediaSubsession* subsession)
	    {
	    	live5clientconnection *con = (live5clientconnection *)ourClientConnection;
	    	printf("\"pause\" command recv from (%s)\n", inet_ntoa(con->get_addr().sin_addr));
	    	RTSPServer::RTSPClientSession::handleCmd_PAUSE(ourClientConnection, subsession);
	    }

	};
	virtual ClientConnection* createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr)
	{
		return new live5clientconnection(*this, clientSocket, clientAddr);
	}

	virtual ClientSession* createNewClientSession(u_int32_t sessionId)
	{
		return new live5clientsession(*this, sessionId);
	}

public:
	live5rtspserver( UsageEnvironment& env,
			live5livemediapp_server_session_parameters &parameter) :
		    RTSPServer(env, GenericMediaServer::setUpOurSocket(env, parameter.port()),
			 parameter._port,
			 nullptr,
			 parameter._reclamationSeconds),
			 _session_parameters(parameter)
	{

		DECLARE_THROW(fServerSocket == -1, "can't use socket port");

		if(!_session_parameters._authentication_id.empty() &&
				!_session_parameters._authentication_password.empty())
		{
			UserAuthenticationDatabase *authdb = new UserAuthenticationDatabase;
			authdb->addUserRecord(_session_parameters._authentication_id.c_str(),
					_session_parameters._authentication_password.c_str());
			setAuthenticationDatabase(authdb);
		}

		ServerMediaSession *livemediappsession = nullptr;

		for(auto &it : parameter._sessionparameters_pairs)
		{
			livemediappsession = livemediapp_serversession::createnew(env, it, this);
			DECLARE_THROW(!livemediappsession, "can't create livemedipp serversession..");
			addServerMediaSession(livemediappsession);
			char *rtspurl = rtspURL(lookupServerMediaSession(session_in_par(it)._session_name.c_str()));
			delete [] rtspurl;
		}
	}
	virtual ~live5rtspserver()
	{

	}

private:
	live5livemediapp_server_session_parameters _session_parameters;

};
