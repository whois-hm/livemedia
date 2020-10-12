#pragma once
class dlog
{


	class logout :
			public filter<std::string , dlog>
	{
		friend class dlog;
		std::list<std::string > _filters;
		bool in_the_filter(std::string &str) const
		{
			if(_filters.size() <= 0)
			{
				return true;
			}
			for(auto &it : _filters)
			{
				if(it == str)
				{
					return true;
				}
			}
			return false;
		}

		public:
		logout(dlog *d) : filter<std::string, dlog>(d){}
			virtual ~logout(){}
		void add(std::string str)
		{
			_filters.push_back(str);
		}
		void sub(std::string str)
		{
			_filters.erase(std::remove_if(_filters.begin(), _filters.end(),
								[&](std::string  &instr)->bool{
								if(instr == std::string(str))
								{
									return true;
								}
								return false;
							}
						));
		}
		void clear()
		{
			_filters.clear();
		}

		virtual void operator >>(std::string &pf)
		{
			int total_size = pf.size();

			if(!in_the_filter(pf))
			{
				return;
			}
			if(ptr()->_console_write)
			{
				if(ptr()->_color_prefix)
				{
					printf("%s",ptr()->_color_prefix);

				}
				printf("%s", pf.c_str());
				if(ptr()->_color_prefix)
				{
					printf("\x1b[0m");
				}
			}
			if(ptr()->_file)
			{
				fwrite(pf.c_str(), sizeof(char), total_size, ptr()->_file.get());
			}

			if(ptr()->_client_sock != -1)
			{
				sendto(ptr()->_client_sock,
					pf.c_str(),
					total_size,
					MSG_DONTWAIT | MSG_NOSIGNAL,
					(struct sockaddr *)&ptr()->_serveraddr,
					sizeof(ptr()->_serveraddr));
			}
			if(ptr()->_usrout_functor)
			{
				ptr()->_usrout_functor(pf);
			}
		}
	};
public:

	enum color_set
	{
		RED,
		GREEN,
		YELLOW,
		BLUE,
		MAGENTA,
		CYAN,
		RESET,
		MAX,
	};
	enum level
	{
		normal,
		warnning,
		critical
	};
	dlog(const dlog &d) = delete;
	dlog(dlog &&d) = delete;
	dlog &operator()(const dlog &d) = delete;
	dlog &operator()(dlog &&d) = delete;
	dlog() :
		_out(this),
		_server_sock(-1),
		_client_sock(-1),
		_file(),
		_console_write(false),
		_level(normal),
		_out_buffer_size(0),
		_serverthread(),
		_color_prefix(nullptr),
		_usrout_functor(nullptr)
	{

		_prefix.clear();
		memset(&_serveraddr,
			0, 
			sizeof(struct sockaddr_in));
		_pipe[0]= _pipe[1] = -1;
		udp_receiver_server_uninstall();
		udp_sender_clinet_uninstall();
		file_writer_uninstall();
		console_writer_uninstall();
		prefix_uninstall();
		outbuffer_increase(0);
	}
	virtual ~dlog()
	{
		udp_receiver_server_uninstall();
		udp_sender_clinet_uninstall();
		file_writer_uninstall();
		console_writer_uninstall();
		prefix_uninstall();
		outbuffer_increase(0);
	}

	void log_enable()
	{
		_out.enable();
	}
	void log_disable()
	{
		_out.disable();
	}
	
