/*!
 Name        : camagent.cpp
 Author      : Xiaomeng Lu
 Version     : 0.9
 Copyright   : SVOM Group, NAOC
 Description : GWAC观测系统相机控制程序
 */

#include "globaldef.h"
#include "daemon.h"
#include "GLog.h"
#include "ConfigParam.h"
#include "CDs9.h"
#include "cameracs.h"

#include "CameraFLICCD.h"

//////////////////////////////////////////////////////////////////////////////
GLog _gLog;

int main(int argc, char **argv) {
	if (argc >= 2) {// 处理命令行参数
		CameraFLICCD fli;
		CameraBase *camera = &fli;
		if (camera->Connect()) {
			boost::asio::io_service ios;
			boost::asio::signal_set signals(ios, SIGINT, SIGTERM); // interrupt signal
			signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));

			ios.run();
			camera->DisConnect();
		}
//		if (strcmp(argv[1], "-d") == 0) {
//			ConfigParameter cfg;
//			cfg.Init("camagent.xml");
//		}
//		else {
//			printf("Usage: camagent <-d>\n");
//		}
	}
	else {// 常规工作模式
		ConfigParameter cfg;
		cfg.Load(gConfigPath);
		if (cfg.imgshow) CDs9::StartDS9();

		boost::asio::io_service ios;
		boost::asio::signal_set signals(ios, SIGINT, SIGTERM); // interrupt signal
		signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));

		if (!MakeItDaemon(ios))
			return 1;
		if (!isProcSingleton(gPIDPath)) {
			_gLog.Write("%s is already running or failed to access PID file",
			DAEMON_NAME);
			return 2;
		}

		_gLog.Start(gLogDir, gLogPrefix);
		_gLog.Write("Try to launch %s %s %s as daemon", DAEMON_NAME,
				DAEMON_VERSION, DAEMON_AUTHORITY);
		// 主程序入口
		cameracs ccs(&ios);
		if (ccs.StartService()) {
			_gLog.Write("Daemon goes running");
			ios.run();
			ccs.StopService();
			_gLog.Write("Daemon stop running");
		}
		else {
			_gLog.Write(LOG_FAULT, NULL, "Fail to launch %s", DAEMON_NAME);
		}
		_gLog.Stop();
	}

	return 0;
}
