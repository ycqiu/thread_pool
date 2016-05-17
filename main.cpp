#include <iostream>
#include "log.h"
#include "thread_pool.h"

using namespace std;

void f(void* arg)
{
	cout << arg << endl;
	//sleep(1);
}

int main()
{
	INIT_LOG("pool");
	CThreadPool pool(2);
	//CThreadPool pool;
	pool.init();
	for(int i = 0; i < 10; ++i)
	{
		pool.add(f, (void*) 1);
	}
	//pool.destory(CThreadPool::SHUTDOWN_GRACEFUL);
	//pool.destory(CThreadPool::SHUTDOWN_IMMEDIATE);
	return 0;
}

