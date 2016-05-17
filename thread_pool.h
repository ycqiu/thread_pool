#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <queue>
#include <vector>

class CThreadPool
{
public:
	enum
	{
		INVALID = -1,
		RUNNING = 0,
		SHUTDOWN_IMMEDIATE = 1,
		SHUTDOWN_GRACEFUL = 2
	};

private:
	struct CTask	
	{
		void (*callBack)(void*);
		void* param;	
	};

	pthread_mutex_t lock_;
	pthread_cond_t notify_;
	std::queue<CTask> tasks_;
	std::vector<pthread_t> threads_;
	int shutdown_;
		
private:
	static void* threadFunc(void* arg);

public:
	CThreadPool(int threadNum = -1);  
	~CThreadPool();  

	int init();
	int add(void (*f)(void*), void* p);
	int destory(int flag);
};

#endif

