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
GLog g_Log;

int main(int argc, char **argv) {
	if (argc >= 2) {// 处理命令行参数
		if (strcmp(argv[1], "-d") == 0) {
			config_parameter param;
			param.Init("camagent.xml");
		}
		else {
			printf("Usage: camagent <-d>\n");
		}
	}
	else {// 常规工作模式
		config_parameter param;
		param.Load(gConfigPath);
		if (param.bShowImg) system("ds9&");
		if (param.bUpdate)  system("mhssvau&");

		boost::asio::io_service ios;
		boost::asio::signal_set signals(ios, SIGINT, SIGTERM);  // interrupt signal
		signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));

		if (!MakeItDaemon(ios)) return 1;
		if (!isProcSingleton(gPIDPath)) {
			g_Log.Write("%s is already running or failed to access PID file", DAEMON_NAME);
			return 2;
		}

		g_Log.Write("Try to launch %s %s %s as daemon", DAEMON_NAME, DAEMON_VERSION, DAEMON_AUTHORITY);
		// 主程序入口
		cameracs ccs(&ios);
		if (ccs.Start()) {
			g_Log.Write("Daemon goes running");
			ios.run();
			ccs.Stop();
			g_Log.Write("Daemon stopped");
		}
		else g_Log.Write(LOG_FAULT, NULL, "Fail to launch %s", DAEMON_NAME);
	}

	return 0;
}
