#pragma once

class base64_encoder
{
private:
	char *_base64_encode_data;
	unsigned _base64_encode_size;
public:
	base64_encoder() = delete;
	base64_encoder(const uint8_t *in, unsigned in_size) :
		_base64_encode_data(nullptr),
		_base64_encode_size(0)
	{
		if(in && in_size > 0)
		{
			_base64_encode_size = AV_BASE64_SIZE(in_size);
			_base64_encode_data = new char[_base64_encode_size];

			_base64_encode_data = 	av_base64_encode(_base64_encode_data,
					_base64_encode_size,
					in,
					in_size);
		}
	}
	base64_encoder(const base64_encoder &rhs) :
		_base64_encode_data(nullptr),
		_base64_encode_size(0)
	{
		if(rhs._base64_encode_size > 0 &&
				rhs._base64_encode_data)
		{
			_base64_encode_data = new char[rhs._base64_encode_size];
			memcpy(_base64_encode_data,
					rhs._base64_encode_data,
					rhs._base64_encode_size);
			_base64_encode_size = rhs._base64_encode_size;
		}
	}
	base64_encoder(base64_encoder &&rhs) :
		_base64_encode_data(rhs._base64_encode_data),
		_base64_encode_size(rhs._base64_encode_size)
	{
		rhs._base64_encode_data = nullptr;
		rhs._base64_encode_size = 0;
	}

	virtual ~base64_encoder()
	{
		if(_base64_encode_data)
		{
			delete [] _base64_encode_data;
			_base64_encode_data = nullptr;
		}

	}
	const char *get() const
	{
		return _base64_encode_data;
	}
	unsigned encoded_size()
	{
		return _base64_encode_size;
	}

};
