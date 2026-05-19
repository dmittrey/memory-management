FROM gcc:14-bookworm

WORKDIR /work/sequent-pool

COPY . .

RUN c++ -std=c++17 -O2 test.cpp -o test-default && \
    c++ -std=c++17 -O2 -DUSE_POOL_ALLOCATOR test.cpp pool.cpp -o test-pool

CMD ["sh", "-c", "./test-default && echo '---' && ./test-pool"]
