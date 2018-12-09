/*
 * @file cameracs.cpp 相机控制软件声明文件, 实现天文相机工作流程
 */
#include <boost/make_shared.hpp>
#include <string.h>
#include "globaldef.h"
#include "GLog.h"
#include "cameracs.h"
#include "CameraApogee.h"
#include "CameraAndorCCD.h"
#include "CameraGY.h"
#include "FilterCtrlFLI.h"

using namespace std;
using namespace boost;

cameracs::cameracs(boost::asio::io_service* ios) {
	ostype_ = 1;
	ios_    = ios;
	cmdexp_ = EXPOSE_INIT;
}

cameracs::~cameracs() {
}

bool cameracs::StartService() {
	param_  = boost::make_shared<Parameter>();
	param_->Load(gConfigPath);
	ascproto_ = make_ascproto();
	bufrcv_.reset(new char[TCP_PACK_SIZE]);
	/* 注册消息并尝试启动消息队列 */
	string name = "msgque_";
	name += DAEMON_NAME;
	register_messages();
	if (!Start(name.c_str())) return false;
	if (!connect_camera()) return false;
	connect_filter();
	if (param_->bNTP) {
		ntp_ = make_ntp(param_->hostNTP.c_str(), 123, param_->maxClockDiff);
		ntp_->EnableAutoSynch(false);
	}
	// 启动线程

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
	switch(param_->camType) {
	case 1: // Andor CCD
	{
		boost::shared_ptr<CameraAndorCCD> ccd = boost::make_shared<CameraAndorCCD>();
		camctl_ = to_cambase(ccd);
	}
		break;
	case 2:	// FLI CCD
		break;
	case 3:	// ApogeeU CCD
	{
		boost::shared_ptr<CameraApogee> ccd = boost::make_shared<CameraApogee>();
		camctl_ = to_cambase(ccd);
	}
		break;
	case 4:	// PI CCD
		break;
	case 5:	// GWAC-GY CCD
	{
		boost::shared_ptr<CameraGY> ccd = boost::make_shared<CameraGY>(param_->camIP);
		camctl_ = to_cambase(ccd);
	}
		break;
	default:
		_gLog.Write(LOG_FAULT, NULL, "unknown camera type <%d>", param_->camType);
		return false;
	}

	const NFDevCamPtr nfdev = camctl_->GetCameraInfo();
	if (!camctl_->Connect()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect camera: %s",
				nfdev->errmsg.c_str());
	}
	else {
		// 设置相机工作参数
		camctl_->SetCooler    (param_->coolerset, true);
		camctl_->SetADChannel (param_->ADChannel);
		camctl_->SetReadPort  (param_->readport);
		camctl_->SetReadRate  (param_->readrate);
		camctl_->SetVSRate    (param_->VSRate);
		camctl_->SetGain      (param_->gain);
		const ExpProcCBSlot &slot = boost::bind(&cameracs::expose_process, this, _1, _2, _3);
		camctl_->RegisterExposeProc(slot);
		// 设置相机监测量
		nfcam_ = boost::make_shared<ascii_proto_camera>();
		nfcam_->gid   = param_->gid;
		nfcam_->uid   = param_->uid;
		nfcam_->cid   = param_->cid;
		nfcam_->state = CAMCTL_IDLE;
		nfcam_->errcode = 0;
	}

	return nfdev->connected;
}

bool cameracs::connect_filter() {
	if (param_->bFilter) {

	}
	return true;
}

void cameracs::connect_gtoaes() {
	const TCPClient::CBSlot &slot = boost::bind(&cameracs::handle_connect_gtoaes, this, _1, _2);

	tcptr_ = maketcp_client();
	tcptr_->RegisterConnect(slot);
	tcptr_->AsyncConnect(param_->hostGC, param_->portGC);
}

void cameracs::handle_connect_gtoaes(const long client, const long ec) {
	PostMessage(MSG_CONNECT_GTOAES, ec);
}

void cameracs::handle_receive_gtoaes(const long client, const long ec) {
	PostMessage(ec ? MSG_CLOSE_GTOAES : MSG_RECEIVE_GTOAES);
}

/*
 * 解析由通信协议发送的滤光片组合, 生成本地滤光片集合
 */
void cameracs::resolve_filter() {
	if (!nfobj_->filter.empty()) {
		string &filter = nfobj_->filter;
		char seps[] = "| ";
		listring tokens;

		algorithm::split(tokens, filter, is_any_of(seps), token_compress_on);
		while(tokens.size()) {
			string name = tokens.front();
			tokens.pop_front();
			trim(name);
			filters_.push_back(name);
		}

		nfcam_->filter = filters_[nfobj_->ifilter];
	}
}

/*
 * 检查曝光序列是否结束
 * - 检查当前曝光参数序列是否结束
 * - 检查是否需要切换滤光片
 * - 检查是否需要开始新的循环轮次
 */
