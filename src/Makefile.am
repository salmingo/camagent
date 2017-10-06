bin_PROGRAMS=camagent
camagent_SOURCES=camagent.cpp cameracs.cpp asciiproto.cpp ATimeSpace.cpp daemon.cpp GLog.cpp udp_asio.cpp \
                 ioservice_keep.cpp msgque_base.cpp NTPClient.cpp tcp_asio.cpp FileTransferClient.cpp \
                 CameraBase.cpp \
                 CameraGY.cpp

camagent_LDFLAGS=-L/usr/local/lib
COMM_LIBS=-lpthread -lcurl -lm -lrt -lcfitsio -lxpa
BOOST_LIBS=-lboost_system-mt -lboost_thread-mt -lboost_date_time-mt
camagent_LDADD=${COMM_LIBS} ${BOOST_LIBS}
