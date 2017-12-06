/*
 * CameraPICCD.h
 *
 *  Created on: Dec 6, 2017
 *      Author: lxm
 */

#ifndef SRC_CAMERAPICCD_H_
#define SRC_CAMERAPICCD_H_

#include "CameraBase.h"

class CameraPICCD: public CameraBase {
public:
	CameraPICCD();
	virtual ~CameraPICCD();

protected:
	/* 功能 */
	/*!
	 * @brief 执行相机连接操作
	 * @return
	 * 连接结果
	 */
	virtual bool connect();
	/*!
	 * @brief 执行相机断开操作
	 */
	virtual void disconnect();
	/*!
	 * @brief 启动相机曝光流程
	 * @param duration 积分时间, 量纲: 秒
	 * @param light    快门控制模式. true: 自动或常开; false: 常关
	 * @return
	 * 曝光流程启动结果
	 */
	virtual bool start_expose(double duration, bool light);
	/*!
	 * @brief 中止曝光流程
	 */
	virtual void stop_expose();
	/*!
	 * @brief 检查相机工作状态
	 * @return
	 * 相机工作状态
	 */
	virtual int check_state();
	/*!
	 * @brief 从控制器读出图像数据进入内存
	 * @return
	 * 相机工作状态
	 */
	virtual int readout_image();
	/*!
	 * @brief 采集探测器温度
	 * @return
	 * 探测器温度
	 */
	virtual double sensor_temperature();
	/*!
	 * @brief 执行相机制冷操作
	 * @param coolset 制冷温度
	 * @param onoff   制冷模式
	 */
	virtual void update_cooler(double &coolset, bool onoff);
	/*!
	 * @brief 设置读出端口
	 * @param index 档位
	 * @return
	 * 改变后档位
	 */
	virtual uint32_t update_readport(uint32_t index);
	/*!
	 * @brief 设置读出速度
	 * @param index 档位
	 * @return
	 * 改变后档位
	 */
	virtual uint32_t update_readrate(uint32_t index);
	/*!
	 * @brief 设置增益
	 * @param index 档位
	 * @return
	 * 改变后档位
	 */
	virtual uint32_t update_gain(uint32_t index);
	/*!
	 * @brief 设置ROI区域
	 * @param xstart X轴起始位置
	 * @param ystart Y轴起始位置
	 * @param width  宽度
	 * @param height 高度
	 * @param xbin   X轴合并因子
	 * @param ybin   Y轴合并因子
	 */
	virtual void update_roi(int &xstart, int &ystart, int &width, int &height, int &xbin, int &ybin);
	/*!
	 * @brief 设置偏置电压
	 * @param index 档位
	 */
	virtual void update_adcoffset(uint16_t offset);
};

#endif /* SRC_CAMERAPICCD_H_ */