bool cameracs::exposure_over() {
	bool rslt(false);
	if (++nfcam_->ifrm == nfcam_->frmcnt) {// 当前曝光参数序列已完成
		if (!filters_.size()) rslt = true;
		else if (++nfcam_->ifilter == filters_.size()) {
			if (++nfcam_->iloop == nfcam_->loopcnt) rslt = true;
			else nfcam_->ifilter = 0;
		}

		if (!rslt) {
			nfcam_->ifrm = 0;
			nfcam_->filter = filters_[nfcam_->ifilter];
		}
	}
	if (rslt) nfobj_.reset();

	return rslt;
}

void cameracs::expose_process(const double left, const double percent, const int state) {
	switch((CAMERA_STATUS) state) {
	case CAMERA_ERROR: // 相机故障
		PostMessage(MSG_FAIL_EXPOSE);
		break;
	case CAMERA_IDLE: // 空闲
		PostMessage(MSG_ABORT_EXPOSE);
		break;
	case CAMERA_EXPOSE:	// 曝光中
		PostMessage(MSG_PROCESS_EXPOSE, long(left * 1E6), long(percent));
		break;
	case CAMERA_IMGRDY:	// 已下载图像数据, 可以执行存储等操作
		PostMessage(MSG_COMPLETE_EXPOSE);
		break;
	default:
		break;
	}
}

void cameracs::process_protocol(apbase proto) {
	string type = proto->type;

	if (iequals(type, APTYPE_OBJECT)) {// 观测目标及曝光参数
		if (!nfobj_.unique()) {
			nfobj_ = from_apbase<ascii_proto_object>(proto);
			_gLog.Write("New sequence: [name=%s, imgtype=%s, expdur=%.3f, frmcnt=%d",
					nfobj_->objname.c_str(), nfobj_->imgtype.c_str(), nfobj_->expdur, nfobj_->frmcnt);
			*nfcam_ = *nfobj_;
			resolve_filter();
		}
		else {
			_gLog.Write(LOG_WARN, NULL, "<object> is rejected for system is busy");
		}
	}
	else if (iequals(type, APTYPE_EXPOSE)) {// 曝光指令
		command_expose(EXPOSE_COMMAND(from_apbase<ascii_proto_expose>(proto)->command));
	}
	else if (iequals(type, APTYPE_TELE)) {// 望远镜指向位置. 平场变更位置
		if (nfobj_.unique()) {
			shared_ptr<ascii_proto_telescope> tele = from_apbase<ascii_proto_telescope>(proto);
			nfobj_->ra  = tele->ra;
			nfobj_->dec = tele->dec;
		}
	}
	else if (iequals(type, APTYPE_FOCUS)) {// 变更焦点位置
		nfcam_->focus = from_apbase<ascii_proto_focus>(proto)->value;
	}
	else if (iequals(type, APTYPE_COOLER)) {// GWAC-GY CCD相机变更制冷温度
		const NFDevCamPtr nfdev = camctl_->GetCameraInfo();
		nfdev->coolerGet = from_apbase<ascii_proto_cooler>(proto)->coolget;
	}
	else if (iequals(type, APTYPE_MCOVER)) {// 变更镜盖状态
		nfcam_->mcstate = from_apbase<ascii_proto_mcover>(proto)->value;
	}
	else if (iequals(type, APTYPE_REG)) {// 注册结果. 反馈观测系统类型
		ostype_ = from_apbase<ascii_proto_reg>(proto)->ostype;
	}
}

void cameracs::command_expose(EXPOSE_COMMAND cmd) {
	if (cmdexp_ != cmd && nfobj_.unique()) {
		if (cmd == EXPOSE_START || cmd == EXPOSE_RESUME) {
			if (filterctl_.unique() && nfobj_->ifrm == nfcam_->ifrm) {// 检查并重定位滤光片

			}
		}
		else if (cmd == EXPOSE_PAUSE) {

		}
		else if (cmd == EXPOSE_STOP) {

		}

		cmdexp_ = cmd;
	}
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

void cameracs::thread_upload() {
	boost::chrono::seconds period(10);
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while(1) {
		cv_statchanged_.wait_for(lck, period);
	}
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
		// 启动线程
		thrd_upload_.reset(new boost::thread(boost::bind(&cameracs::thread_upload, this)));
	}
}

void cameracs::on_receive_gtoaes(const long, const long) {
	char term[] = "\n";	   // 换行符作为信息结束标记
	int len = strlen(term);// 结束符长度
	int pos;      // 标志符位置
	int toread;   // 信息长度
	apbase proto;

	while (tcptr_->IsOpen() && (pos = tcptr_->Lookup(term, len)) >= 0) {
		if ((toread = pos + len) > TCP_PACK_SIZE) {
			_gLog.Write(LOG_FAULT, "cameracs::on_receive_gtoaes",
					"too long message from server");
			tcptr_->Close();
		}
		else {// 读取协议内容并解析执行
			tcptr_->Read(bufrcv_.get(), toread);
			bufrcv_[pos] = 0;

			proto = ascproto_->Resolve(bufrcv_.get());
			// 检查: 协议有效性及设备标志基本有效性
			if (proto.use_count()) process_protocol(proto);
			else {
				_gLog.Write(LOG_FAULT, "cameracs::on_receive_gtoaes",
						"illegal protocol. received: %s", bufrcv_.get());
				tcptr_->Close();
			}
		}
	}

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
