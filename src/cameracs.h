/*
 * @file cameracs.h 控制程序入口声明文件
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
 * @version 0.6
 * @date 2017年6月8日
 * - 更新相机工作状态
 * - 更新相机控制指令
 *
 * @date 2017年11月1日
 * - 更新相机工作状态, 增加异常处理
 *
 * @version 0.7
 * @date 2017年11月9日
 * - 删除互斥锁mtxsys_和mtxTcp_, 由消息队列实现互斥
 * - 在OnCompleteWait中启动曝光
 *
 * @date 2017年11月??日
 * - fits文件首先存储在内存虚拟硬盘中, 完成图像显示、网络传输和硬盘存储后删除
 * - 更改文件上传接口
 * - 增加磁盘剩余空间监测与磁盘清理
 *
 * @date 2017年11月??日
 * - 加入相机重连机制, 需要在与相机操作和访问前查看相机连接状态
 * - 重连机制:
 *   (1) 当相机出错时, 断开与相机连接
 *   (2) 间隔周期1、2、3、4、5分钟, 尝试重连相机
 *   (3) 连续5次重连失败, 退出程序
 *
 * @version 0.8
 * @date 2017年12月5日
 * - 基于优化后框架, 重新设计相机控制软件, 版本确定为0.8
 */

#ifndef CAMERACS_H_
#define CAMERACS_H_

#include "MessageQueue.h"
#include "tcpasio.h"
#include "parameter.h"
#include "CameraBase.h"

class cameracs : public MessageQueue {
public:
	cameracs(boost::asio::io_service* ios);
	virtual ~cameracs();

protected:
	/* 局部数据类型声明 */
	enum {// 定义消息
		MSG_CONNECT_GC = MSG_USER,	//< 消息: 与总控服务器网络连接结果
		MSG_RECEIVE_GC,		//< 消息: 处理来自总控服务器信息
		MSG_CLOSE_GC,		//< 消息: 总控服务器断开网络连接
		MSG_CONNECT_CAMERA,	//< 消息: 连接相机成功
		MSG_PREPARE_EXPOSE,	//< 消息: 准备开始曝光
		MSG_PROCESS_EXPOSE,	//< 消息: 曝光中
		MSG_COMPLETE_EXPOSE,//< 消息: 曝光正常结束
		MSG_ABORT_EXPOSE,	//< 消息: 完成物理曝光终止过程
		MSG_FAIL_EXPOSE,	//< 消息: 曝光过程中错误
		MSG_COMPLETE_WAIT,	//< 消息: 等待线程正常结束
		MSG_CCS_END
	};
	typedef boost::shared_array<char> charray;	//< 字符型数组

protected:
	/* 局部成员变量 */
	boost::asio::io_service* ios_;	//< 主io_service服务, 用于内部终止程序
	boost::shared_ptr<param_config> param_;		//< 程序配置参数
	TcpCPtr tcpCli_;			// 网络连接
	charray bufrcv_;			// 网络信息缓冲区
	threadptr thrdNetwork_;		// 线程: 重连网络
	threadptr thrdCamera_;		// 线程: 重连相机
	threadptr thrdPeriod_;		// 线程: 周期信息
	threadptr thrdWaitExpose_;	// 线程: 曝光前延时等待
	threadptr thrdFreeDisk_;	// 线程: 清理磁盘

	boost::shared_ptr<CameraBase> camPtr_;	//< 相机控制接口

public:
	/*!
	 * @brief 启动相关服务
	 * @return
	 * 服务启动结果
	 */
	bool Start();
	/*!
	 * @brief 中止所有服务
	 */
	void Stop();

protected:
	/*!
	 * @brief 回调函数: 网络
	 * @param client TCPClient指针
	 * @param ec     错误代码. 0: 正确
	 */
	void network_connect(const long client, const long ec);
	void network_receive(const long client, const long ec);
	/*!
	 * @brief 回调函数: 曝光
	 * @param state   状态
	 * @param left    曝光剩余时间
	 * @param percent 曝光百分比
	 */
	void expose_process(int state, double left, double percent);

protected:
	/*!
	 * @brief 连接服务器
	 */
	void connect_server();
	/*!
	 * @brief 连接相机
	 * @return
	 * 相机连接结果
	 */
	bool connect_camera();
	/*!
	 * @brief 启动NTP守时服务
	 */
	void start_ntp();
	/*!
	 * @brief 启动文件上传服务
	 */
	void start_fs();

protected:
	/*!
	 * @brief 线程: 重连服务器
	 */
	void thread_re_network();
	/*!
	 * @brief 线程: 重连相机
	 */
	void thread_re_camera();
	/*!
	 * @brief 线程: 发送周期状态
	 */
	void thread_period_info();
	/*!
	 * @brief 线程: 曝光前延时等待
	 */
	void thread_wait_expose();
	/*!
	 * @brief 线程: 清理磁盘空间
	 */
	void thread_free_disk();

protected:
	/*!
	 * @brief 注册消息响应函数
	 */
	void register_messages();
	/*!
	 * @brief 网络消息函数
	 */
	void on_connect_gc(long ec, long par2);
	void on_receive_gc(long par1, long par2);
	void on_close_gc(long par1, long par2);
	/*!
	 * @brief 相机连接结果
	 */
	void on_connect_camera(long par1, long par2);
	/*!
	 * @brief 曝光流程消息函数
	 */
	void on_prepare_expose(long par1, long par2);
	void on_process_expose(long par1, long par2);
	void on_complete_expose(long par1, long par2);
	void on_abort_expose(long par1, long par2);
	void on_fail_expose(long par1, long par2);
	void on_complete_wait(long par1, long par2);
};

#endif /* CAMERACS_H_ */
