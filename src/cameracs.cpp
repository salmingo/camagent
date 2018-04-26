/*
 * @file cameracs.cpp 相机控制软件声明文件, 实现天文相机工作流程
 */
#include <boost/make_shared.hpp>
#include "GLog.h"
#include "cameracs.h"

cameracs::cameracs(boost::asio::io_service* ios) {
	ios_ = ios;
}

cameracs::~cameracs() {
}

bool cameracs::Start() {
	/* 注册消息并尝试启动消息队列 */
	/* 尝试连接相机 */
	/* 启动其它服务 */
	/* 启动定时器 */

	return true;
}

void cameracs::Stop() {
	/* 销毁消息队列 */
	/* 释放定时器 */
	/* 断开与相机连接 */
}

void cameracs::MonitorVersion() {
	g_Log.Write("version update will terminate program");
	ios_->stop();
}
