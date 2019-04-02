FROM debian:stretch

# Make no effort to shrink this image as it is only for local development and
# we want to be able to exploit layer caching

RUN apt-get -y update
RUN apt-get -y --no-install-recommends install ssh tar gzip
RUN apt-get -y --no-install-recommends install curl ca-certificates
RUN apt-get -y --no-install-recommends install git
RUN apt-get -y --no-install-recommends install build-essential pkg-config 
RUN apt-get -y --no-install-recommends install debhelper fakeroot
RUN apt-get -y --no-install-recommends install libfreetype6-dev 
RUN apt-get -y --no-install-recommends install libogg-dev libvorbis-dev
RUN apt-get -y --no-install-recommends install libaldmb1-dev libtheora-dev 

RUN curl -O -L https://github.com/Kitware/CMake/releases/download/v3.13.4/cmake-3.13.4-Linux-x86_64.tar.gz
RUN tar -C /opt -xf cmake-3.13.4-Linux-x86_64.tar.gz
ENV PATH="/opt/cmake-3.13.4-Linux-x86_64/bin:${PATH}"

RUN mkdir lib-allegro \
&& cd lib-allegro/ \
&& curl -O -L https://github.com/liballeg/allegro5/releases/download/4.4.3.1/allegro-4.4.3.1.tar.gz \
&& tar -C . -xf allegro-4.4.3.1.tar.gz \
&& cd allegro-4.4.3.1 \
&& mkdir build-liballegro \
&& cd build-liballegro \
&& cmake -D SHARED=off -D WANT_ALLEGROGL=off -D WANT_LOADPNG=off -D WANT_LOGG=off \
         -D WANT_JPGALLEG=off -D WANT_EXAMPLES=off -D WANT_TOOLS=off \
         -D WANT_TESTS=off -D WANT_DOCS=off -DCMAKE_BUILD_TYPE=Debug .. \
&& make && make install && cd ../../../

CMD /bin/bash
