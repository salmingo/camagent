/*!
 * @file CameraFLICCD.h FLI CCD声明文件
 * @version 0.2
 * @date 2019-07-04
 * @author Xiaomeng Lu
 */

#ifndef SRC_CAMERAFLICCD_H_
#define SRC_CAMERAFLICCD_H_

#include <libfli.h>
#include "CameraBase.h"

class CameraFLICCD: public CameraBase {
public:
	CameraFLICCD();
	virtual ~CameraFLICCD();

protected:
	flidev_t hcam_;	// 设备句柄

protected:
	/* 基类定义的虚函数 */
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
	bool update_adoffset(uint16_t value);
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
	/*!
	 * @brief 加载相机参数
	 */
	bool load_parameters();
	/*!
	 * @brief 初始化相机参数
	 */
	bool init_parameters();
};

#endif /* SRC_CAMERAFLICCD_H_ */
