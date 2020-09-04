#pragma once
class pixelutils : public pixel
{
public:
	pixelutils() :
		pixel()
	{
	}
	virtual ~pixelutils()
	{

	}
	bool imagefile(char const *file)
	{
		if(!file)
		{
			return false;
		}
		mediacontainer con(file);
		if(!con.find_codec(AVMEDIA_TYPE_VIDEO))
		{
			return false;
		}
		decoder dec(con.find_stream(AVMEDIA_TYPE_VIDEO)->codecpar);

		while(!(*this))
		{
			avpacket_class pkt;
			if(!con.read_packet(AVMEDIA_TYPE_VIDEO, pkt))
			{
				break;
			}
			dec(pkt, [&](avpacket_class &packet,
					avframe_class & frm, void *ptr)->void{
				pixelframe pixfrm(*frm.raw());
				pixfrm >> *this;
			});
		}

		return (*this);
	}
	bool convert(int width, int height, enum AVPixelFormat fmt)
	{
		if(!(*this))
		{
			return false;
		}
		pixelframe pixfrm(*this);

		avattr attr;
		attr.set(avattr::frame_video, avattr::frame_video, 0, 0.0);
		attr.set(avattr::width, avattr::width, width, 0.0);
		attr.set(avattr::height, avattr::height, height, 0.0);
		attr.set(avattr::pixel_format, avattr::pixel_format, (int)fmt, 0.0);

		swxcontext_class (pixfrm,
				attr);
		pixfrm >> *this;
		return true;
	}
};
