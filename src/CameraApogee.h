/*!
 * @file CameraApogee.h Apogee Alta CCD相机控制接口
 * @version 0.2
 * @date Apr 9, 2017
 * @author Xiaomeng Lu
 *
 * @version 0.3
 * @date July 3, 2019
 * @author Xiaomeng Lu
 * @note
 * - 支持Apogee Alta系列USB口和网口相机
 */

#ifndef SRC_CAMERAAPOGEE_H_
#define SRC_CAMERAAPOGEE_H_

#include <apogee/Alta.h>
#include <vector>
#include "CameraBase.h"

/////////////////////////////////////////////////////////////////////////////
std::vector<string> MakeTokens(const string &str, const string &separator);
string GetItemFromFindStr( const string & msg, const string & item );
string GetUsbAddress( const string & msg );
string GetEthernetAddress( const string & msg );
uint16_t GetID( const string & msg );
uint16_t GetFrmwrRev( const string & msg );
/////////////////////////////////////////////////////////////////////////////
class CameraApogee: public CameraBase {
public:
	CameraApogee();
	virtual ~CameraApogee();

protected:
	/*------- 成员变量 -------*/
	// 以下4项是连接相机时的地址参数
	string iotype_;		//< 相机接口类型
	string addr_;		//< 接口地址
	uint16_t frmwr_;	//< 固件版本
	uint16_t id_;		//< 相机编号

	boost::shared_ptr<Alta> altacam_;	//< 相机SDK接口
	std::vector<uint16_t> data_; 	//< 存储从相机读出的数据

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
	 * @brief 尝试搜索Apogee相机
	 * @return
	 */
	bool find_camera();
	/*!
	 * @brief 尝试搜索USB连接类型相机
	 * @return
	 * 搜索结果
	 */
	bool find_camera_usb(string &desc);
	/*!
	 * @brief 尝试在局域网内搜索相机
	 * @return
	 * 搜索结果
	 */
	bool find_camera_lan(string &desc);
	/*!
	 * @brief 初始化相机参数
	 */
	void init_parameters();
	/*!
	 * @brief 从配置文件中加载相机参数
	 */
	bool load_parameters();
};

#endif /* SRC_CAMERAAPOGEE_H_ */
