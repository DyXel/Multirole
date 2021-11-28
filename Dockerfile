FROM debian:bullseye-slim as base

# Install all the runtime dependencies for Multirole.
RUN apt-get update && \
	apt-get --no-install-recommends --yes install \
		ca-certificates \
		libfmt7 \
		libgit2-1.1 \
		libsqlite3-0 \
		libssl1.1 \
		python3 \
		libtcmalloc-minimal4 && \
	rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

FROM base as build-base

# Install all the development environment that Multirole needs.
RUN apt-get update && \
	apt-get --no-install-recommends --yes install \
		build-essential \
		libfmt-dev \
		libgit2-dev \
		libsqlite3-dev \
		libssl-dev \
		libgoogle-perftools-dev \
		g++ \
		python3-pip \
		ninja-build \
		pkg-config \
		tar \
		bzip2 \
		wget && \
	pip install meson && \
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
RUN BOOST_ROOT=/usr/local/boost/ meson setup build -Dbuildtype=release -Dstrip=true -Db_lto=true -Db_ndebug=true -Duse_tcmalloc=enabled && \
	meson compile -C build

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
