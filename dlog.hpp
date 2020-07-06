#pragma once
class dlog
{
public:
	enum level
	{
		normal,
		warnning,
		critical
	};
	dlog() : _server_sock(-1),
		_client_sock(-1),
		_file(nullptr),
		_console_write(false),
		_level(normal),
		_out_buffer_size(0),
		_serverthread(nullptr)
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

	

	void udp_receiver_server_uninstall()
	{
		if(_pipe[1] != -1)
		{
			char d = 1;
			write(_pipe[1], &d, 1);
		}
		if(_serverthread)
		{
			_serverthread->join();
			delete _serverthread;
			_serverthread = nullptr;
		}
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
		
		_server_sock = tempsock;
		_serverthread = new std::thread([&]()->void{
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

				
			});
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
	bool udp_sender_client_install(char const *serverip, int serverport)
	{
		int tempsock = -1;
		
		if(!serverip || serverport < 0)
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
		_serveraddr.sin_addr.s_addr = inet_addr(serverip);
		_serveraddr.sin_port = htons(serverport);
		
		udp_sender_clinet_uninstall();

		_client_sock = tempsock;

	}
	void file_writer_uninstall()
	{
		if(_file)
		{
			fclose(_file);
			_file = nullptr;
		}
	}
	bool file_writer_install(char const *file, bool boverwrite)
	{
		FILE *tempfile = nullptr;
		if(!file)
		{
			return false;
		}
		
		tempfile = fopen(file, boverwrite ? "w" : "a");
		if(!tempfile)
		{
			return false;
		}
		file_writer_uninstall();

		_file = tempfile;
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
	void filter_add(char const *str)
	{
		if(str)
		{
			_filters.push_back(std::string(str));
		}
	}
	void filter_sub(char const *str)
	{
		if(!str)
		{
			return;
		}
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
	void filter_clear()
	{
		_filters.clear();
	}

	void operator()(enum level lvl, 
		const char *format, ...) const
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
			if(prefix_size >= _out_buffer_size)
			{
				prefix_size = _out_buffer_size;
			}
			memcpy(out_buffer, _prefix.c_str(), prefix_size);
		}
		if(prefix_size < _out_buffer_size)
		{
			va_start(s, format);	
			vsnprintf(out_buffer + prefix_size, 
				_out_buffer_size - prefix_size, 
				format, 
				s);			
			va_end(s);
		}
		out_buffer[_out_buffer_size - 1] = 0;
		total_size = strlen(out_buffer);

		if(!in_the_filter(out_buffer))
		{
			return;
		}
		if(_console_write)
		{
			printf("%s", out_buffer);
		}
		if(_file)
		{
			fwrite(out_buffer, sizeof(char), total_size, _file);
		}
		
		if(_client_sock != -1)
		{
			sendto(_client_sock, 
				out_buffer,
				total_size,
				MSG_DONTWAIT | MSG_NOSIGNAL, 
				(struct sockaddr *)&_serveraddr, 
				sizeof(_serveraddr));
		}		
	}			
	void operator()(enum level lvl, 
		void *ptr, unsigned size) const
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
		printf("%08x  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
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
	bool in_the_filter(char *str) const
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
	int _server_sock;
	int _client_sock;
	FILE *_file;
	bool _console_write;
	enum level _level;
	unsigned _out_buffer_size;
	std::thread *_serverthread;
	std::string _prefix;
	struct sockaddr_in _serveraddr;
	int _pipe[2];
	std::list<std::string > _filters;
	
};
