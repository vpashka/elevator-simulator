# Version: 0.0.1
FROM fotengauer/altlinux-p8
MAINTAINER Pavel Vainerman <pv@etersoft.ru>
RUN apt-get update \
 && apt-get install -y apt-repo wget etersoft-build-utils gcc5-c++ \
 && apt-repo add http://ftp.etersoft.ru/pub/Etersoft/LINUX@Etersoft/p8 branch/x86_64 addon \
 && apt-repo add http://ftp.etersoft.ru/pub/Etersoft/LINUX@Etersoft/p8 branch/noarch addon

RUN apt-get update \
 && apt-get -y install libsobjectizer-devel \
 && apt-get clean

RUN cd /tmp/ \
 && wget https://github.com/vpashka/elevator-simulator/archive/master.zip \
 && unzip master.zip && rm -rf master.zip \
 && cd elevator-simulator-master \
 && ./autogen.sh && ./configure && make \
 && src/Control/tests/tests \
 && make install

CMD ["/bin/bash"]
