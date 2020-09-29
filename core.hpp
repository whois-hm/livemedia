#pragma once


#if defined __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
extern "C"
{

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)

/*
	get ffmpeg
 */
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/error.h>
#include <libavutil/base64.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libpostproc/postprocess.h>
}



/*
 	 get live555
 */
#include "UsageEnvironment.hh"
#include "BasicUsageEnvironment.hh"
#include "Groupsock.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"


/*
 	 get workqueue
 */
#include "WQ.h"


/*
 	 get stl c++
 */
#include <type_traits>
#include <iostream>
#include <execinfo.h>
#include <stdexcept>
#include <vector>
#include <list>
#include <algorithm>
#include <thread>
#include <chrono>
#include <map>
#include <memory>
#include <tuple>
#include <mutex>
#include <atomic>
#include <locale>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <functional>
#include <sstream>
#include <string>
#include <condition_variable>
#include <future>

/*
 	 livemedia main
 */

#include "utils.hpp"
#include "filter.hpp"
#include "dlog.hpp"
#include "base64.hpp"
#include "livemedia++.hpp"
#include "wthread.hpp"
#include "busyscheduler.hpp"
#include "avattr.hpp"
#include "media_data.hpp"
#include "avpacket_class.hpp"
#include "pixel.hpp"
#include "pcm.hpp"
#include "avframe_class.hpp"
#include "pixelframe.hpp"
#include "pcmframe.hpp"
#include "swxcontext_class.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "mediacontainer.hpp"
#include "avpacket_bsf.hpp"
#include "mediacontainer_record.hpp"
#include "pixelutils.hpp"
#include "framescheduler.hpp"
#include "uvc.hpp"
#include "live5scheduler.hpp"
#include "live5rtspclient.hpp"
#include "live5livemediapp_sessions_parameters.hpp"
#include "live5livemediapp_sessions_source.hpp"
#include "live5livemediapp_sessions.hpp"
#include "live5rtspserver.hpp"
#include "playback_inst.hpp"
#include "local_playback.hpp"
#include "rtsp_playback.hpp"
#include "uvc_playback.hpp"
#include "playback.hpp"
#include "ui.hpp"


