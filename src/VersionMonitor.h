/*
 * @file VersionMonitor.h 版本监视声明文件
 * @version 1.0
 * @author 卢晓猛
 * @email lxm@nao.cas.cn
 * @note
 * - 打开消息队列msgque_mhssvau
 * - 当检测到消息ID=128时, 回调函数结束程序
 */

#ifndef SRC_VERSIONMONITOR_H_
#define SRC_VERSIONMONITOR_H_

#include "MessageQueue.h"

class VersionMonitor: public MessageQueue {
public:
	VersionMonitor();
	virtual ~VersionMonitor();

public:
	// 数据类型
	// 声明VersionMonitor回调函数类型
	typedef boost::signals2::signal<void ()> CallbackFunc;
	// 基于boost::signals2声明插槽类型
	typedef CallbackFunc::slot_type CBSlot;

protected:
	enum MSG_VM {
		MSG_TERMINATE	// 终止程序
	};

public:
	/*!
	 * @brief 启动服务
	 * @return
	 * 服务启动结果
	 */
	bool Start();
	/*!
	 * @brief 终止服务
	 */
	void Stop();
	/*!
	 * @brief 注册退出回调函数, 终止程序
	 * @param slot 函数插槽
	 */
	void RegisterTerminate(const CBSlot& slot);

protected:
	/*!
	 * @brief 响应消息MSG_TERMINATE, 终止程序
	 */
	void on_terminate(const long p1, const long p2);

protected:
	/* 成员变量 */
	CallbackFunc cb_term_;	//< 回调函数
};

#endif /* SRC_VERSIONMONITOR_H_ */
