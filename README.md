# Compile

## Install 3rdparty on Windows 10

```bash
pacman -S git diffutils pkg-config mingw-w64-x86_64-yasm mingw-w64-x86_64-x264 mingw-w64-x86_64-x265 mingw-w64-x86_64-toolchain mingw-w64-x86_64-python3 mingw-w64-python-pip python3-devel

git clone https://git.ffmpeg.org/ffmpeg.git ffmpeg
cd ffmpeg
mkdir install && mkdir install/release && mkdir install/debug && mkdir install/release/x86_64 && mkdir install/debug/x86_64 && mkdir install/release/x86 && mkdir install/debug/x86

./configure --arch=x86_64  --enable-shared --prefix=./install/release/x86_64 && make -j4 && make install -j4 && ./configure --arch=x86_64 --enable-shared --enable-debug --prefix=./install/debug/x86_64

./configure --arch=x86 --enable-shared --prefix=./install/release/x86 && make -j4 && make install -j4 && ./configure --arch=x86 --enable-shared --enable-debug --prefix=./install/debug/x86 && make -j4 && make install -j4

# --enable-libx264 --enable-libx265 --enable-ffplay --disable-x86asm

pacman -S mingw-w64-x86_64-openssl mingw-w64-x86_64-zlib mingw-w64-libssh mingw-w64-libssh2 mingw-w64-mbedtls mingw-w64-gnutls 

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

wget https://curl.se/download/curl-8.0.1.tar.gz && tar -xvf curl-8.0.1.tar.gz
cd curl-8.0.1 

mkdir install && mkdir install/release && mkdir install/debug && mkdir install/release/x86_64 && mkdir install/debug/x86_64 && mkdir install/release/x86 && mkdir install/debug/x86


./configure --enable-static --enable-shared --with-openssl --with-gnutls --with-mbedtls --with-default-ssl-backend=openssl --with-zlib --with-libssh2 --prefix=/home/19115/Compile/curl-8.0.1/install/release/x86_64 && make -j4 && make install -j4

./configure --enable-static --enable-shared --with-openssl --with-gnutls --with-mbedtls --with-default-ssl-backend=openssl --with-zlib --with-libssh2 --enable-debug --prefix=/home/19115/Compile/curl-8.0.1/install/debug/x86_64 && make -j4 && make install -j4

# CC=i686-w64-mingw32-gcc CFLAGS="-O2 -march=i686" ./configure --enable-static --enable-shared --with-openssl --with-gnutls --with-mbedtls --with-default-ssl-backend=openssl --with-zlib --with-libssh2 --prefix=/home/19115/Compile/curl-8.0.1/install/release/x86 && make -j4 && make install -j4

# CC=i686-w64-mingw32-gcc CFLAGS="-O2 -march=i686" ./configure --enable-static --enable-shared --with-openssl --with-gnutls --with-mbedtls --with-default-ssl-backend=openssl --with-zlib --with-libssh2 --enable-debug --prefix=/home/19115/Compile/curl-8.0.1/install/debug/x86 && make -j4 && make install -j4




# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

curl -LO https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz && tar -xzvf libevent-2.1.12-stable.tar.gz

cd libevent-2.1.12-stable

mkdir install && mkdir install/release && mkdir install/debug && mkdir install/release/x86_64 && mkdir install/debug/x86_64 && mkdir install/release/x86 && mkdir install/debug/x86

mkdir build && cd build

export CFLAGS="-m64" && export LDFLAGS="-m64" && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/libevent-2.1.12-stable/install/release/x86_64 -DENABLE_SHARED=TRUE  -DENABLE_STATIC=TRUE ../ && make -j4 && make install -j4

export CFLAGS="-m64" && export LDFLAGS="-m64" && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/libevent-2.1.12-stable/install/debug/x86_64 -DENABLE_SHARED=TRUE  -DENABLE_STATIC=TRUE ../ && make -j4 && make install -j4

export CFLAGS="-m32" && export LDFLAGS="-m32" && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/libevent-2.1.12-stable/install/release/x86 -DENABLE_SHARED=TRUE  -DENABLE_STATIC=TRUE ../ && make -j4 && make install -j4

export CFLAGS="-m32" && export LDFLAGS="-m32" && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/libevent-2.1.12-stable/install/debug/x86 -DENABLE_SHARED=TRUE  -DENABLE_STATIC=TRUE ../ && make -j4 && make install -j4

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git jpeg-turbo
cd jpeg-turbo
mkdir install && mkdir install/release && mkdir install/debug && mkdir install/release/x86_64 && mkdir install/debug/x86_64 && mkdir install/release/x86 && mkdir install/debug/x86

mkdir build && cd build

export CFLAGS="-m64" && export LDFLAGS="-m64" && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DWITH_JPEG8=1 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/jpeg-turbo/install/release/x86_64 -DENABLE_SHARED=TRUE  -DENABLE_STATIC=TRUE ../ && make -j4 && make install -j4

export CFLAGS="-m64" && export LDFLAGS="-m64" && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug -DWITH_JPEG8=1 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/jpeg-turbo/install/debug/x86_64 -DENABLE_SHARED=TRUE  -DENABLE_STATIC=TRUE ../ && make -j4 && make install -j4


export CFLAGS="-m32" && export LDFLAGS="-m32" && cmake -G"MSYS Makefiles" -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_BUILD_TYPE=Release -DWITH_JPEG8=1 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/jpeg-turbo/install/release/x86 -DENABLE_SHARED=TRUE  -DENABLE_STATIC=TRUE ../ && make -j4 && make install -j4


export CFLAGS="-m32" && export LDFLAGS="-m32" && cmake -G"MSYS Makefiles" -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_BUILD_TYPE=Debug -DWITH_JPEG8=1 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/jpeg-turbo/install/debug/x86 -DENABLE_SHARED=TRUE  -DENABLE_STATIC=TRUE ../ && make -j4 && make install -j4

# -DCMAKE_BUILD_TYPE=Debug/Release CFLAGS/LDFLAGS=-m32/-m64

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
pacman -S meson

wget https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.5.tar.gz && tar -xzvf 1.9.5.tar.gz && cd jsoncpp-1.9.5


mkdir install && mkdir install/release && mkdir install/debug && mkdir install/release/x86_64 && mkdir install/debug/x86_64 && mkdir install/release/x86 && mkdir install/debug/x86

mkdir build && cd build

export CFLAGS="-m64" && export LDFLAGS="-m64" && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/jsoncpp-1.9.5/install/release/x86_64 ../ && make -j4 && make install -j4

export CFLAGS="-m64" && export LDFLAGS="-m64" && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON  -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/jsoncpp-1.9.5/install/debug/x86_64 ../  && make -j4 && make install -j4


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

wget https://github.com/numpy/numpy/archive/refs/tags/v1.24.3.tar.gz && tar -xzvf v1.24.3.tar.gz && cd numpy-1.24.3

mkdir build && cd build

python setup.py build -j 4 install --prefix $HOME/.local

CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++\
python setup.py build -j 4 install --prefix C:/msys64/home/19115/Compile/numpy-1.24.3/install/release/x86_64
make -j4 && make install -j4 -DESTDIR=C:/msys64/home/19115/Compile/numpy-1.24.3/install/release/x86_64

CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ FC=x86_64-w64-mingw32-gfortran \
python ../setup.py  --debug --prefix=/home/19115/Compile/numpy-1.24.3/install/debug/x86_64
make -j4 && make install -j4 -DESTDIR=/home/19115/Compile/numpy-1.24.3/install/debug/x86_64

CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ FC=i686-w64-mingw32-gfortran \
python ../setup.py --prefix=/home/19115/Compile/numpy-1.24.3/install/release/x86
make -j4 && make install -j4 -DESTDIR=/home/19115/Compile/numpy-1.24.3/install/release/x86

CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ FC=i686-w64-mingw32-gfortran \
python ../setup.py --debug --prefix=/home/19115/Compile/numpy-1.24.3/install/debug/x86
make -j4 && make install -j4 -DESTDIR=/home/19115/Compile/numpy-1.24.3/install/debug/x86


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

wget https://github.com/opencv/opencv/archive/refs/tags/4.7.0.tar.gz && tar -xzvf 4.7.0.tar.gz && cd opencv-4.7.0
mkdir build && cd build

# 在类 unix 平台需要取消编译支持 OBSENSOR_MSMF
export CFLAGS="-m64" && export LDFLAGS="-m64" && cmake -G"MSYS Makefiles" -DWITH_MSMF=OFF -DWITH_OBSENSOR=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_INSTALL_PREFIX=../install/release/x86_64 ../ && make -j4 && make install -j4

export CFLAGS="-m64" && export LDFLAGS="-m64" && cmake -G"MSYS Makefiles" -DWITH_MSMF=OFF -DWITH_OBSENSOR=OFF -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON  -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/opencv-4.7.0/install/debug/x86_64 ../  && make && make install


export CFLAGS="-m32" && export LDFLAGS="-m32" && cmake -G"MSYS Makefiles" -DWITH_MSMF=OFF -DWITH_OBSENSOR=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_SYSTEM_PROCESSOR=x86 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/opencv-4.7.0/install/release/x86 ../ && make -j4 && make install -j4

export CFLAGS="-m32" && export LDFLAGS="-m32" && cmake -G"MSYS Makefiles" -DWITH_MSMF=OFF -DWITH_OBSENSOR=OFF -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON  -DCMAKE_SYSTEM_PROCESSOR=x86 -DCMAKE_INSTALL_PREFIX=/home/19115/Compile/opencv-4.7.0/install/debug/x86 ../  && make -j4 && make install -j4

# 以下指令需要在 windows 命令行下使用 msvc 编译器执行

cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_INSTALL_PREFIX=../install/release/x86_64 ../ && cmake --build . --target install  && cd .. && rmdir /s /q build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_SYSTEM_PROCESSOR=x86_64 -DCMAKE_INSTALL_PREFIX=../install/debug/x86_64 ../ && cmake --build . --target install && cd .. && rmdir /s /q build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_SYSTEM_PROCESSOR=x86 -DCMAKE_INSTALL_PREFIX=../install/release/x86 ../ && cmake --build . --target install && cd .. && rmdir /s /q build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_SYSTEM_PROCESSOR=x86 -DCMAKE_INSTALL_PREFIX=../install/debug/x86 ../ && cmake --build . --target install && cd .. && rmdir /s /q build && mkdir build && cd build


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

wget https://github.com/python/cpython/archive/refs/tags/v3.10.11.tar.gz && tar -xzvf v3.10.11.tar.gz && cd cpython-3.10.11

PCbuild\build.bat -t Clean  && PCbuild\build.bat -t Build -p x64 -c Release
# PCbuild\build.bat -t Clean  && PCbuild\build.bat -t Build -p x64 -c Debug\
# PCbuild\build.bat -t Clean  && PCbuild\build.bat -t Build -p Win32 -c Debug
# PCbuild\build.bat -t Clean  && PCbuild\build.bat -t Build -p Win32 -c Release
```

## TODO

- [√] 参数管理
- [√] 任务和任务调度管理
- [√] 接口服务和任务调度接口管理
- [√] 任务：流媒体拉流和推流（单线程）
- [ ] 任务：算法分析
- [ ] 多端控制
