#pragma once




typedef struct WQ workqueue;
template <typename _Class>
struct _wthread_functor
/*
 	 	 workqueue thread 'main  functor'
 */
{

	template <typename ...Args>
	void operator () (
			std::string name,
			workqueue *_wq,
			const std::function<_dword
				(workqueue *,
						void **,
						_dword *,
						_dword )> &recv,
			_dword time,
			Args...args)
	{
		DECLARE_LIVEMEDIA_NAMEDTHREAD(name.c_str());

		/*
		 	 	 we create object to stack memory
				default constructor has no '()'
		 */
		if (sizeof... (args) <= 0)
		{
			_Class t;
			s(_wq, recv, time, t);
			return;
		}

		_Class t(args...);

		s(_wq, recv, time, t);
	}
private:
	void s(workqueue *_wq,
			const std::function<_dword
				(workqueue *,
						void **,
						_dword *,
						_dword )>  &recv,
			_dword time,
			 _Class &o)
	{
		_dword r = WQ_FAIL;
		void * par = NULL;
		_dword size= 0;
		_dword qtime = time;
		do
		{
			/*
			 	 	 receving event from queue
			 */
			r = recv(_wq, &par, &size, qtime);
			if(r == WQ_FAIL)
			{
				DECLARE_THROW(true, "workqueue has return fail");
			}

			/*
			 	 	 notify to object
			 */
		}while(o(r, par, size, &qtime));
	}
};

template <typename _Class>
class wthread
{
private:
	std::thread *_th;
	workqueue *_wq;
	std::string _wt_name;

	/*
	 	 	 get workqueue operation functions
	 */
	const std::function<workqueue *
	(_dword, _dword)> open;
	const std::function<void
	(workqueue ** )> close;
	const std::function<_dword
	(workqueue *,
			void **,
			_dword *,
			_dword )> recv;
	const std::function<_dword
	(workqueue *,
			void *,
			_dword ,
			_dword,
			int  )> send;
public:
	wthread() = delete;
	wthread(wthread &rhs) = delete;
	wthread(const wthread &rhs) = delete;
	/*just can move it*/
	wthread( wthread &&rhs):
		_th(nullptr),
		_wq(NULL),
		_wt_name(nullptr),
		open(rhs.open),
		close(rhs.close),
		recv(rhs.recv),
		send(rhs.send)
	{
		operator = (static_cast<wthread &&>(rhs));
	}
	wthread(_dword len,/*length of queue*/
			_dword size,/*size of queue parameter*/
			char const *wt_name) :
		_th(nullptr),
		_wq(NULL),
		_wt_name(wt_name),
		open(WQ_open),
		close(WQ_close),
		recv(WQ_recv),
		send(WQ_send)
	{

		_wq = open(size , len );
		DECLARE_THROW(!_wq, "can't create workqueue");

	}
	virtual ~wthread()
	{
		end();
	}
	template <typename ... Args>
	void start(_dword timeout, Args ...args)
	/*
	 	 	 start thread
	 */
	{
		if(_th)
		{
			/*already running*/
			return;
		}
		_th = new std::thread(_wthread_functor<_Class>(),
				_wt_name,
				_wq,
				recv,
				timeout,
				args...);
	}
	_dword  sendto(void *p,
			_dword n,
			_dword timeout = INFINITE)
	/*
	 	 	 sending event 'nonblocking' mode
	 */
	{
		send(_wq, p, n, timeout, 0);
		return WQOK;
	}
	_dword sendto_wait( void *p,
			_dword n,
			_dword timeout = INFINITE)
	/*
	 	 	 sending event 'blocking' mode
	 	 	 return when 'WQ_recv' return the this event
	 */
	{
		send(_wq, p, n, timeout, WQ_SEND_FLAGS_BLOCK);
		return WQOK;
	}
	wthread & operator = ( wthread &&rhs)
	{
		this->_th = rhs._th;
//		this->open = rhs.open;
//		this->recv = rhs.recv;
//		this->send = rhs.send;
//		this->close = rhs.close;
		this->_wq = rhs._wq;
		this->_wt_name = rhs._wt_name;
		
		rhs._th = nullptr;
		rhs._wq = nullptr;
		rhs._wt_name = nullptr;
		return *this;
	}
	void end()
	{
		if(_th)
		{
			_th->join();
			delete _th;
			_th = nullptr;
		}
		if(_wq)
		{
			close(&_wq);
			_wq = nullptr;
		}
	}


	bool exec()
	{
		return _th || _wq ? true : false;
	}
};


