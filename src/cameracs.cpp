/*
 * @file cameracs.cpp 相机控制软件声明文件, 实现天文相机工作流程
 */

#include "cameracs.h"

cameracs::cameracs(boost::asio::io_service* ios) {
}

cameracs::~cameracs() {
}

bool cameracs::StartService() {
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
