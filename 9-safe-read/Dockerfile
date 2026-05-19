FROM gcc:14-bookworm

WORKDIR /work/safe-read

COPY . .

RUN c++ -std=c++17 -O2 -Wall -Wextra test.cpp safe_read.cpp -o test-release && \
    c++ -std=c++17 -O0 -g -Wall -Wextra -DDEBUG_TRACE test.cpp safe_read.cpp -o test-debug

CMD ["sh", "-c", "./test-release && echo '---' && ./test-debug"]
