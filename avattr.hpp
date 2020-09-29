#pragma once

namespace livemedia_attribute {
class avattr
{
	/*
	 	 map -> <key> : <value_int, value_double, value_string>
	 	 	 	 	 	 <key> : <value_int, value_double, value_string>
	 	 	 	 	 	 ................
	 	 	 	 	 	 .................

	 	 select three value int or double or string.
	 	 non use or initialize value is your choice
	 */
public:
	/*
	 	 defined string's

	 	 project "livemedia++" class's use this some regulated rules string
	 	 like about pixel or video class use key "frame_video", and
	 	 like about pcm or audio class use key "frame_audio".

	 	 //you need how to use initial value stored class 'avattr'
	 */

	static constexpr char const * const not_found = "not found";
	static constexpr char const * const frame_video = "frame video";
	static constexpr char const * const frame_audio = "frame audio";
	static constexpr char const * const video_x = "video x";
	static constexpr char const * const video_y = "video y";
	static constexpr char const * const width = "width";
	static constexpr char const * const height = "height";
	static constexpr char const * const pixel_format = "pixel format";
	static constexpr char const * const channel = "channel";
	static constexpr char const * const samplerate = "samplerate";
	static constexpr char const * const pcm_format= "pcm format";

	static constexpr char const * const video_encoderid= "video encoder id";
	static constexpr char const * const audio_encoderid= "audio encoder id";
	static constexpr char const * const fps= "fps";
	static constexpr char const * const bitrate= "bitrate";
	static constexpr char const * const gop= "gop";
	static constexpr char const * const max_bframe= "max bframe";
	typedef std::string avattr_type_string ;		/*key or value string*/
	typedef int                avattr_type_int;			/*value int*/
	typedef double      avattr_type_double;	/*value double*/
	typedef std::tuple <avattr_type_string,
			avattr_type_int,
			avattr_type_double>
	avattr_types;
private:
	 avattr_types get_types(char const *key) const
	{
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				return it.second;
			}
		}
		return avattr_types("",0 ,0.0);
	}
public:
	/*
	 	 insert map
	 */
	void set(avattr_type_string &&key,
			avattr_types  &&value)
	{
		_map.insert(
				std::make_pair(key,
						value)
		);
	}
	void set(char const *key, char const *str, int n, double d)
	{
		_map.insert(
				std::make_pair(avattr_type_string(key),
				avattr_types(avattr_type_string(str), n, d)));
	}

	void set(char const *key, avattr_types  &&value)
	{
		_map.insert(
				std::make_pair(avattr_type_string(key),
				value));
	}
	void set(char const *key, char const *str)
	{
		set(key, str, 0, 0.0);
	}
	void set(char const *key, int n)
	{
		set(key, "", n, 0.0);
	}
	void set(char const *key, double d)
	{
		set(key, "", 0, d);
	}

	void set(avattr_type_string &key,
			avattr_type_string &first,
			const avattr_type_int second,
			const avattr_type_double third)
	{
		set (key.c_str(), first.c_str(), second, third);
	}

	void set(avattr_type_string &&key,
			avattr_type_string &&first,
			const avattr_type_int second,
			const avattr_type_double third)
	{
		set(static_cast<avattr_type_string &&>(key),
				std::make_tuple(first,
						second,
						third));
	}
	void reset(char const *key, char const *str, int n, double d)
	{
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				it.second = avattr_types(avattr_type_string(str), n, d);
				break;
			}
		}
			
	}

	template <typename T>
	 T get(char const *key) const
	{
		T t;
		return t;
	}
	bool notfound(char const *key)  const
	{
		bool has = false;
		for(auto &it : _map)
		{
			if(it.first == key)
			{
				has = true;
				break;
			}
		}

		return !has;
	}
    void clear()
    {
        _map.clear();
    }
	avattr()
	{
		_map.clear();
	}
	virtual ~avattr(){}
protected:
	std::map<
	std::string,
	avattr_types
	> _map;
};
template<>
inline  avattr::avattr_type_string avattr::get( char const *key)const 	{ return std::get<0>(get_types(key)); }
template<>
inline avattr::avattr_type_int avattr::get( char const *key) 	const	{ return std::get<1>(get_types(key)); }
template<>
inline avattr::avattr_type_double avattr::get( char const *key) const	{ return std::get<2>(get_types(key)); }
template<>
inline avattr::avattr_types avattr::get( char const *key) const			{ return get_types(key); }
}
using namespace livemedia_attribute;

