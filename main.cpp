#include "core.hpp"
#include "SDL.h"


static avattr *attribute = nullptr;
static Uint32 sdl_requestevent = 0;
static SDL_Window *window = nullptr;
static SDL_Renderer *render = nullptr;
static SDL_Texture *texture = nullptr;
static soundcardplayback *soundcard = nullptr;
static playback *player = nullptr;
static std::thread *_ath = nullptr;
static std::thread *_vth = nullptr;
static bool runflag = true;
static bool pauseflag = true;

static Sint32 sdl_end_of_videoframe = 0;
static Sint32 sdl_new_videoframe = 1;
static Sint32 sdl_end_of_audioframe = 2;
struct _vth_functor
{
	void push_to_end()
	{
		SDL_Event e;
		memset(&e, 0, sizeof(SDL_Event));
		e.type =sdl_requestevent;
		e.user.code = sdl_end_of_videoframe;
		e.user.data1 =NULL;
		SDL_PushEvent(&e);
	}
	void push_to_new_frame(pixel &pix)
	{
		SDL_Event e;
		memset(&e, 0, sizeof(SDL_Event));
		e.type =sdl_requestevent;
		e.user.code = sdl_new_videoframe;
		e.user.data1 = (void *)new pixel(std::move(pix));
		SDL_PushEvent(&e);
	}
	void operator()()
	{
		while(runflag)
		{
			pixel pix;
			printf("=======> vide2o\n");
			int res = player->take(pix, std::string(avattr::frame_video));
			printf("=======> video = %d\n", res);
			if(res < 0)
			{
				/*end of video frame*/
				push_to_end();
				break;
			}
			if(res == 0)
			{
				/*ready to frame*/
			}
			/*to*/
			push_to_new_frame(pix);
		}
	}
};
struct _ath_functor
{
	void push_to_end()
	{
		SDL_Event e;
		memset(&e, 0, sizeof(SDL_Event));
		e.type =sdl_requestevent;
		e.user.code = sdl_end_of_audioframe;
		e.user.data1 =NULL;
		SDL_PushEvent(&e);
	}
	void write(pcm &p)
	{
		soundcard->write(p);
	}
	void operator()()
	{
		while(runflag)
		{
			pcm_require re = std::make_pair(pcm(), soundcard->write_size());
			printf("=======> audio\n");
			int res = player->take(re, std::string(avattr::frame_audio));
			printf("=======> audio = %d\n", res);
			if(res < 0)
			{
				/*end of audio frame*/
				push_to_end();
				break;
			}
			if(res == 0)
			{
				/*ready to frame*/
			}
			/*to*/
			write(re.first);
		}
	}
};

void help_opt()
{
	printf("----------------------------------------------------------------------------\n");
	printf("                          with player option                                \n");
	printf("----------------------------------------------------------------------------\n");

	printf("*contents=x			'x' the 'player's target contents, this will 'file' or 'rtsp url' or 'cam device'\n");
	printf("*%s=x 			will be running with video. value 'x' it doesn't matther anyting.\n"
								"				just see a key, you want this vaild value, you should be precede %s, %s, %s\n", avattr::frame_video,
			avattr::width,
			avattr::height,
			avattr::pixel_format);
	printf("*%s=x 			value x is video frame width. \n", avattr::width);
	printf("*%s=x 			value x is video frame height. \n", avattr::height);
	printf("*%s=x 		value x is video frame format. this can be current 'yv12' only \n", avattr::pixel_format);

	printf("*%s=x 			will be running with audio. value 'x' it doesn't matther anyting.\n"
			"				just see a key, you want this vaild value, you should be precede %s, %s, %s\n", avattr::frame_audio,
			avattr::channel,
			avattr::samplerate,
			avattr::pcm_format);
	printf("*%s=x 			value x is audio frame channel. \n", avattr::channel);
	printf("*%s=x 			value x is audio frame samplingrate. \n", avattr::samplerate);
	printf("*%s=x 			value x is audio frame format. this can be current 'unsigned 8bit', 'signed 16bit'\n", avattr::pcm_format);

	printf("*connection time=x 		value x is 'player' time for connection to server if conntents mean that rtsp.\n");
	printf("*auth id=x 			value x is server auth id if conntents mean that rtsp.\n");
	printf("*auth pwd=x 			value x is server auth password if conntents mean that rtsp.\n");
	printf("----------------------------------------------------------------------------\n");
}
/*
 	 open the read only file
 	 param file : target full path
 	 return opend file handle
 */
