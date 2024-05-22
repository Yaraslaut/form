# docker build . -t reflection --progress=plain
# clang++ -std=c++26 -freflection -stdlib=libc++

FROM yaraslaut/clang-p2996:latest
#FROM clang-p2996

COPY . /mnt/src
WORKDIR /mnt/src
RUN cmake -S . -B build -G Ninja -D CMAKE_CXX_COMPILER=clang++
RUN cmake --build build --verbose
RUN ./build/src/test
