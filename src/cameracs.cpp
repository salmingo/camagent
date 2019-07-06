/*
 * @file cameracs.cpp 相机控制软件声明文件, 实现天文相机工作流程
 */

#include "cameracs.h"
#include "globaldef.h"
#include "GLog.h"

cameracs::cameracs(boost::asio::io_service* ios) {
}

cameracs::~cameracs() {
}

bool cameracs::StartService() {
	param_ = boost::make_shared<ConfigParameter>();
	if (!param_->Load(gConfigPath)) {
		_gLog.Write(LOG_FAULT, NULL, "failed to load configured parameters");
		return false;
	}
	if (!connect_camera()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect camera");
		return false;
	}
	if (!connect_filter()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect filter");
		return false;
	}
	return false;
}

void cameracs::StopService() {

}

/////////////////////////////////////////////////////////////////////////////
/* 建立并维护与硬件设备的连接 */
bool cameracs::connect_camera() {
	return false;
}

bool cameracs::connect_filter() {
	return false;
}

void cameracs::disconnect_camera() {

}

void cameracs::disconnect_filter() {

}
/////////////////////////////////////////////////////////////////////////////
