#pragma once

typedef std::tuple
		<
		int,/*hour*/
		int,/*min*/
		int,/*sec*/
		int,/*ms*/
		int64_t/*duration given ffmpeg*/
		>
duration_div;

/*
 	 	 use exam this,
 	 	 case video : width, height, format
 	 	 case audio : channel, samplerate, format
 */
typedef std::tuple
		<
		int,
		int,
		int
		>
triple_int;

/*
 	 use exam this,
 	 first triple : origin video or audio
 	 second triple : new video or audio
 */
typedef std::pair
		<
		triple_int,
		triple_int
		>
compare_triple_int;

class pcm;
typedef std::pair<pcm,/*pcm data*/
		int/*requre pcm size*/
		> pcm_require;

typedef std::function<void * (size_t)> base_allocator;
#define __base__malloc__ malloc// libwq_malloc

typedef std::function<void (void *)> base_deallocator;
#define __base__free__ free//libwq_free

/*
inline void* operator new(std::size_t size)
{
	void *userblock = __base__malloc__(size);
	return  userblock;
}
inline void* operator new[](std::size_t size)
{
	void *userblock = __base__malloc__(size);
	return  userblock;
}
inline void operator delete(void *ptr)
{
	if(ptr) __base__free__(ptr);
}
inline void operator delete[](void *ptr)
{ if(ptr) __base__free__(ptr); }
*/


typedef std::lock_guard<std::mutex> autolock;


inline
bool contain_string(char const *str,
		char const *findstr)
/*
 	 find character's from str
 */
{
	if(!str ||
			!findstr)
	{
		return false;
	}
	std::string dump(str);

	return dump.find(findstr) != std::string::npos;
}

inline
unsigned sys_time_c()
/*
 	 get system time
 */
{
	struct timeval v;
	gettimeofday(&v, NULL);

	return (v.tv_sec * 1000) + (v.tv_usec / 1000);
}
inline
int conv_hex_to_string(unsigned char *pSource,
	int nSource_perByte, 
	char *pDst, 
	int nDst)
{
	unsigned char  nHighNibleTo_StringByte = 0x00;
	unsigned char  nLowNibleTo_StringByte  = 0x00;
	unsigned short tempBridge    = 0;
	unsigned char *pCastDst = (unsigned char *)pDst;

	if((!pSource) 				||
	   (nSource_perByte <= 0) 	||
	   (!pDst) 					||
	   (nDst < (nSource_perByte * 2) + 1)
	   ) return WQEINVAL;

	while(nSource_perByte-- > 0)
	{
		nHighNibleTo_StringByte = (((*(pSource + nSource_perByte)) & 0xF0) >> 4);
		nLowNibleTo_StringByte  = ((*(pSource + nSource_perByte))  & 0x0F);


		tempBridge = (nHighNibleTo_StringByte >= 0x00 && nHighNibleTo_StringByte <= 0x09) ? (nHighNibleTo_StringByte | 0x30)       : (nHighNibleTo_StringByte + 0x57);
		tempBridge |= (nLowNibleTo_StringByte >= 0x00 && nLowNibleTo_StringByte <= 0x09)  ? ((nLowNibleTo_StringByte | 0x30) << 8) : ((nLowNibleTo_StringByte + 0x57) << 8);


		*(pCastDst++) = (unsigned char )(tempBridge & 0x00FF);
		*(pCastDst++) = (unsigned char )((tempBridge >> 8) & 0x00FF);
	}
	return WQOK;
}
inline bool socket_make_nonblock(int &sock)
{
#if defined (_platform_linux)

	if(sock < 0)
	{
		return false;
	}
	int opt = 0;
	opt = fcntl(sock, F_GETFL);
	if(opt < 0)
	{
		return false;
	}
	opt = (O_NONBLOCK | O_RDWR);
	return fcntl(sock, F_SETFL, opt) >= 0;
#endif
	return false;
}
#if defined (_platform_linux)
class fd_signal_detector
{
#define FD_IN	(1 << 0)
#define FD_OUT	(1 << 1)
#define FD_ERR	(1 << 2)

private:
	struct timeval tv;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	int maxfd;

public:
 	fd_signal_detector(){clear();}
	virtual ~fd_signal_detector(){clear();}
	void clear()
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);
		maxfd = -1;
	}
	void set(int fd, _dword flag = (FD_IN | FD_ERR))
	{	
		if(fd == -1)
		{
			return;
		}
		if(IS_BIT_SET(flag, FD_IN))
		{
			FD_SET(fd, &readfds);
		}
		if(IS_BIT_SET(flag, FD_OUT))
		{
			FD_SET(fd, &writefds);
		}
		if(IS_BIT_SET(flag, FD_ERR))
		{
			FD_SET(fd, &exceptfds);
		}
		if(maxfd <= fd)
		{
			maxfd = fd;
		}
	}
	int sigwait(int timeout)
	{
		if(maxfd == -1)
		{
			return -1;
		}
		struct timeval *tm = NULL;
		if(timeout >= 0)
		{
			tv.tv_sec = timeout / 1000;
			tv.tv_usec = (timeout % 1000) * 1000;
			tm = &tv;
		}

		return select(maxfd + 1, &readfds, &writefds, &exceptfds, tm);
	}
	_dword issetbit(int fd, _dword flag)
	{

		_dword returnbit = 0;
		if(fd == -1)
		{
			return returnbit;
		}
		if(IS_BIT_SET(flag, FD_IN))
		{
			if(FD_ISSET(fd, &readfds))
			{
				BIT_SET(returnbit, FD_IN);
			}
		}
		if(IS_BIT_SET(flag, FD_OUT))
		{
			if(FD_ISSET(fd, &writefds))
			{
				BIT_SET(returnbit, FD_OUT);
			}
		}
		if(IS_BIT_SET(flag, FD_ERR))
		{
			if(FD_ISSET(fd, &exceptfds))
			{
				BIT_SET(returnbit, FD_ERR);
			}
		}
		return returnbit;
	}
	bool issetbit_err(int fd)
	{
		if(fd == -1)
		{
			return false;
		}
		return issetbit(fd, FD_ERR) & FD_ERR;
	}
	bool issetbit_in(int fd)
	{
		if(fd == -1)
		{
			return false;
		}
		return issetbit(fd, FD_IN) & FD_IN;
	}
	bool issetbit_out(int fd)
	{
		if(fd == -1)
		{
			return false;
		}
		return issetbit(fd, FD_OUT) & FD_OUT;
	}
};
#endif



