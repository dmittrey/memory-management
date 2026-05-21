FROM gcc:14-bookworm

WORKDIR /work/parallel-copy

COPY . .

RUN c++ -std=c++17 -O2 -pthread -Wall -Wextra test.cpp parallel_memcpy.cpp -o test

CMD ["./test"]
