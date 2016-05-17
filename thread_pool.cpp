#include "thread_pool.h"
#include "log.h"
#include <pthread.h>

#define CHECK_ERROR_RETURN(res, ret) \
	do\
	{\
		if(res)\
		{\
			return res = ret;\
		}\
	} while(0)


void* CThreadPool::threadFunc(void* arg)
{
	int ret = 0;
	CThreadPool* pool = static_cast<CThreadPool*>(arg);	
	unsigned long id = (unsigned long)pthread_self();
	//循环从任务队列中取出任务并执行，如果队列为空则等待
	for(; ;)
	{
		pthread_mutex_lock(&(pool->lock_));	

		while(pool->shutdown_ == RUNNING && pool->tasks_.empty() == true)	
		{
			DEBUG_LOG("%lu wait", id);
			pthread_cond_wait(&(pool->notify_), &(pool->lock_));
			DEBUG_LOG("%lu wait ok", id);
		}

		if(pool->shutdown_ == SHUTDOWN_IMMEDIATE || 
				(pool->tasks_.empty() == true && pool->shutdown_ == SHUTDOWN_GRACEFUL))	
		{
			DEBUG_LOG("%lu exit", id);
			pthread_mutex_unlock(&(pool->lock_));
			break;
		}

		CTask task = pool->tasks_.front();   pool->tasks_.pop();
		pthread_mutex_unlock(&(pool->lock_));

		(*(task.callBack))(task.param);
	}

	return reinterpret_cast<void*>(ret);
}

CThreadPool::CThreadPool(int threadNum): shutdown_(INVALID) 
{
	//默认情况下线程数量为cpu数量两倍
	if(threadNum == -1)
	{
		threadNum = sysconf(_SC_NPROCESSORS_ONLN) * 2;
	}
	threads_.resize(threadNum);
}

int CThreadPool::init()
{
	int res = 0;
	shutdown_ = RUNNING;
	res = pthread_mutex_init(&lock_, NULL);
	if(res != 0)
	{
		ERROR_LOG("pthread_mutex_init error: %d", res);	
		return -1;
	}

	res = pthread_cond_init(&notify_, NULL);
	if(res != 0)
	{
		ERROR_LOG("pthread_cond_init error: %d", res);	
		return -2;
	}

	for(std::vector<pthread_t>::size_type i = 0; i < threads_.size(); ++i)
	{
		pthread_t id;
		res = pthread_create(&id, NULL, threadFunc, this);		
		if(res != 0)
		{
			ERROR_LOG("pthread_create error: %d", res);	
			return -3;
		}
		threads_[i] = id;
	}
	return res;
}

CThreadPool::~CThreadPool() 
{
	destory(SHUTDOWN_GRACEFUL);
	if(shutdown_ != INVALID)
	{
		pthread_mutex_destroy(&lock_);
		pthread_cond_destroy(&notify_);
	}
}

int CThreadPool::add(void (*f)(void*), void* p)
{
	if(shutdown_)
	{
		return -1;	
	}

	int ret = 0;
	CTask task;
	task.callBack = f;	
	task.param = p;
	ret = pthread_mutex_lock(&lock_);
	CHECK_ERROR_RETURN(ret, -2);
	tasks_.push(task);
	ret = pthread_mutex_unlock(&lock_);
	CHECK_ERROR_RETURN(ret, -3);

	ret = pthread_cond_signal(&notify_);
	CHECK_ERROR_RETURN(ret, -4);
	return ret;
}

int CThreadPool::destory(int flag)
{
	if(shutdown_ != RUNNING)
	{
		return -1;
	}

	int ret = 0;
	ret = pthread_mutex_lock(&lock_);
	CHECK_ERROR_RETURN(ret, -2);
	shutdown_ = flag;  
	ret = pthread_mutex_unlock(&lock_);
	CHECK_ERROR_RETURN(ret, -3);

	ret = pthread_cond_broadcast(&notify_);
	CHECK_ERROR_RETURN(ret, -4);

	for(std::vector<pthread_t>::size_type i = 0; i < threads_.size(); ++i)
	{
		pthread_join(threads_[i], NULL);	
	}
	return 0;
}

