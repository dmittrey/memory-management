build: test.cpp pool.cpp
	c++ -std=c++17 -O2 test.cpp pool.cpp -o test-default && \
	c++ -std=c++17 -O2 -DUSE_POOL_ALLOCATOR test.cpp pool.cpp -o test-pool

.PHONY: run
run:
	./test-default && ./test-pool