static FILE *file_open_readonly(const std::string &file)
{
	FILE *t = nullptr;
	t = fopen(file.c_str(), "rb");
	DECLARE_THROW(!t, "can't open the option file");
	return t;
}
/*
 	 close the file
 	 param f : target file open handle
 */
static void file_close(FILE *&f)
{
	if(f)
	{
		fclose(f);
		f = nullptr;
	}
}
/*
 	 read one line the 'key=value' from file
 	 param f : target file
 	 param iseof : file has been reach 'eof'
 	 return pair string key, value
 */
static std::pair<std::string, std::string>
	file_read_nextline(FILE *f, bool &iseof)
{
	iseof = false;
	char linebuf[256] = {0, };
	char *plinebuf = linebuf;
	char *s = nullptr;
	char *v = nullptr;
	if(fgets(linebuf, 256, f) == nullptr)
	{
		if(feof(f))
		{
			iseof = true;
		}
		return std::make_pair(std::string(),
				std::string());
	}

	s = plinebuf;
	if(!*s ||
			*s == '#')/*comment*/
	{
		return std::make_pair(std::string(),
						std::string());
	}
	while((*plinebuf) != 0)
	{
		if((*plinebuf) == '=')
		{
			if(plinebuf == s || /*key not found*/
					*(plinebuf + 1) == '\n' ||/*value not found*/
					*(plinebuf + 1) == 0/*value not found*/
					)
			{
				return std::make_pair(std::string(),
								std::string());
			}
			(*plinebuf) = 0;
			v = plinebuf + 1;
		}
		if((*plinebuf) == '\n')
		{
			(*plinebuf) = 0;
		}
		plinebuf++;
	}

	if(s && v)
	{
//		printf("key = %s value = %s\n", s, v);
		return std::make_pair(s, v);
	}

	return std::make_pair(std::string(),
					std::string());
}

/*
 	 reading the option
 	 param argc : from main the number of argument
 	 param argv : from main the arguments
 	 return for avplayer's options
 */
static avattr readopt(int argc, char **argv)
{
	avattr attr;
	FILE *f = nullptr;
	bool iseof = false;
	do
	{
		if(argc < 2)
		{
			help_opt();
			break;
		}
		f = file_open_readonly(std::string(std::string(argv[1])));

		while(!iseof)
		{
			std::pair<std::string, std::string> v =
					file_read_nextline(f, iseof);
			if(!v.first.empty() &&
					!v.second.empty())
			{
				int numeric = 0;
				std::stringstream sint(v.second.c_str());
				if(!sint.fail())
				{
					sint >> numeric;
				}
				attr.set(v.first.c_str(),
						v.second.c_str(), numeric, 0.0);
			}
		}

	}while(0);
	file_close(f);
	return attr;
}
static void draw_display(pixel *p = nullptr)
{
	if(!p)
	{
		int w, h;
		SDL_SetRenderDrawColor(render, 0,0,0,255);
		SDL_GetWindowSize(window, &w, &h);
		SDL_Rect r = {0, 0, w, h};
		SDL_RenderFillRect(render, &r);
		SDL_RenderPresent(render);
		return;
	}
	SDL_Rect sr;

	int width = p->width();
	int height = p->height();
	int ySize = (width * height);
	int uvSize = ySize / 4;
	int yStartPos = 0;
	int uStartPos = ySize;
	int vStartPos = yStartPos + uStartPos + uvSize;
	sr.x = sr.y  = 0;
	sr.w = width;
	sr.h = height;

	unsigned char *pixel = (unsigned char *)p->take<raw_media_data::type_ptr>();
	unsigned char *yplane = pixel + yStartPos;
	unsigned char *uplane = pixel + uStartPos;
	unsigned char *vplane = pixel + vStartPos;
	SDL_UpdateYUVTexture(texture, NULL, yplane, width , uplane, width/2, vplane, width /2);
	SDL_RenderCopy(render, texture, &sr, &sr);
	SDL_RenderPresent(render);
	delete p;
}
/*
 	 open the display
 */
