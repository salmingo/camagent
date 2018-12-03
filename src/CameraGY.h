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
	const uint16_t portCamera_;	//< 相机UDP服务端口
	const uint16_t portLocal_;	//< 本地UDP服务端口
	const uint8_t  idLeader_;	//< 数据包标志: 引导
	const uint8_t  idTrailer_;	//< 数据包标志: 结尾
	const uint8_t  idPayload_;	//< 数据包标志: 数据

	boost::mutex mtx_reg_;	//< 互斥锁: 寄存器

	string camIP_;		//< 相机IP地址
	UdpPtr udpcmd_;		//< UDP连接: 控制指令
	UdpPtr udpdata_;	//< UDP连接: 数据

	/* 定义: 控制指令 */
	uint16_t msgcnt_;	//< 指令帧序列号

protected:
	/* 基类定义的虚函数 */
	/*!
	 * @brief 设置相机IP地址
	 */
	void SetIP(string const ip, string const mask, string const gw);
	/*!
	 * @brief 软重启相机
	 * @return
	 * 相机重启结果
	 */
	bool SoftwareReboot();
	/*!
	 * @brief 继承类实现与相机的真正连接
	 * @return
	 * 连接结果
	 */
	bool open_camera();
	/*!
	 * @brief 继承类实现真正与相机断开连接
	 */
	void close_camera();
	/*!
	 * @brief 更新ROI区域
	 * @param xb  X轴合并因子
	 * @param yb  Y轴合并因子
	 * @param x   X轴起始位置, 相对原始图像起始位置
	 * @param y   Y轴起始位置, 相对原始图像起始位置
	 * @param w   ROI区宽度
	 * @param h   ROI区高度
	 */
	void update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h);
	/*!
	 * @brief 改变制冷状态
	 */
	void cooler_onoff(float &coolset, bool &onoff);
	/*!
	 * @brief 采集探测器芯片温度
	 */
	float sensor_temperature();
	/*!
	 * @brief 设置AD通道
	 */
	void update_adchannel(uint16_t &index, uint16_t &bitpix);
	/*!
	 * @brief 设置读出端口
	 */
	void update_readport(uint16_t &index, string &readport);
	/*!
	 * @brief 设置读出速度
	 */
	void update_readrate(uint16_t &index, string &readrate);
	/*!
	 * @brief 设置行转移速度
	 */
	void update_vsrate(uint16_t &index, float &vsrate);
	/*!
	 * @brief 设置增益
	 */
	void update_gain(uint16_t &index, float &gain);
	/*!
	 * @brief 设置A/D基准偏压
	 */
	void update_adoffset(uint16_t &index);
	/*!
	 * @brief 启动曝光
	 */
	bool start_expose(float duration, bool light);
	/*!
	 * @brief 中止曝光
	 */
	void stop_expose();
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
};

#endif /* SRC_CAMERAGY_H_ */
