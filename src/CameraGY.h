/*
 * @file CameraGY.h GWAC定制相机(重庆港宇公司研发电控系统)控制接口声明文件
 * ============================================================================
 * @date May 09, 2017
 * @author Xiaomeng Lu
 * @version 0.1
 * @note
 * 环境配置(主要是GenICam SDK和Runtime)
 *
 * @note
 * - 由于读出等问题, 抛弃原基于JAISDK的x86平台, 重新基于GenICam开发接口, 封装为类CameraGY
 *
 * @note
 * 问题:
 * - socket采用阻塞式函数(send_to, receive_from), 当相机不可访问时不会收到相机反馈,
 *   程序卡死. 采用异步函数和延时判断(500ms)机制, 判定相机的可访问性
 * - 未实现增益与快门寄存器读取功能. 待实现
 * - 未实现0xF0F00830地址写入功能. 不再使用
 * ============================================================================
 * @date Dec 03, 2018
 * @author Xiaomeng Lu
 * @version 0.2
 * @note
 * @li UDP数据包最大长度由MTU决定, 其中包括:
 * - IP包头     : 20
 * - UDP包头    :  8
 * - 数据长度   :  4
 * - 有效数据   : MTU - 32 ??
 */

#ifndef SRC_CAMERAGY_H_
#define SRC_CAMERAGY_H_

#include "CameraBase.h"
#include "udpasio.h"

class CameraGY: public CameraBase {
public:
	CameraGY(string const camIP);
	virtual ~CameraGY();

protected:
	/* 成员变量 */
	const uint16_t portLocal_;	//< 本地UDP服务端口
	const uint16_t portCamera_;	//< 相机UDP服务端口
	const uint8_t  idLeader_;	//< 数据包标志: 引导
	const uint8_t  idTrailer_;	//< 数据包标志: 结尾
	const uint8_t  idPayload_;	//< 数据包标志: 数据
	const uint32_t headsize_;	//< GWAC相机出厂定义数据包头长度

	boost::mutex mtx_reg_;	//< 互斥锁: 寄存器

	string camIP_;		//< 相机IP地址
	uint32_t expdur_;	//< 曝光时间, 量纲: 微秒
	uint32_t shtrmode_;	//< 快门模式. 0: Normal; 1: AlwaysOpen; 2: AlwaysClose
	uint32_t gain_;		//< 增益. 0: 1x; 1: 1.5x; 2: 2x. x: e-/DU

	/* 定义: 图像数据包 */
	boost::condition_variable cv_imgrdy_;	//< 事件: 完成图像据读出
	int64_t		tmdata_;	//< 时间戳: 接收图形数据包
	/*！
	 * 图像数据包定义1(相机发送数据包):
	 * - 相机通过UDP连接(udpdata_)将数字化的图像数据发送给控制机
	 * - 图像数据有效长度为byteimg_, 即width*height*2
	 * - 图像数据被分为多个数据包, 每个数据包由包头和包数据组成
	 * - 数据包头包含: 20字节IP头+8字节UDP头+headsize_(==8, 自定义)字节头
	 * - 最后一个数据包多出64字节校验信息
	 * - 数据包长度为packsize_ = MTU - 20 - 8 - 8
	 * - UDP数据传输中可能出现的问题: 丢包、错位、阻塞
	 * 图像数据包定义2(程序缓存数据包):
	 * - 图像数据被分为多个数据包, 缓存在packbuf_中
	 * - 每个数据包包含包头和包数据, 包头4字节、记录包数据长度
	 */
	uint32_t	byteimg_;	//< 图像数据长度, 量纲: 字节
	uint32_t	bytercd_;	//< 已接收图像数据长度, 量纲: 字节
	uint32_t	packcnt_;	//< 图像数据包数量, 对应图像数据长度
	uint32_t	packsize_;	//< 单个数据包长度, 量纲: 字节
	uint16_t	idFrame_;	//< 图像帧编号
	uint32_t	idPack_;	//< 数据包编号, 起始位置: 1
	boost::shared_array<uint8_t> packflag_;	//< 数据包接收标志

	/* 定义: 控制指令 */
	uint16_t msgcnt_;	//< 指令帧序列号
	UdpPtr udpcmd_;		//< UDP连接: 控制指令
	UdpPtr udpdata_;	//< UDP连接: 数据

