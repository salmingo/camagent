bin_PROGRAMS=camagent
camagent_SOURCES= daemon.cpp GLog.cpp IOServiceKeep.cpp MessageQueue.cpp NTPClient.cpp tcpasio.cpp udpasio.cpp \
				  CameraBase.cpp \
				  apgSampleCmn.cpp \
				  cameracs.cpp camagent.cpp

AM_CPPFLAGS=-I/usr/local/include/libapogee-3.0
camagent_LDFLAGS=-L/usr/local/lib
COMM_LIBS=-lpthread -lcurl -lm -lrt -lcfitsio -lxpa
BOOST_LIBS=-lboost_system-mt -lboost_thread-mt -lboost_date_time-mt -lboost_filesystem-mt
camagent_LDADD=${COMM_LIBS} ${BOOST_LIBS}
