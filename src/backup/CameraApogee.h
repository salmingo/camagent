/*
 * @file CameraApogee.h Apogee Alta CCD相机控制接口
 * @version 0.2
 * @date Apr 9, 2017
 * @author Xiaomeng Lu
 *
 * @version 0.3
 * @date Decr 4, 2018
 * @author Xiaomeng Lu
 */

#ifndef SRC_CAMERAAPOGEE_H_
#define SRC_CAMERAAPOGEE_H_

#include <apogee/Alta.h>
#include <vector>
#include "CameraBase.h"

class CameraApogee: public CameraBase {
public:
	CameraApogee();
	virtual ~CameraApogee();

protected:
	/* 成员变量 */
	boost::shared_ptr<Alta> altacam_;	//< 相机SDK接口
	std::vector<uint16_t> data_; 	//< 存储从相机读出的数据

protected:
	/* 基类定义的虚函数 */
	/*!
	 * @brief 设置相机IP地址
	 * @return
	 * 操作结果
	 */
	bool SetIP(string const ip, string const mask, string const gw);
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
};

#endif /* SRC_CAMERAAPOGEE_H_ */
