FROM gcc:latest

RUN apt update && \
	apt install -y \
		libasio-dev \
		libfmt-dev \
		libgit2-dev \
		nlohmann-json3-dev \
		libspdlog-dev \
		libsqlite3-dev && \
	rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3-Linux-x86_64.sh \
	-q -O /tmp/cmake-install.sh \
	&& chmod u+x /tmp/cmake-install.sh \
	&& /tmp/cmake-install.sh --skip-license \
	&& rm /tmp/cmake-install.sh

WORKDIR /multirole

COPY cmake src CMakeLists.txt COPYING ./

RUN cmake . -DIGNIS_MULTIROLE_SYSTEM_DEPS=ON -DCMAKE_BUILD_TYPE=Release && \
	cmake --build . --config Release --parallel

COPY etc/config.json multirole-config.json

EXPOSE 7911 7922

CMD [ "./multirole" ]
