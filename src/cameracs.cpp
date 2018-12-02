/*
 * @file cameracs.cpp 相机控制软件声明文件, 实现天文相机工作流程
 */
#include <boost/make_shared.hpp>
#include "globaldef.h"
#include "GLog.h"
#include "cameracs.h"

#include "FilterCtrlFLI.h"

cameracs::cameracs(boost::asio::io_service* ios) {
	ios_ = ios;
	cmdexp_ = EXPOSE_INIT;
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
bool cameracs::connect_camera() {
	return false;
}

bool cameracs::connect_filter() {
	return false;
}

void cameracs::connect_gtoaes() {
	const TCPClient::CBSlot &slot1 = boost::bind(&cameracs::handle_connect_gtoaes, this, _1, _2);
	const TCPClient::CBSlot &slot2 = boost::bind(&cameracs::handle_receive_gtoaes, this, _1, _2);

	tcptr_ = maketcp_client();
	tcptr_->RegisterConnect(slot1);
	tcptr_->RegisterRead(slot2);
	tcptr_->AsyncConnect(param_->hostGC, param_->portGC);
}

void cameracs::handle_connect_gtoaes(const long client, const long ec) {
	PostMessage(MSG_CONNECT_GTOAES, ec);
}

void cameracs::handle_receive_gtoaes(const long client, const long ec) {
	PostMessage(ec ? MSG_CLOSE_GTOAES : MSG_RECEIVE_GTOAES);
}

//////////////////////////////////////////////////////////////////////////////
void cameracs::thread_tryconnect_gtoaes() {
	int count(0);

	while(1) {
		boost::this_thread::sleep_for(boost::chrono::minutes(++count));
		_gLog.Write("try to connect gtoaes");
		if (count > 5) count = 5;
		connect_gtoaes();
	}
}

void cameracs::thread_tryconnect_camera() {
	int count(0);
	bool success;

	do {
		boost::this_thread::sleep_for(boost::chrono::minutes(++count));
		_gLog.Write("try to connect camera");
		if (count > 5) count = 5;
		success = connect_camera();
	} while(!success);
	if (success) PostMessage(MSG_CONNECT_CAMERA);
}

void cameracs::thread_tryconnect_filter() {
	int count(0);
	bool success;

	do {
		boost::this_thread::sleep_for(boost::chrono::minutes(++count));
		_gLog.Write("try to connect filter controller");
		if (count > 5) count = 5;
		success = connect_filter();
	} while(!success);
	if (success) PostMessage(MSG_CONNECT_FILTER);
}

//////////////////////////////////////////////////////////////////////////////
void cameracs::register_messages() {
	const CBSlot &slot01 = boost::bind(&cameracs::on_connect_gtoaes,  this, _1, _2);
	const CBSlot &slot02 = boost::bind(&cameracs::on_receive_gtoaes,  this, _1, _2);
	const CBSlot &slot03 = boost::bind(&cameracs::on_close_gtoaes,    this, _1, _2);
	const CBSlot &slot04 = boost::bind(&cameracs::on_connect_camera,  this, _1, _2);
	const CBSlot &slot05 = boost::bind(&cameracs::on_connect_filter,  this, _1, _2);
	const CBSlot &slot06 = boost::bind(&cameracs::on_prepare_expose,  this, _1, _2);
	const CBSlot &slot07 = boost::bind(&cameracs::on_process_expose,  this, _1, _2);
	const CBSlot &slot08 = boost::bind(&cameracs::on_complete_expose, this, _1, _2);
	const CBSlot &slot09 = boost::bind(&cameracs::on_abort_expose,    this, _1, _2);
	const CBSlot &slot10 = boost::bind(&cameracs::on_fail_expose,     this, _1, _2);

	RegisterMessage(MSG_CONNECT_GTOAES,  slot01);
	RegisterMessage(MSG_RECEIVE_GTOAES,  slot02);
	RegisterMessage(MSG_CLOSE_GTOAES,    slot03);
	RegisterMessage(MSG_CONNECT_CAMERA,  slot04);
	RegisterMessage(MSG_CONNECT_FILTER,  slot05);
	RegisterMessage(MSG_PREPARE_EXPOSE,  slot06);
	RegisterMessage(MSG_PROCESS_EXPOSE,  slot07);
	RegisterMessage(MSG_COMPLETE_EXPOSE, slot08);
	RegisterMessage(MSG_ABORT_EXPOSE,    slot09);
	RegisterMessage(MSG_FAIL_EXPOSE,     slot10);
}

void cameracs::on_connect_gtoaes(const long ec, const long) {
	if (!ec) {// 注册相机并销毁线程
		interrupt_thread(thrd_gtoaes_);
		// 注册回调函数
		const TCPClient::CBSlot &slot = boost::bind(&cameracs::handle_receive_gtoaes, this, _1, _2);
		tcptr_ = maketcp_client();
		tcptr_->RegisterRead(slot);
		// 注册相机
		_gLog.Write("register camera as [%s:%s:%s]",
				param_->gid.c_str(), param_->uid.c_str(), param_->cid.c_str());

		int n;
		const char *s;
		apreg proto = boost::make_shared<ascii_proto_reg>();
		proto->gid = param_->gid;
		proto->uid = param_->uid;
		proto->cid = param_->cid;
		s = ascproto_->CompactRegister(proto, n);
		tcptr_->Write(s, n);
	}
}

void cameracs::on_receive_gtoaes(const long, const long) {

}

void cameracs::on_close_gtoaes(const long, const long) {
	_gLog.Write(LOG_WARN, NULL, "connection with gtoaes was broken");
	tcptr_->Close();
	thrd_gtoaes_.reset(new boost::thread(boost::bind(&cameracs::thread_tryconnect_gtoaes, this)));
}

void cameracs::on_connect_camera(const long, const long) {
	_gLog.Write("re-connection to camera succeed");
	interrupt_thread(thrd_camera_);
}

void cameracs::on_connect_filter(const long, const long) {
	_gLog.Write("re-connection to filter controller succeed");
	interrupt_thread(thrd_filter_);
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
