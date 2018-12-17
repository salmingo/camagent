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

	/*!
	 * @struct FilePath 定义FITS文件路径及文件名
	 */
	struct FilePath {
		bool   newseq;		//< 是否新的观测序列
		string subpath;		//< 在文件根路径下的目录名
		string pathname;	//< 包含根路径的目录名
		string filename;	//< 文件名
		string filepath;	//< 文件全路径名
	};

//////////////////////////////////////////////////////////////////////////////
protected:
	/* 成员变量 */
//////////////////////////////////////////////////////////////////////////////
	OBSS_TYPE ostype_;	//< 观测系统类型
	double lgt_;	//< 地理经度, 量纲: 角度, 东经为正
	double lat_;	//< 地理纬度, 量纲: 角度, 北纬为正
	double alt_;	//< 海拔, 量纲: 米
	boost::asio::io_service* ios_;	//< IO服务
	boost::shared_ptr<VersionMonitor> vermon_;	//< 版本更新监视
	boost::shared_ptr<Parameter> param_;		//< 配置参数
	boost::shared_ptr<FilterCtrl> filterctl_;	//< 滤光片控制接口
	cambase camctl_;		//< 相机控制接口
	EXPOSE_COMMAND cmdexp_;	//< 曝光指令
	boost::condition_variable cv_statchanged_;	//< 事件: 工作状态发生变化
	NTPPtr   ntp_;		//< NTP服务器控制接口
	apcam    nfcam_;	//< 相机工作状态
	apobject nfobj_;	//< 观测目标及曝光参数
	FilePath filepath_;	//< FITS文件路径名
	vector<string> filters_;	//< 曝光参数中的滤光片集合
//////////////////////////////////////////////////////////////////////////////
	TcpCPtr tcptr_;		//< 网络连接接口
	AscProtoPtr ascproto_;	//< 通信协议接口
	boost::shared_array<char> bufrcv_;	//< 网络信息接收缓存区
//////////////////////////////////////////////////////////////////////////////
	threadptr thrd_gtoaes_;		//< 线程: 尝试连接总控服务器
	threadptr thrd_camera_;		//< 线程: 尝试连接相机
	threadptr thrd_filter_;		//< 线程: 尝试连接滤光片控制器
	threadptr thrd_upload_;		//< 线程: 向gtoaes服务器上传工作状态
	threadptr thrd_noon_;		//< 线程: 每日正午检查/清理

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
	/*!
	 * @brief 从观测计划中解析滤光片集合
	 */
	void resolve_filter();
	/*!
	 * @brief 检查曝光序列是否结束
	 * @return
	 * 如果结束则返回true, 否则返回false继续曝光
	 */
	bool exposure_over();
	/*!
	 * @brief 回调函数, 处理曝光进度
	 * @param 剩余曝光时间
	 * @param 曝光进度, 百分比
	 * @param 曝光状态
	 */
	void expose_process(const double left, const double percent, const int state);
	/*!
	 * @brief 处理服务器发送的通信协议
	 * @param proto 通信协议
	 */
	void process_protocol(apbase proto);
	/*!
	 * @brief 控制改变曝光状态
	 * @param cmd 曝光指令
	 */
	void command_expose(EXPOSE_COMMAND cmd);

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
	/*!
	 * @brief 线程: 向gtoaes服务器上传工作状态信息
	 */
	void thread_upload();
	/*!
	 * @brief 线程: 每日正午执行的检查与清理操作
	 * @note
	 * - 检查清理磁盘空间
	 * - 重新连接滤光片控制器
	 */
	void thread_noon();
	/*!
	 * @brief 线程: 定时检查相机温度
	 */
	void thread_cycle();
	/*!
	 * @brief 检查磁盘可用空间并清理磁盘
	 */
	void free_disk();
	/*!
	 * @brief 计算当前时间距离下一个中午的秒数
	 */
	long next_noon();

//////////////////////////////////////////////////////////////////////////////
/* 存储FITS文件 */
protected:
	/*!
	 * @brief 创建FITS文件路径和文件名
	 */
	void create_filepath();
	/*!
	 * @brief 保存FITS文件
	 */
	void save_fitsfile();

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
};

#endif /* SRC_CAMERACS_H_ */
