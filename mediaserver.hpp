#pragma once


class mediaserver :
		public live5scheduler
		<live5rtspserver>
{
private:
    int startif(bool another,
              const live5rtspserver::report &report)
    {
        if(!can())
        {
            return -1;
        }


        live5scheduler<live5rtspserver>::start(another,
                        report,
                        _attr.get_char("url"),
                        _attr.get_char("session name"),
                        _attr.get_char("proxy id"),
                        _attr.get_char("proxy pwd"),
                        _attr.get_int("port"),
                        _attr.get_char("server id"),
                        _attr.get_char("server pwd"),
                        0,
                        _attr.get_int("reuse") > 0 ? true : false);
        return 0;
    }

public:
	mediaserver() :
		live5scheduler <live5rtspserver>(){ }
    mediaserver(const avattr &attr) :
        live5scheduler <live5rtspserver>(),
    _attr(attr){ }
	virtual ~mediaserver() { }
    int start(bool another, const live5rtspserver::report &report)
    {
        return startif(another, report);
    }

    int start(bool another,
              avattr &attr,
              const live5rtspserver::report &report)
    {
        _attr = attr;
        return startif(another, report);
    }

    int start(bool another,
            const live5rtspserver::report &report,
			char const *url,
			char const*sessionname,
			char const*proxy_id,
			char const*proxy_pwd,
			int port,
              bool reuse,
			char const*server_id = nullptr,
			char const *server_pwd = nullptr)
	{
        _attr.clear();
		if(url)_attr.set("url", url, 0, 0.0);
		if(sessionname)_attr.set("session name", sessionname, 0, 0.0);
		if(proxy_id)_attr.set("proxy id", proxy_id, 0, 0.0);
		if(proxy_pwd)_attr.set("proxy pwd", proxy_pwd, 0, 0.0);
		if(server_id)_attr.set("server id", server_id, 0, 0.0);
		if(server_pwd)_attr.set("server pwd", server_pwd, port, 0.0);
		_attr.set("port", "port", port, 0.0);
        _attr.set("reuse", "reuse", reuse ? 1 : 0, 0.0);
		
		if(!can())
		{
			return -1;
		}

       return  startif(another, report);
	}
private:
	bool can()
	{
        if(_attr.notfound("url"))
        {
            return false;
        }
        if(_attr.notfound("port"))
        {
            return false;
        }
        if(_attr.notfound("session name"))
        {
            return false;
        }
        return true;
	}
	avattr _attr;
};
