/*!
 * @file cameracs.h 相机控制软件定义文件, 定义天文相机工作流程
 * @version 1.0
 * @author 卢晓猛
 * @email lxm@nao.cas.cn
 * @note
 */

#ifndef SRC_CAMERACS_H_
#define SRC_CAMERACS_H_

#include "MessageQueue.h"
#include "CDs9.h"
#include "ConfigParam.h"
#include "FilterCtrl.h"
#include "NTPClient.h"
#include "tcpasio.h"

class cameracs : public MessageQueue {
public:
	cameracs(boost::asio::io_service* ios);
	virtual ~cameracs();

protected:

/////////////////////////////////////////////////////////////////////////////
public:
	// 接口函数
	/*!
	 * @brief 启动程序服务
	 */
	bool StartService();
	/*!
	 * @brief 停止程序服务
	 */
	void StopService();

/////////////////////////////////////////////////////////////////////////////
protected:
	/* 建立并维护与硬件设备的连接 */
	/*!
	 * @brief 建立与相机的连接
	 * @return
	 * 连接结果
	 */
	bool connect_camera();
	/*!
	 * @brief 建立与滤光片的连接
	 * @return
	 * 连接结果
	 */
	bool connect_filter();
	/*!
	 * @brief 断开与相机的连接
	 */
	void disconnect_camera();
	/*!
	 * @brief 断开与滤光片的连接
	 */
	void disconnect_filter();

/////////////////////////////////////////////////////////////////////////////
protected:
	/* 多线程, 执行并行工作逻辑 */
};
#endif /* SRC_CAMERACS_H_ */
