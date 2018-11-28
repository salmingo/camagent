/*
 * @file cameracs.h 相机控制软件定义文件, 定义天文相机工作流程
 * @version 1.0
 * @author 卢晓猛
 * @email lxm@nao.cas.cn
 * @note
 * cameracs: Camera Control System Interface
 */

#ifndef SRC_CAMERACS_H_
#define SRC_CAMERACS_H_

#include <boost/asio.hpp>
#include "MessageQueue.h"
#include "VersionMonitor.h"
#include "AsciiProtocol.h"
#include "NTPClient.h"
#include "tcpasio.h"
#include "FilterCtrl.h"
#include "CameraBase.h"

class cameracs : public MessageQueue {
public:
	cameracs(boost::asio::io_service* ios);
	virtual ~cameracs();

protected:
	/* 定义消息 */
	enum MSG_CCS {
		MSG_CONNECT_GENERAL_CONTROL = MSG_USER,	//< 与调度服务器的网络连接结果
		MSG_RECEIVE_GENERAL_CONTROL,	//< 收到调度服务器信息
		MSG_CLOSE_GENERAL_CONTROL,		//< 调度服务器断开网络连接
		MSG_CONNECT_CAMERA,		//< 与相机的连接结果
		MSG_PREPARE_EXPOSE,		//< 完成曝光前准备
		MSG_PROCESS_EXPOSE,		//< 监视曝光过程
		MSG_COMPLETE_EXPOSE,	//< 完成曝光流程
		MSG_ABORT_EXPOSE,		//< 中止曝光过程
		MSG_FAIL_EXPOSE,		//< 曝光过程中遇到错误, 曝光失败
		MSG_END_CCS
	};

//////////////////////////////////////////////////////////////////////////////
public:
	/*!
	 * @brief 启动服务
	 * @return
	 * 服务启动结果
	 */
	bool StartService();
	/*!
	 * @brief 终止服务
	 */
	void StopService();
	/*!
	 * @brief 监测是否有版本更新请求
	 */
	void MonitorVersion();

//////////////////////////////////////////////////////////////////////////////
protected:
	/*!
	 * @brief 注册消息响应函数
	 */
	void register_messages();
	void on_connect_general_control(const long, const long);
	void on_receive_general_control(const long, const long);
	void on_close_general_control(const long, const long);
	void on_connect_camera(const long, const long);
	void on_prepare_expose(const long, const long);
	void on_process_expose(const long, const long);
	void on_complete_expose(const long, const long);
	void on_abort_expose(const long, const long);
	void on_fail_expose(const long, const long);

//////////////////////////////////////////////////////////////////////////////
protected:
	/* 成员变量 */
	boost::asio::io_service* ios_;	//< IO服务
	boost::shared_ptr<VersionMonitor> vermon_;	//< 版本更新监视

	boost::shared_ptr<FilterCtrl> filter_;	//< 滤光片控制接口
};

#endif /* SRC_CAMERACS_H_ */
