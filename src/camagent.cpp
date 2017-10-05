/*
 Name        : camagent.cpp
 Author      : Xiaomeng Lu
 Version     : 0.9
 Copyright   : SVOM Group, NAOC
 Description : GWAC观测系统相机控制程序
 */

#include <iostream>
#include "globaldef.h"
#include "daemon.h"
#include "GLog.h"
#include "cameracs.h"
#include "parameter.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
GLog gLog;

int main(int argc, char **argv) {
	param_config param;
	if (param.bShowImg) system("ds9&");

	boost::asio::io_service ios;
	boost::asio::signal_set signals(ios, SIGINT, SIGTERM);  // interrupt signal
	signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));

	if (!MakeItDaemon(ios)) return 1;
	if (!isProcSingleton(gPIDPath)) {
		gLog.Write("%s is already running or failed to access PID file", DAEMON_NAME);
		return 2;
	}

	gLog.Write("Try to launch %s %s %s as daemon", DAEMON_NAME, DAEMON_VERSION, DAEMON_AUTHORITY);
	// 主程序入口
	cameracs ccs;
	if (ccs.Start()) {
		gLog.Write("Daemon goes running");
		ios.run();
		ccs.Stop();
	}
	else gLog.Write(NULL, LOG_FAULT, "Fail to launch %s", DAEMON_NAME);
	gLog.Write("Daemon stopped");

	return 0;
}
