FROM ubuntu:20.04

# Install all the stuff we are going to need in one go.
RUN apt-get update && \
	apt-get --no-install-recommends --yes install \
		build-essential \
		ca-certificates \
		g++ \
		libfmt-dev \
		libgit2-dev \
		libsqlite3-dev \
		libssl-dev \
		meson \
		ninja-build \
		pkg-config \
		python3 \
		wget

# Build and install boost libraries (we'll only need filesystem to be compiled),
# we require >=1.75.0 because it has Boost.JSON and apt doesn't have it.
WORKDIR /root/boost-src
RUN wget http://sourceforge.net/projects/boost/files/boost/1.75.0/boost_1_75_0.tar.bz2 && \
	tar --bzip2 -xf boost_1_75_0.tar.bz2 && \
	cd boost_1_75_0 && \
	./bootstrap.sh --prefix=/usr/ --with-libraries=filesystem && \
	./b2 install

# Build multirole.
WORKDIR /root/multirole-src
COPY src/ ./src/
COPY meson.build .
RUN BOOST_ROOT=/usr/ meson setup build --buildtype=release && \
	cd build && \
	ninja

# Setup the final environment.
WORKDIR /multirole
COPY etc/config.json .
COPY util/area-zero.py .
RUN cp /root/multirole-src/build/hornet . && \
	cp /root/multirole-src/build/multirole .

# Clean up.
RUN rm -rf /var/lib/apt/lists/* && rm -rf /root/boost-src && rm -rf /root/multirole-src

# Execute.
EXPOSE 7922 7911 34343 62672 49382 43632
CMD [ "./area-zero.py" ]
