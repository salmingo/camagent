/*
 * @file cameracs.cpp 控制程序入口定义文件
 * @date 2017年4月9日
 * @version 0.3
 * @author Xiaomeng Lu
 * @note
 * 功能列表:
 * (1) 响应和反馈网络消息
 * (2) 控制相机工作流程
 * (3) 存储FITS文件
 * (4) 同步NTP时钟
 * (5) 上传FITS文件
 * @date 2017年5月19日
 * - CAMCTL_WAIT_FLAT状态可接收object_info, 更新坐标
 * - 改变延迟时间不足时的变更策略
 */

#include <boost/make_shared.hpp>
#include "globaldef.h"
#include "GLog.h"
#include "cameracs.h"
//////////////////////////////////////////////////////////////////////////////
// Camera API
#include "CameraAndorCCD.h"
#include "CameraApogee.h"
#include "CameraFLICCD.h"
#include "CameraGYCCD.h"
#include "CameraPICCD.h"
//////////////////////////////////////////////////////////////////////////////

cameracs::cameracs(boost::asio::io_service* ios) {
	ios_   = ios;
	param_ = boost::make_shared<param_config>();
	bufrcv_.reset(new char[TCP_PACK_SIZE]);
}

cameracs::~cameracs() {
}

bool cameracs::Start() {
	/* 创建消息队列 */
	string name = "msgque_";
	name += DAEMON_NAME;
	register_messages();
	if (!MessageQueue::Start(name.c_str())) return false;
	/* 连接相机 */
	if (!connect_camera()) return false;

	/* 启动其它服务 */
	start_ntp();
	start_fs();
	connect_server();
	/* 启动线程 */
	thrdPeriod_.reset  (new boost::thread(boost::bind(&cameracs::thread_period_info, this)));
	thrdFreeDisk_.reset(new boost::thread(boost::bind(&cameracs::thread_free_disk,   this)));

	return true;
}

void cameracs::Stop() {
	MessageQueue::Stop();
	interrupt_thread(thrdNetwork_);
	interrupt_thread(thrdCamera_);
	interrupt_thread(thrdPeriod_);
	interrupt_thread(thrdWaitExpose_);
	interrupt_thread(thrdFreeDisk_);
	/* 显示释放资源 */
	if (camPtr_.unique()) camPtr_->Disconnect();
}

/*
 * @note 注册消息
 */
void cameracs::register_messages() {
	const CBSlot &slot1  = boost::bind(&cameracs::on_connect_gc,      this, _1, _2);
	const CBSlot &slot2  = boost::bind(&cameracs::on_receive_gc,      this, _1, _2);
	const CBSlot &slot3  = boost::bind(&cameracs::on_close_gc,        this, _1, _2);
	const CBSlot &slot4  = boost::bind(&cameracs::on_connect_camera,  this, _1, _2);
	const CBSlot &slot5  = boost::bind(&cameracs::on_prepare_expose,  this, _1, _2);
	const CBSlot &slot6  = boost::bind(&cameracs::on_process_expose,  this, _1, _2);
	const CBSlot &slot7  = boost::bind(&cameracs::on_complete_expose, this, _1, _2);
	const CBSlot &slot8  = boost::bind(&cameracs::on_abort_expose,    this, _1, _2);
	const CBSlot &slot9  = boost::bind(&cameracs::on_fail_expose,     this, _1, _2);
	const CBSlot &slot10 = boost::bind(&cameracs::on_complete_wait,   this, _1, _2);

	RegisterMessage(MSG_CONNECT_GC,      slot1);
	RegisterMessage(MSG_RECEIVE_GC,      slot2);
	RegisterMessage(MSG_CLOSE_GC,        slot3);
	RegisterMessage(MSG_CONNECT_CAMERA,  slot4);
	RegisterMessage(MSG_PREPARE_EXPOSE,  slot5);
	RegisterMessage(MSG_PROCESS_EXPOSE,  slot6);
	RegisterMessage(MSG_COMPLETE_EXPOSE, slot7);
	RegisterMessage(MSG_ABORT_EXPOSE,    slot8);
	RegisterMessage(MSG_FAIL_EXPOSE,     slot9);
	RegisterMessage(MSG_COMPLETE_WAIT,   slot10);
}

/*
 * @note 回调函数: 响应网络连接结果
 */
void cameracs::network_connect(const long client, const long ec) {
	PostMessage(MSG_CONNECT_GC, ec);
}

/*
 * @note 回调函数: 响应收到的网络信息
 */
void cameracs::network_receive(const long client, const long ec) {
	PostMessage(!ec ? MSG_RECEIVE_GC : MSG_CLOSE_GC);
}

/*
 * @note 回调函数: 响应相机曝光过程
 */
