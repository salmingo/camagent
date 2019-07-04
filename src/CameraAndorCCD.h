/*!
 * CameraAndorCCD.h Andor CCD相机的声明文件
 * @date 2015-09-22
 * @version 0.1
 * @author 卢晓猛

 * @date 2019-07-01
 * @version 0.2
 * @author 卢晓猛
 * @note
 * - 基于Andor SDK, 封装对Andor CCD相机的操作
 * - 第一次连接相机时, 采集所有读出参数并写入配置文件
 * - 连接相机成功后, 加载配置文件, 并从中查找读出参数档位对应的数值
 */

#ifndef SRC_CAMERAANDORCCD_H_
#define SRC_CAMERAANDORCCD_H_

#include <atmcdLXd.h>
#include <boost/property_tree/ptree.hpp>
#include "CameraBase.h"

class CameraAndorCCD: public CameraBase {
public:
	CameraAndorCCD();
	virtual ~CameraAndorCCD();

protected:
	// 成员变量
	boost::property_tree::ptree xmlpt_;	//< 相机参数访问接口. 相机参数存入XML配置文件中
	int shtropening_;	//< 快门打开时间, 量纲: 毫秒
	int shtrclosing_;	//< 快门关闭时间, 量纲: 毫秒

public:
	/*!
	 * @brief 访问相机配置参数表
	 * @return
	 * 以XML树格式存储的相机配置参数
	 */
	boost::property_tree::ptree &GetParameters();
	/*!
	 * @brief 改变EM增益
	 * @param gain 增益
	 * @return
	 * 成功标志
	 */
	bool UpdateEMGain(uint16_t gain);

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
	bool update_adoffset(uint16_t value);
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
	/* Ancod CCD 相机工作参数 */
	/*!
	 * @brief 从文件中加载相机参数
	 */
	bool load_parameters();
	/*!
	 * @brief 从相机ROM中读取相机参数并写入文件
	 */
	void init_parameters();
};

#endif /* SRC_CAMERAANDORCCD_H_ */
