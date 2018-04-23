/*
 * @file cameracs.h 相机控制软件声明文件
 * @version 1.0
 * @author 卢晓猛
 * @email lxm@nao.cas.cn
 * @note
 */

#ifndef SRC_CAMERACS_H_
#define SRC_CAMERACS_H_

#include "MessageQueue.h"

class cameracs : public MessageQueue {
public:
	cameracs(boost::asio::io_service* ios);
	virtual ~cameracs();

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

protected:
	/* 成员变量 */
	boost::asio::io_service* ios_;	//< IO服务
};

#endif /* SRC_CAMERACS_H_ */