static void open_display()
{
	SDL_Init(SDL_INIT_VIDEO);
	sdl_requestevent = SDL_RegisterEvents(1);
	window = SDL_CreateWindow("avplayer",
			0,
			0,
			attribute->get<avattr::avattr_type_int>(avattr::width),
			attribute->get<avattr::avattr_type_int>(avattr::height),
			SDL_WINDOW_SHOWN);

	render = SDL_CreateRenderer(window, -1 , 0);

	texture = SDL_CreateTexture(render,
			(Uint32)attribute->get<avattr::avattr_type_int>("pixelformat_sdl"),
			SDL_TEXTUREACCESS_STREAMING,
			attribute->get<avattr::avattr_type_int>(avattr::width),
			attribute->get<avattr::avattr_type_int>(avattr::height));

	printf("zz =>   %d %d\n", attribute->get<avattr::avattr_type_int>(avattr::width), attribute->get<avattr::avattr_type_int>(avattr::height));

	DECLARE_THROW(!window ||
			!render ||
			!texture, "can't open the display");
	draw_display();
}
/*
 	 close the opend display
 */
static void close_display()
{
	if(texture) SDL_DestroyTexture(texture);
	if(render) SDL_DestroyRenderer(render);
	if(window) SDL_DestroyWindow(window);

	window = nullptr;
	render = nullptr;
	texture = nullptr;
	SDL_Quit();
}
/*
 	 open system soundcard.
 	 return 0 if soundcard opend other nagative value
 */
static void open_soundcard()
{
	audiospec s(attribute->get<avattr::avattr_type_int>(avattr::channel),
			attribute->get<avattr::avattr_type_int>(avattr::samplerate),
			512,
			(enum AVSampleFormat)attribute->get<avattr::avattr_type_int>(avattr::pcm_format));

	soundcard = new soundcardplayback();
	DECLARE_THROW(soundcard->open_default(s) != 0, "can't open soundcard")
}
/*
 	 close the opend system soundcard.
 */
static void close_soundcard()
{
	if(soundcard)
	{
		delete soundcard;
	}
	soundcard = nullptr;
}
/*
 	 open the contents.
 	 this function create our main 'player'
 	 return  nagative value if player has not found 'video' && 'audio' frame
 	         other return > 0
 */
static void open_contents()
{
	unsigned contime = 500;
	const char *auth_id = nullptr;
	const char *auth_pwd = nullptr;
	if(!attribute->notfound("connection time")) 	contime = attribute->get<avattr::avattr_type_int>("connection time");
	if(!attribute->notfound("auth id")) 				auth_id = attribute->get<avattr::avattr_type_string>("auth id").c_str();
	if(!attribute->notfound("auth pwd")) 			auth_pwd = attribute->get<avattr::avattr_type_string>("auth pwd").c_str();
	player = new playback((const avattr &)*attribute,
			attribute->get<avattr::avattr_type_string>("contents").c_str(),
			contime,
			auth_id,
			auth_pwd);



	if(player->has(std::string(avattr::frame_video)))
	{
		open_display();
		_vth = new std::thread(_vth_functor());
	}
	if(player->has(std::string(avattr::frame_audio)))
	{
		open_soundcard();
		_ath = new std::thread(_ath_functor());
	}
	DECLARE_THROW(!_ath && !_vth, "can't found any stream from player");
	printf("zzzzzzzzzzz11111\n");
	player->pause();
	printf("zzzzzzzzzzz11111222222222\n");
}
/*
 	 close the opend 'player'
 */
static void close_contents()
{
	runflag = false;
	if(player)
	{
		player->resume(true);
	}
	if(_vth)
	{
		_vth->join();
		delete _vth;
		_vth = nullptr;
	}
	if(_ath)
	{
		_ath->join();
		delete _ath;
		_ath = nullptr;
	}
	if(player)
	{
		delete player;
	}
	player = nullptr;
	close_soundcard();
	close_display();
}

