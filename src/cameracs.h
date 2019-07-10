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
#include "CameraBase.h"
#include "NTPClient.h"
#include "tcpasio.h"
#include "udpasio.h"
#include "FlatField_Sky.h"

typedef boost::shared_ptr<ConfigParameter> ParamPtr;
typedef boost::shared_ptr<CDs9> CDs9Ptr;

class cameracs : public MessageQueue {
public:
	cameracs(boost::asio::io_service* ios);
	virtual ~cameracs();

protected:
	enum {// 消息
		MSG_RECEIVE_GC = MSG_USER,		//< 收到来自总控服务器的信息
		MSG_CLOSE_GC,		//< 总控服务器断开网络连接
		MSG_CONNECT_GC,		//< 已建立与总控服务器的连接
		MSG_LAST
	};

protected:
	/*---- 成员变量 ----*/
	/* 操作对象访问接口 */
	ParamPtr param_;	//< 配置参数
	TcpCPtr gtoaes_;	//< 总控服务器访问指针
	CameraBasePtr camera_;	//< 相机访问指针
	FilterCtrlPtr filter_;	//< 滤光片访问指针
	NTPCliPtr ntp_;			//< NTP时钟同步
	CDs9Ptr ds9_;			//< ds9访问指针
	FlatField_Sky flatsky_;	//< 天光平场控制参数
	UdpPtr cool_alone_;		//< 单独的温控接口
	//...缺文件服务器, 网络信息解析/封装接口

	boost::shared_array<char> bufrcv_;	//< 网络信息存储区: 消息队列中调用

	/* 线程 */
	threadptr thrd_state_;	//< 向总控服务器发送相机工作状态
	threadptr thrd_noon_;	//< 每日正午执行的一些诊断操作: 检查/清理磁盘空间
	threadptr thrd_reconn_gtoaes_;	//< 重新连接总控服务器
	threadptr thrd_cool_;	//< 定时查询独立温控器的制冷温度

	/* 事件: 条件变量 */
	boost::condition_variable cv_camstate_changed_;		//< 相机工作状态发生变化

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
	 * @brief 建立与总控服务器的连接
	 */
	bool connect_server_gtoaes();
	/*!
	 * @brief 建立与文件服务器的连接
	 */
	bool connect_server_file();
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
	/*!
	 * @brief 回调函数: 处理来自gtoaes服务器的信息
	 * @param addr TCPClient对象地址
	 * @param ec   错误代码. 0: 接收到有效信息; !=0: 服务器断开连接
	 */
	void receive_gtoaes(const long, const long ec);
	/*!
	 * @brief 回调函数: 处理异步连接gtoaes服务器的结果
	 * @param TCPClient对象地址
	 * @param ec   错误代码. 0: 连接成功; !=0: 失败
	 */
	void connect_gtoaes(const long addr, const long ec);
	/*!
	 * @brief 回调函数: 处理来自独立温控的反馈信息
	 */
	void receive_alone_cooler(const long, const long);

/////////////////////////////////////////////////////////////////////////////
protected:
	/* 消息机制 */
	/*!
	 * @brief 注册消息及其处理函数
	 */
	void register_message();
	/*!
	 * @brief 处理来自总控服务器的信息
	 */
	void on_receive_gc(const long addr = 0, const long ec = 0);
	/*!
	 * @brief 总控服务器断开连接
	 */
	void on_close_gc(const long addr = 0, const long ec = 0);
	/*!
	 * @brief 成功连接总控服务器
	 * @note
	 * 该消息处理: 在工作过程中, 与服务器网络连接异常断开后的重连
	 */
	void on_connect_gc(const long addr = 0, const long ec = 0);

/////////////////////////////////////////////////////////////////////////////
protected:
	/* 多线程, 执行并行工作逻辑 */
	/*!
	 * @brief 定时向总控服务器发送系统工作状态
	 * @note
	 * 发送时间:
	 * - 当相机工作状态发生变化时
	 * - 周期: 10秒
	 */
	void thread_state();
	/*!
	 * @brief 每日正午执行诊断/清理操作
	 */
	void thread_noon();
	/*!
	 * @brief 重新连接总控服务器
	 * @note
	 * 周期: 2分钟
	 */
	void thread_reconn_gtoaes();
	/*!
	 * @brief 独立温控器时定期查询探测器温度
	 */
	void thread_cooler();

/////////////////////////////////////////////////////////////////////////////
protected:
	/*!
	 * @brief 清理本地磁盘空间
	 */
	void free_local_storage();
};
#endif /* SRC_CAMERACS_H_ */
