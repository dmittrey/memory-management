.PHONY: test run

test: test.cpp parallel_memcpy.cpp pool.h parallel_memcpy.h
	c++ -std=c++17 -O2 -pthread -Wall -Wextra test.cpp parallel_memcpy.cpp -o test

run: test
	./test