void cameracs::expose_process(int state, double left, double percent) {
	if      (state == CAMSTAT_ERROR)    PostMessage(MSG_FAIL_EXPOSE);
	else if (state == CAMSTAT_IDLE)     PostMessage(MSG_ABORT_EXPOSE);
	else if (state == CAMSTAT_EXPOSE)   PostMessage(MSG_PROCESS_EXPOSE, (long) (left * 1E6), (long) (percent * 100));
	else if (state == CAMSTAT_READOUT)  PostMessage(MSG_PROCESS_EXPOSE, 0, 100);
	else if (state == CAMSTAT_IMGRDY)   PostMessage(MSG_COMPLETE_EXPOSE);
}

/*
 * @note 尝试连接服务器
 */
void cameracs::connect_server() {

}

/*
 * @note 尝试连接相机
 */
bool cameracs::connect_camera() {
	switch(param_->camType) {
	case 1: // Andor
	{
		boost::shared_ptr<CameraAndorCCD> camera = boost::make_shared<CameraAndorCCD>();
		camPtr_ = boost::static_pointer_cast<CameraBase>(camera);
	}
		break;
	case 2: // FLI
	{
		boost::shared_ptr<CameraFLICCD> camera = boost::make_shared<CameraFLICCD>();
		camPtr_ = boost::static_pointer_cast<CameraBase>(camera);
	}
		break;
	case 3: // Apogee
	{
		boost::shared_ptr<CameraApogee> camera = boost::make_shared<CameraApogee>();
		camPtr_ = boost::static_pointer_cast<CameraBase>(camera);
	}
		break;
	case 4: // PI
	{
		boost::shared_ptr<CameraPICCD> camera = boost::make_shared<CameraPICCD>();
		camPtr_ = boost::static_pointer_cast<CameraBase>(camera);
	}
		break;
	case 5: // GWAC-GY
	{
		boost::shared_ptr<CameraGYCCD> camera = boost::make_shared<CameraGYCCD>(param_->camIP.c_str());
		camPtr_ = boost::static_pointer_cast<CameraBase>(camera);
	}
		break;
	default:
		_gLog.Write(LOG_FAULT, NULL, "unknown camera type<%d>", param_->camType);
		return false;
	}

	CameraBase::InfoPtr nfcam = camPtr_->GetInformation();
	if (!camPtr_->Connect()) {
		_gLog.Write(LOG_FAULT, NULL, "Fail to connect camera. %s", nfcam->errmsg.c_str());
	}
	else {
		const CameraBase::ExpProcSlot &slot = boost::bind(&cameracs::expose_process, this, _1, _2, _3);
		camPtr_->RegisterExposeProcess(slot);
		camPtr_->SetReadPort(param_->readport);
		camPtr_->SetReadRate(param_->readrate);
		camPtr_->SetGain(param_->gain);
		camPtr_->SetCooler(param_->coolerset);
	}

	return (camPtr_->IsConnected());
}

/*
 * @note 启动守时服务
 */
void cameracs::start_ntp() {

}

/*
 * @brief 启动文件上传服务
 */
void cameracs::start_fs() {

}

/*
 * @note 线程: 重连服务器
 */
void cameracs::thread_re_network() {

}

/*
 * @note 线程: 重连相机
 */
void cameracs::thread_re_camera() {

}

/*
 * @note 线程: 发送相机定时信息
 */
void cameracs::thread_period_info() {

}

/*
 * @note 线程: 曝光前延时等待
 */
void cameracs::thread_wait_expose() {

}

/*
 * @note 线程: 清理磁盘空间
 */
void cameracs::thread_free_disk() {

}

/*
 * @note 消息: 响应网络连接结果
 */
void cameracs::on_connect_gc(long ec, long par2) {
}

/*
 * @note 消息: 响应收到的网络信息
 */
void cameracs::on_receive_gc(long par1, long par2) {

}

/*
 * @note 消息: 响应远程主机断开网络
 */
void cameracs::on_close_gc(long par1, long par2) {
	// 启动网络重连
}

/*
 * @note 消息: 响应相机连接结果
 */
void cameracs::on_connect_camera(long par1, long par2) {

}

/*
 * @note 消息: 曝光前准备
 */
void cameracs::on_prepare_expose(long par1, long par2) {

}

/*
 * @note 消息: 曝光过程
 */
void cameracs::on_process_expose(long par1, long par2) {

}

/*
 * @note 消息: 完成单帧曝光流程
 */
void cameracs::on_complete_expose(long par1, long par2) {

}

/*
 * @note 消息: 中止曝光
 */
void cameracs::on_abort_expose(long par1, long par2) {

}

/*
 * @note 消息: 曝光失败
 */
void cameracs::on_fail_expose(long par1, long par2) {

}

/*
 * @note 消息: 曝光延时等待
 */
void cameracs::on_complete_wait(long par1, long par2) {

}
