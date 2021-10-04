FROM debian:bullseye-slim as base

# Install all the stuff we are going to need in one go.
RUN apt-get update && \
	apt-get --no-install-recommends --yes install \
		build-essential \
		ca-certificates \
		libfmt-dev \
		libgit2-dev \
		libsqlite3-dev \
		libssl-dev \
		python3 \
		libtcmalloc-minimal4 && \
	rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

FROM base as build-base

RUN apt-get update && \
	apt-get --no-install-recommends --yes install \
		g++ \
		meson \
		ninja-build \
		pkg-config \
		build-essential \
		ca-certificates \
		tar \
		bzip2 \
		wget && \
	rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*


# Build and install boost libraries (we'll only need filesystem to be compiled),
# we require >=1.75.0 because it has Boost.JSON and apt doesn't have it.
FROM build-base as boost-builder

WORKDIR /root/boost-src
RUN wget -O - http://sourceforge.net/projects/boost/files/boost/1.75.0/boost_1_75_0.tar.bz2 | tar --bzip2 -xf - && \
	cd boost_1_75_0 && \
	./bootstrap.sh --prefix=/usr/local/boost --with-libraries=filesystem && \
	./b2 install && \
	cd ..

# Build multirole.
FROM build-base as multirole-builder

WORKDIR /root/multirole-src
COPY src/ ./src/
COPY meson.build .
COPY meson_options.txt .
COPY --from=boost-builder /usr/local/boost /usr/local/boost
RUN BOOST_ROOT=/usr/local/boost/ meson setup build --buildtype=release && \
	cd build && \
	ninja

# Setup the final environment.
FROM base

WORKDIR /multirole
COPY etc/config.json .
COPY util/area-zero.py .
COPY --from=boost-builder /usr/local/boost/lib/libboost_filesystem.so /usr/lib/libboost_filesystem.so.1.75.0
COPY --from=multirole-builder /root/multirole-src/build/hornet .
COPY --from=multirole-builder /root/multirole-src/build/multirole .

# Execute.
EXPOSE 7922 7911 34343 62672 49382 43632
CMD [ "python3", "./area-zero.py" ]