static bool run_contents_sdl_event_from_our(SDL_Event *e)
{
	if(e->type == sdl_requestevent)
	{
		if(e->user.code == sdl_end_of_videoframe)
		{
			draw_display();
			return true;
		}
		if(e->user.code == sdl_new_videoframe)
		{
			draw_display((pixel *)e->user.data1);
			return true;
		}
		if(e->user.code == sdl_end_of_audioframe)
		{
			return true;
		}
		return false;
	}
	return true;
}
static int run_contents_sdl_event_from_sys(SDL_Event *e)
{
	if(e->type == SDL_KEYDOWN && e->key.repeat == 0)
	{
		if(e->key.keysym.sym == SDLK_SPACE)
		{
			if(pauseflag)
			{
				 player->resume();
				 pauseflag = false;
			}
			else
			{
				player->pause();
				pauseflag = true;
			}
		}
		if(e->key.keysym.sym == SDLK_LEFT)
		{
			player->seek(-10.0);
		}
		if(e->key.keysym.sym == SDLK_RIGHT)
		{
			player->seek(10.0);
		}
		return 1;
	}
	if(e->type == SDL_WINDOWEVENT &&
			e->window.event == SDL_WINDOWEVENT_CLOSE)
	{
		return -1;
	}
	return 0;
}
/*
 	 run the 'player'
 */
static void run_contents()
{
	SDL_Event e;
	while(SDL_WaitEvent(&e))
	{
		if(!run_contents_sdl_event_from_our(&e))
		{
			break;
		}
		if(run_contents_sdl_event_from_sys(&e) < 0)
		{
			break;
		}
	}
}
/*
 	 checking 'attribute' option.
 	 param attr : check from this 'attribute'
 	 return 'true' if option can running the 'player'
 	          other return 'false'
 */
static void checkopt( avattr &attr)
{

	DECLARE_THROW(attr.notfound("contents"), "can't find option 'contents'");
	DECLARE_THROW(attr.notfound(avattr::frame_video) &&
			attr.notfound(avattr::frame_audio), "can't find option any stream");

	/*invalid video option*/
	if(!attr.notfound(avattr::frame_video))
	{
		DECLARE_THROW(attr.notfound(avattr::width) ||
				attr.notfound(avattr::height) ||
				attr.notfound(avattr::pixel_format), "can't find option video's format value");
	}
	/*invalid audio option*/
	if(!attr.notfound(avattr::frame_audio))
	{
		DECLARE_THROW(attr.notfound(avattr::channel) ||
				attr.notfound(avattr::samplerate) ||
				attr.notfound(avattr::pcm_format), "can't find option audio's format value");
	}
	if(!attr.notfound(avattr::frame_video))
	{
		DECLARE_THROW(attr.get<avattr::avattr_type_string>(avattr::pixel_format) != "yv12",
				"can't support video format");
		/*this yv12 set*/
		attr.set("pixelformat_sdl", (int)SDL_PIXELFORMAT_YV12);
		attr.set("pixelforamt_ffmpeg", (int)AV_PIX_FMT_YUV420P);
	}

	if(!attr.notfound(avattr::frame_audio))
	{
		std::string pcmfmt = attr.get<avattr::avattr_type_string>(avattr::pcm_format);
		DECLARE_THROW(pcmfmt != "unsigned 8bit" ||
				pcmfmt != "signed 16bit",
						"can't support audio format");
		if(pcmfmt == "unsigned 8bit")
		{
			attr.reset(avattr::pcm_format, avattr::pcm_format, (int)AV_SAMPLE_FMT_U8, 0.0);
		}
		if(pcmfmt == "signed 16bit")
		{
			attr.reset(avattr::pcm_format, avattr::pcm_format, (int)AV_SAMPLE_FMT_S16, 0.0);
		}
	}
}
int main(int argc, char **argv)
{
	livemedia_pp::ref();
	livemedia_pp::ref()->log().log_enable();
	livemedia_pp::ref()->log().console_writer_install();

	avattr attr = readopt(argc, argv);
	checkopt(attr);

	attribute = new avattr(std::move(attr));
	open_contents();
	run_contents();

	close_contents();

	delete attribute;
	livemedia_pp::ref(false);
	return 0;
}
