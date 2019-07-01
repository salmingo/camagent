bin_PROGRAMS=camagent
#camagent_SOURCES= daemon.cpp GLog.cpp IOServiceKeep.cpp MessageQueue.cpp NTPClient.cpp tcpasio.cpp udpasio.cpp \
#				  VersionMonitor.cpp \
#				  FilterCtrl.cpp FilterCtrlFLI.cpp \
#				  CameraBase.cpp apgSampleCmn.cpp CameraApogee.cpp CameraAndorCCD.cpp CameraGY.cpp \
#				  AsciiProtocol.cpp CDs9.cpp \
#				  FileTransferClient.cpp cameracs.cpp camagent.cpp
camagent_SOURCES=daemon.cpp GLog.cpp CDs9.cpp MessageQueue.cpp IOServiceKeep.cpp \
                 NTPClient.cpp FitsHandler.cpp FilterCtrl.cpp FilterCtrlFLI.cpp \
                 tcpasio.cpp udpasio.cpp CameraBase.cpp \
                 CameraAndor.cpp \
                 cameracs.cpp camagent.cpp

AM_CPPFLAGS=-I/usr/local/include -I/usr/local/include/libapogee-3.0
camagent_LDFLAGS=-L/usr/local/lib

COMM_LIBS=-lpthread -lcurl -lm -lrt -lcfitsio -lxpa
#BOOST_LIBS=-lboost_system -lboost_thread -lboost_date_time -lboost_chrono -lboost_filesystem
BOOST_LIBS=-lboost_system-mt -lboost_thread-mt -lboost_date_time-mt -lboost_chrono-mt -lboost_filesystem-mt
#APOGEE_LIBS=-lapogee
ANDOR_LIBS=-landor
FLI_LIBS=-lfli
#camagent_LDADD=${COMM_LIBS} ${BOOST_LIBS} ${APOGEE_LIBS} ${ANDOR_LIBS} ${FILTER_LIBS}
camagent_LDADD=${COMM_LIBS} ${BOOST_LIBS} ${ANDOR_LIBS} ${FLI_LIBS}