	void udp_receiver_server_uninstall()
	{
		if(_pipe[1] != -1)
		{
			char d = 1;
			write(_pipe[1], &d, 1);
		}
		_serverthread.reset();

		if(_pipe[1] != -1)
		{
			close(_pipe[1]);
			_pipe[1]= -1;
		}
		if(_pipe[0] != -1)
		{
			close(_pipe[0]);
			_pipe[0]= -1;
		}
		if(_server_sock != -1)
		{
			close(_server_sock);
			_server_sock = -1;
		}
	}
	template <class functor>
	bool udp_receiver_server_install(char const *ip, int port, functor &&_f)
	{
		int opt = 0;
		int tempsock = -1;
		if(!ip || 
			port < 0)
		{
			return false;
		}

		struct sockaddr_in   server_addr;		 
		tempsock = socket(PF_INET, SOCK_DGRAM, 0);
		if(tempsock < 0)
		{
			return false;
		}

		memset(&server_addr, 0, sizeof(struct sockaddr_in));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		server_addr.sin_addr.s_addr = inet_addr(ip);

		if(bind(tempsock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		{
			close(tempsock);
			tempsock = -1;
			return false;
		}
		if(!socket_make_nonblock(tempsock))
		{
			close(tempsock);
			tempsock = -1;
			return false;
		}

		udp_receiver_server_uninstall();		
		
		pipe(_pipe);
		_server_sock = tempsock;
		_serverthread.reset(new std::thread([&]()->void{
				fd_signal_detector d;
				while(1)
				{
					d.clear();
					d.set(_server_sock);
					d.set(_pipe[0]);
					int res = d.sigwait(-1);
					if(res == -1 || 
						d.issetbit_err(_server_sock) || 
						d.issetbit_err(_pipe[0]))
					{
						return;
					}
					if(res == 0)
					{
						continue;
					}
					if(d.issetbit_in(_pipe[0]))
					{
						return;
					}
					if(d.issetbit_in(_server_sock))
					{
						struct sockaddr_in clientaddr;
						int clientaddr_size = sizeof(clientaddr);
						if(_out_buffer_size > 0)
						{
							int res = 0;
							char out_buffer[_out_buffer_size] = {0, };
							
							res = recvfrom(_server_sock, 
								out_buffer,
								_out_buffer_size,
								MSG_NOSIGNAL, 
								(struct sockaddr *)&clientaddr,
								&clientaddr_size);
							if(res > 0)
							{
								out_buffer[_out_buffer_size - 1]= 0;
								f(inet_ntoa(clientaddr.sin_addr, out_buffer));
							}
						}
						continue;
					}					
				}

				
			}), _delete_thread());
		return true;
	}
	void udp_sender_clinet_uninstall()
	{

		if(_client_sock != -1)
		{
			close(_client_sock);
			_client_sock = -1;
		}		
	}
	bool udp_sender_client_install(std::string serverip, int serverport)
	{
		int tempsock = -1;
		
		if(serverip.size() <= 0 || serverport < 0)
		{
			return false;
		}
		tempsock = socket(PF_INET, SOCK_DGRAM, 0);
		if(tempsock < 0)
		{
			return false;
		}
		memset(&_serveraddr, 0, sizeof(_serveraddr));
		_serveraddr.sin_family = AF_INET;
		_serveraddr.sin_addr.s_addr = inet_addr(serverip.c_str());
		_serveraddr.sin_port = htons(serverport);
		
		udp_sender_clinet_uninstall();

		_client_sock = tempsock;

	}
	void userout_writer_install(std::function<void (std::string &)> &&l)
	{
		_usrout_functor = l;
	}
	void userout_writer_uninstall()
	{
		_usrout_functor = nullptr;
	}
	void file_writer_uninstall()
	{
		_file.reset();
	}
	bool file_writer_install(std::string file, bool boverwrite)
	{
		FILE *tempfile = nullptr;
		if(file.size() <= 0)
		{
			return false;
		}
		
		tempfile = fopen(file.c_str(), boverwrite ? "w" : "a");
		if(!tempfile)
		{
			return false;
		}
		file_writer_uninstall();

		_file.reset(tempfile, _delete_file());
		return true;
	}
	void console_writer_uninstall()
	{
		_console_write = false;
	}
	bool console_writer_install()
	{
		console_writer_uninstall();
		_console_write = true;
		return true;
	}
	void prefix_uninstall()
	{
		_prefix.clear();
	}
	void prefix_install(char const *prefix)
	{
		prefix_uninstall();
		
		_prefix = prefix;
	}
	void level_install(enum level lvl)
	{
		_level = lvl;
	}
	void outbuffer_increase(unsigned size)
	{
		size > _out_buffer_size ?
			_out_buffer_size = size : 
			_out_buffer_size = _out_buffer_size;
	}
	void filter_add(std::string &str)
	{
		_out.add(str);
	}
	void filter_sub(std::string &str)
	{
		_out.sub(str);
	}
	void filter_clear()
	{
		_out.clear();
	}
	void color(dlog::color_set c)
	{
		char const *strcolor [dlog::MAX] =
		{
				"\x1b[31m",
				"\x1b[32m",
				"\x1b[33m",
				"\x1b[34m",
				"\x1b[35m",
				"\x1b[36m",
				"\x1b[0m"

		};
		if(c == dlog::RESET &&
				_color_prefix != nullptr)
		{
			puts(strcolor[c]);
			_color_prefix = nullptr;
			return;
		}
		_color_prefix = strcolor[c];
	}

	void operator()(enum level lvl, 
		const char *format, ...)
	{
		int prefix_size = 0;
		int total_size = 0;
		if(_out_buffer_size <= 0)
		{
			return;
		}
		if(lvl < _level)
		{
			return;
		}
		va_list s;
		char out_buffer[_out_buffer_size] = {0, };
		if(!_prefix.empty())
		{
			prefix_size = _prefix.size();
			if((unsigned)prefix_size >= _out_buffer_size)
			{
				prefix_size = _out_buffer_size;
			}
			memcpy(out_buffer, _prefix.c_str(), prefix_size);
		}
		if((unsigned)prefix_size < _out_buffer_size)
		{
			va_start(s, format);	
			vsnprintf(out_buffer + prefix_size, 
				_out_buffer_size - prefix_size, 
				format, 
				s);			
			va_end(s);
		}
		out_buffer[_out_buffer_size - 1] = 0;

		_out <<(std::string(out_buffer));

	}			
	void operator()(enum level lvl, 
		void *ptr, unsigned size)
	{
		if(!ptr || size <= 0)
		{
			return;
		}
#define CONV_PRINT_ASCII(v)	(	((v >= 0x20) && (v <= 0x7e))	)
#define PVH(c) 				(	*(pBuffer + (nBf_Cs + c))		)
#define PVC(c)  			(CONV_PRINT_ASCII(*(pBuffer + (nBf_Cs + c))) ? PVH(c) : 0x2e)

		_dword nBf_Cs = 0;
		int nTot_Line = 0;
		int nTot_Rest = 0;
		unsigned char *pBuffer 			= (unsigned char *)ptr;
		char RestBuffer[77] 	= {0, };
		int i = 0;
		_dword nStartAddress 	= 0;

		if(!pBuffer || size <= 0)
		{
		return;
		}

		nTot_Line = (size	>>	4);
		nTot_Rest = (size 	&	0x0000000F);

		dlog::operator ()(lvl, "===========================================================================\n");
		dlog::operator ()(lvl, "offset h  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  Size [%uByte]\n", size);
		dlog::operator ()(lvl, "---------------------------------------------------------------------------\n");

		while(nTot_Line-- > 0)
		{
		dlog::operator ()(lvl, "%08x  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
		(pBuffer + nBf_Cs),   	PVH(0),		 PVH(1), 	PVH(2), 	PVH(3), 	PVH(4), 	PVH(5), 	PVH(6), 	PVH(7),
		PVH(8), 	 PVH(9), 	PVH(10), 	PVH(11), 	PVH(12), 	PVH(13), 	PVH(14), 	PVH(15),

		PVC(0),		 PVC(1),	PVC(2),		PVC(3),		PVC(4),		PVC(5),		PVC(6),		PVC(7),
		PVC(8),		 PVC(9),	PVC(10),	PVC(11),	PVC(12),	PVC(13),	PVC(14),	PVC(15));
		nBf_Cs += 16;
		}

		if(nTot_Rest > 0 && nTot_Rest <= 16)
		{
		memset((void *)(RestBuffer), 0x20, (sizeof(RestBuffer) / sizeof(RestBuffer[0]) - 2));
		nStartAddress = (_dword)(_dword *)pBuffer + nBf_Cs;


		conv_hex_to_string((unsigned char *)&nStartAddress, 
			sizeof(nStartAddress),
			RestBuffer, 
			((sizeof(nStartAddress) << 1) + 1));

		for(; i < nTot_Rest; i++)
		{
			conv_hex_to_string((pBuffer + nBf_Cs),
				sizeof(unsigned char), 
				(RestBuffer + (10 + ((i << 1) + i))),
				((sizeof(unsigned char) << 1) + 1));
			
			*(RestBuffer + (59 + i)) = PVC(0);
			nBf_Cs++;
		}
		RestBuffer[59+nTot_Rest] = 0x0A;
		RestBuffer[59+nTot_Rest + 1] = 0x00;
		dlog::operator ()(lvl, RestBuffer);

		}

		dlog::operator ()(lvl, "---------------------------------------------------------------------------\n");

	}

private:
	logout _out;
	int _server_sock;
	int _client_sock;
	std::shared_ptr<FILE> _file;
	bool _console_write;
	enum level _level;
	unsigned _out_buffer_size;
	std::shared_ptr<std::thread> _serverthread;
	std::string _prefix;
	struct sockaddr_in _serveraddr;
	int _pipe[2];
	char const *_color_prefix;
	std::function<void (std::string &)>_usrout_functor;

};
