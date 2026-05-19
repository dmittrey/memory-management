FROM gcc:14-bookworm

WORKDIR /work/nonblock-sequent-pool

COPY . .

RUN c++ -std=c++17 -O2 -pthread test.cpp pool_registry.cpp -o test-default && \
    c++ -std=c++17 -O2 -pthread -DUSE_MUTEX_POOL test.cpp pool.cpp mutex_pool.cpp pool_registry.cpp -o test-mutex-pool && \
    c++ -std=c++17 -O2 -pthread -DUSE_LOCKFREE_POOL test.cpp lockfree_pool.cpp pool_registry.cpp -o test-lockfree-pool && \
    c++ -std=c++17 -O2 -pthread -DUSE_THREAD_LOCAL_POOL test.cpp pool.cpp pool_registry.cpp -o test-thread-local-pool

CMD ["sh", "-c", "./test-default && echo '---' && ./test-mutex-pool && echo '---' && ./test-lockfree-pool && echo '---' && ./test-thread-local-pool"]
