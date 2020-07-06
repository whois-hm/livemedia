#pragma once
struct playback_inst
{
	avattr _attr;
	playback_inst(const avattr &attr) : _attr(attr){}
        virtual ~playback_inst (){}
        virtual bool connectionwait(){return true;}
    virtual void record(char const *file)=0;
	virtual void pause()  = 0;
	virtual void resume(bool closing = false)  = 0;
	virtual void seek(double incr)  = 0;
	virtual bool has(avattr::avattr_type_string &&key)  = 0;
	virtual void play()   = 0;
	virtual int take(const std::string &title, pixel &output) = 0;
	virtual int take(const std::string &title, pcm_require &output) = 0;
        virtual duration_div duration() = 0;
    virtual void resolution(int w, int h)
    {
        /*if want change resolution */
        if(w > 0 &&
                h > 0)
        {
            _attr.reset(avattr_key::width, avattr_key::width, w, (double)w);
            _attr.reset(avattr_key::height, avattr_key::height, h, (double)w);
        }
    }

	virtual enum AVMediaType get_master_clock(){return AVMEDIA_TYPE_UNKNOWN;}
};
