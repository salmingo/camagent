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

bool cameracs::StartService() {
	/* 注册消息并尝试启动消息队列 */
	/* 尝试连接相机 */
	/* 启动其它服务 */
	/* 启动定时器 */

	return true;
}

void cameracs::StopService() {
	/* 销毁消息队列 */
	/* 释放定时器 */
	/* 断开与相机连接 */
}

void cameracs::MonitorVersion() {
	_gLog.Write("version update will terminate program");
	ios_->stop();
}

//////////////////////////////////////////////////////////////////////////////
void cameracs::register_messages() {
	const CBSlot &slot01 = boost::bind(&cameracs::on_connect_general_control, this, _1, _2);
	const CBSlot &slot02 = boost::bind(&cameracs::on_receive_general_control, this, _1, _2);
	const CBSlot &slot03 = boost::bind(&cameracs::on_close_general_control,   this, _1, _2);
	const CBSlot &slot04 = boost::bind(&cameracs::on_connect_camera,          this, _1, _2);
	const CBSlot &slot05 = boost::bind(&cameracs::on_prepare_expose,          this, _1, _2);
	const CBSlot &slot06 = boost::bind(&cameracs::on_process_expose,          this, _1, _2);
	const CBSlot &slot07 = boost::bind(&cameracs::on_complete_expose,         this, _1, _2);
	const CBSlot &slot08 = boost::bind(&cameracs::on_abort_expose,            this, _1, _2);
	const CBSlot &slot09 = boost::bind(&cameracs::on_fail_expose,             this, _1, _2);

	RegisterMessage(MSG_CONNECT_GENERAL_CONTROL, slot01);
	RegisterMessage(MSG_RECEIVE_GENERAL_CONTROL, slot02);
	RegisterMessage(MSG_CLOSE_GENERAL_CONTROL,   slot03);
	RegisterMessage(MSG_CONNECT_CAMERA,          slot04);
	RegisterMessage(MSG_PREPARE_EXPOSE,          slot05);
	RegisterMessage(MSG_PROCESS_EXPOSE,          slot06);
	RegisterMessage(MSG_COMPLETE_EXPOSE,         slot07);
	RegisterMessage(MSG_ABORT_EXPOSE,            slot08);
	RegisterMessage(MSG_FAIL_EXPOSE,             slot09);
}

void cameracs::on_connect_general_control(const long, const long) {

}

void cameracs::on_receive_general_control(const long, const long) {

}
void cameracs::on_close_general_control(const long, const long) {

}

void cameracs::on_connect_camera(const long, const long) {

}

void cameracs::on_prepare_expose(const long, const long) {

}

void cameracs::on_process_expose(const long, const long) {

}

void cameracs::on_complete_expose(const long, const long) {

}

void cameracs::on_abort_expose(const long, const long) {

}

void cameracs::on_fail_expose(const long, const long) {

}

void cameracs::on_wait_time(const long, const long) {

}
