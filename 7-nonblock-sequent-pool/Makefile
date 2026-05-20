.PHONY: run build

build:
	c++ -std=c++17 -O2 -pthread test.cpp pool_registry.cpp -o test-default && \
	c++ -std=c++17 -O2 -pthread -DUSE_MUTEX_POOL test.cpp pool_registry.cpp -o test-mutex-pool && \
	c++ -std=c++17 -O2 -pthread -DUSE_LOCKFREE_POOL test.cpp pool_registry.cpp -o test-lockfree-pool && \
	c++ -std=c++17 -O2 -pthread -DUSE_THREAD_LOCAL_POOL test.cpp pool_registry.cpp -o test-thread-local-pool

run:
	./test-default && echo '---' && ./test-mutex-pool && echo '---' && ./test-lockfree-pool && echo '---' && ./test-thread-local-pool