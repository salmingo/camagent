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

#include "MessageQueue.h"
#include "parameter.h"
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
		MSG_CONNECT_GTOAES = MSG_USER,	//< 与调度服务器的网络连接结果
		MSG_RECEIVE_GTOAES,		//< 收到调度服务器信息
		MSG_CLOSE_GTOAES,		//< 调度服务器断开网络连接
		MSG_CONNECT_CAMERA,		//< 与相机的连接结果
		MSG_CONNECT_FILTER,		//< 与滤光片控制器的连接结果
		MSG_PREPARE_EXPOSE,		//< 完成曝光前准备
		MSG_PROCESS_EXPOSE,		//< 监视曝光过程
		MSG_COMPLETE_EXPOSE,	//< 完成曝光流程
		MSG_ABORT_EXPOSE,		//< 中止曝光过程
		MSG_FAIL_EXPOSE			//< 曝光过程中遇到错误, 曝光失败
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
	 * @brief 尝试连接相机
	 * @return
	 * 相机连接结果
	 */
	bool connect_camera();
	/*!
	 * @brief 尝试连接滤光片控制器
	 * @return
	 * 滤光片控制器连接结果
	 */
	bool connect_filter();
	/*!
	 * @brief 尝试连接总控服务器
	 */
	void connect_gtoaes();
	/*!
	 * @brief 处理与总控服务器的连接结果
	 * @param client 网络访问接口
	 * @param ec     错误代码
	 */
	void handle_connect_gtoaes(const long client, const long ec);
	/*!
	 * @brief 处理总控服务器发送的指令或信息
	 * @param client 网络访问接口
	 * @param ec     错误代码
	 */
	void handle_receive_gtoaes(const long client, const long ec);

//////////////////////////////////////////////////////////////////////////////
protected:
	/*!
	 * @brief 线程: 尝试连接总控服务器
	 */
	void thread_tryconnect_gtoaes();
	/*!
	 * @brief 线程: 尝试连接相机
	 */
	void thread_tryconnect_camera();
	/*!
	 * @brief 线程: 尝试连接滤光片控制器
	 */
	void thread_tryconnect_filter();

//////////////////////////////////////////////////////////////////////////////
protected:
	/*!
	 * @brief 注册消息响应函数
	 */
	void register_messages();
	/*!
	 * @brief 处理与总控服务器的连接结果
	 * @param ec     错误代码
	 * @note
	 * - ec == 0表示成功
	 */
	void on_connect_gtoaes(const long ec, const long);
	/*!
	 * @brief 处理收到的服务器指令或消息
	 */
	void on_receive_gtoaes(const long, const long);
	/*!
	 * @brief 断开与总控服务器的连接
	 */
	void on_close_gtoaes(const long, const long);
	/*!
	 * @brief 处理与相机的连接结果
	 * @note
	 * - 当与相机重新连接成功后触发该消息
	 */
	void on_connect_camera(const long, const long);
	/*!
	 * @brief 处理与滤光片控制器的连接结果
	 * @note
	 * - 当与滤光片控制器重新连接成功后触发该消息
	 */
	void on_connect_filter(const long, const long);
	void on_prepare_expose(const long, const long);
	void on_process_expose(const long, const long);
	void on_complete_expose(const long, const long);
	void on_abort_expose(const long, const long);
	void on_fail_expose(const long, const long);

//////////////////////////////////////////////////////////////////////////////
protected:
	/* 成员变量 */
//////////////////////////////////////////////////////////////////////////////
	boost::asio::io_service* ios_;	//< IO服务
	boost::shared_ptr<VersionMonitor> vermon_;	//< 版本更新监视
	boost::shared_ptr<Parameter> param_;	//< 配置参数
	boost::shared_ptr<FilterCtrl> filter_;	//< 滤光片控制接口
	NTPPtr ntp_;		//< NTP服务器控制接口
	cambase camera_;	//< 相机控制接口
	EXPOSE_COMMAND cmdexp_;	//< 曝光指令
	apcam    nfcam_;	//< 相机工作状态
	apobject nfobj_;	//< 观测目标信息
//////////////////////////////////////////////////////////////////////////////
	TcpCPtr tcptr_;		//< 网络连接接口
	AscProtoPtr ascproto_;	//< 通信协议接口
//////////////////////////////////////////////////////////////////////////////
	threadptr thrd_gtoaes_;	//< 线程: 尝试连接总控服务器
	threadptr thrd_camera_;	//< 线程: 尝试连接相机
	threadptr thrd_filter_;	//< 线程: 尝试连接滤光片控制器
};

#endif /* SRC_CAMERACS_H_ */