	/* 线程 */
	threadptr thrdhb_;		//< 线程: 心跳机制
	threadptr thrdread_;	//< 线程: 读出图像数据
	boost::condition_variable cv_waitread_;	//< 事件: 等待完成图像数据读出

protected:
	/* 基类定义的虚函数 */
	/*!
	 * @brief 设置相机IP地址
	 * @return
	 * 操作结果
	 */
	bool UpdateIP(string const ip, string const mask, string const gw);
	/*!
	 * @brief 继承类实现与相机的真正连接
	 * @return
	 * 连接结果
	 */
	bool open_camera();
	/*!
	 * @brief 继承类实现真正与相机断开连接
	 */
	bool close_camera();
	/*!
	 * @brief 更新ROI区域
	 * @param xb  X轴合并因子
	 * @param yb  Y轴合并因子
	 * @param x   X轴起始位置, 相对原始图像起始位置
	 * @param y   Y轴起始位置, 相对原始图像起始位置
	 * @param w   ROI区宽度
	 * @param h   ROI区高度
	 */
	bool update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h);
	/*!
	 * @brief 改变制冷状态
	 */
	bool cooler_onoff(bool &onoff, float &coolset);
	/*!
	 * @brief 采集探测器芯片温度
	 */
	float sensor_temperature();
	/*!
	 * @brief 设置AD通道
	 */
	bool update_adchannel(uint16_t &index, uint16_t &bitpix);
	/*!
	 * @brief 设置读出端口
	 */
	bool update_readport(uint16_t &index, string &readport);
	/*!
	 * @brief 设置读出速度
	 */
	bool update_readrate(uint16_t &index, string &readrate);
	/*!
	 * @brief 设置行转移速度
	 */
	bool update_vsrate(uint16_t &index, float &vsrate);
	/*!
	 * @brief 设置增益
	 */
	bool update_gain(uint16_t &index, float &gain);
	/*!
	 * @brief 设置A/D基准偏压
	 */
	bool update_adoffset(uint16_t offset);
	/*!
	 * @brief 启动曝光
	 */
	bool start_expose(float duration, bool light);
	/*!
	 * @brief 中止曝光
	 */
	bool stop_expose();
	/*!
	 * @brief 检测曝光状态
	 */
	CAMERA_STATUS camera_state();
	/*!
	 * @brief 从相机读出图像数据到存储区
	 * @return
	 * 相机工作状态
	 * @note
	 * 返回值对应download_image()的结果:
	 * - CAMERA_IMGRDY: 读出成功
	 * - CAMERA_IDLE:   读出失败, 源于中止曝光
	 * - CAMERA_ERROR:  相机错误
	 */
	CAMERA_STATUS download_image();

protected:
	// 成员函数
	/*!
	 * @brief 查看与相机IP在同一网段的本机IP地址
	 * @return
	 * 主机字节排序方式的本机地址
	 * @note
	 * 返回值0表示无效
	 */
	uint32_t get_hostaddr();
	/*!
	 * @brief 计算帧头序号
	 * @return
	 * 帧序号
	 * @note
	 * 帧序号有效区间:[1, 65535], 逐一增加
	 */
	uint16_t msg_count();
	/*!
	 * @brief 更改寄存器对应地址数值
	 * @param addr 地址
	 * @param val  数值
	 * @note
	 * 操作失败抛出异常
	 */
	void reg_write(uint32_t addr, uint32_t val);
	/*!
	 * @brief 查看寄存器对应地址数值
	 * @param addr 地址
	 * @param val  数值
	 * @note
	 * 操作失败抛出异常
	 */
	void reg_read(uint32_t addr, uint32_t &val);
	/*!
	 * @brief 回调函数, 处理来自相机的数据信息
	 * @param udp UDP连接指针
	 * @param len 收到的数据长度, 量纲: 字节
	 */
	void receive_data(const long udp, const long len);
	/*!
	 * @brief 申请相机重传数据包
	 * @param iPack0 起始帧编号
	 * @param iPack1 截止帧编号
	 */
	void re_transmit(uint32_t iPack0, uint32_t iPack1);
	void re_transmit();
	/*!
	 * @brief 更新网络配置参数
	 * @param addr  地址
	 * @param vstr  新的网络地址/掩码/网关
	 * @return
	 * 操作结果
	 */
	bool update_network(const uint32_t addr, const char *vstr);
	/*!
	 * @brief 线程: 与相机之间的心跳机制
	 */
	void thread_heartbeat();
	/*!
	 * @brief 线程: 监测数据读出异常
	 */
	void thread_readout();
};

#endif /* SRC_CAMERAGY_H_ */
