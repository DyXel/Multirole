FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
	apt-get install -y --no-install-recommends \
		build-essential \
		cmake \
		pkg-config \
		libasio-dev \
		libboost-date-time-dev \
		libboost-dev \
		libfmt-dev \
		libgit2-dev \
		nlohmann-json3-dev \
		meson \
		ninja-build \
		libspdlog-dev \
		libsqlite3-dev && \
	rm -rf /var/lib/apt/lists/*

#RUN wget https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3-Linux-x86_64.sh \
#	-q -O /tmp/cmake-install.sh \
#	&& chmod u+x /tmp/cmake-install.sh \
#	&& /tmp/cmake-install.sh --skip-license \
#	&& rm /tmp/cmake-install.sh

WORKDIR /multirole

COPY . .

RUN meson setup build && cd build && ninja

COPY etc/config.json multirole-config.json

EXPOSE 7911 7922

CMD [ "./multirole" ]