struct av_type_string_map
/*just mapping ffmpeg type to string*/
{
	char const *operator()(enum AVMediaType media_type)
	{
		return av_get_media_type_string(media_type);
	}
	char const *operator()(enum AVCodecID codecid)
	{
		return avcodec_get_name(codecid);
	}
};
struct media_type_string_support_map
{
	char const *operator()(enum AVMediaType media_type)
	{
		if(media_type == AVMEDIA_TYPE_VIDEO)
		{
			return "video";
		}
		if(media_type == AVMEDIA_TYPE_AUDIO)
		{
			return "audio";
		}
		return nullptr;
	}
	enum AVMediaType operator()(char const *media_type)
	{
		if(media_type)
		{
			if(!strcmp(media_type, media_type_string_support_map::operator ()(AVMEDIA_TYPE_VIDEO)))
			{
				return AVMEDIA_TYPE_VIDEO;
			}
			if(!strcmp(media_type, media_type_string_support_map::operator ()(AVMEDIA_TYPE_AUDIO)))
			{
				return AVMEDIA_TYPE_AUDIO;
			}
		}
		return AVMEDIA_TYPE_UNKNOWN;
	}
};
struct codec_type_string_support_map
{
	char const *operator()(enum AVCodecID codecid)
	{
		if(codecid == AV_CODEC_ID_H264)
		{
			return "H264";
		}
	}
	enum AVCodecID operator()(char const *codecid)
	{
		if(codecid)
		{
			if(!strcmp(codecid, codec_type_string_support_map::operator ()(AV_CODEC_ID_H264)))
			{
				return AV_CODEC_ID_H264;
			}
		}
		return AV_CODEC_ID_NONE;
	}
};
