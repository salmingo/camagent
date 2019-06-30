/*!
 * @file CameraBase.h 相机基类定义文件, 定义相机统一访问接口
 * @version 1.0
 * @date 2018-04-26
 * @note
 * - 实现相机工作逻辑流程
 * - 定义真实相机需实现的纯虚函数
 */

#ifndef SRC_CAMERABASE_H_
#define SRC_CAMERABASE_H_

#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>

using std::string;
using namespace boost::posix_time;

class CameraBase {
/////////////////////////////////////////////////////////////////////////////
public:
	/* 声明数据类型 */
	enum CAMERA_STATUS {// 相机工作状态
		CAMERA_ERROR,	//< 错误
		CAMERA_IDLE,	//< 空闲
		CAMERA_EXPOSE,	//< 曝光过程中
		CAMERA_IMGRDY	//< 已完成曝光, 可以读出数据进入内存
	};

	/*!
	 * @brief 声明曝光进度插槽函数
	 * @param <1> 曝光剩余时间, 量纲: 秒
	 * @param <2> 曝光进度, 量纲: 百分比
	 * @param <3> 图像数据状态, CAMERA_STATUS
	 */
	typedef boost::signals2::signal<void (const double, const double, const int)> ExposeProcess;
	typedef ExposeProcess::slot_type ExpProcSlot;

/////////////////////////////////////////////////////////////////////////////
public:
	CameraBase();
	virtual ~CameraBase();

/////////////////////////////////////////////////////////////////////////////
protected:
	/* 成员变量 */
	ExposeProcess cbexp_;	//< 曝光进度插槽函数
/////////////////////////////////////////////////////////////////////////////
public:
	/*!
	 * @brief 尝试连接相机
	 * @return
	 * 相机连接结果
	 */
	bool Connect();
	/*!
	 * @brief 断开与相机的连接
	 */
	void DisConnect();
	/*!
	 * @brief 检查与相机的连接状态
	 * @return
	 * 与相机的连接状态
	 */
	bool IsConnected();
	/*!
	 * @brief 尝试启动曝光流程
	 * @param duration  曝光周期, 量纲: 秒
	 * @param light     是否需要外界光源
	 * @return
	 * 曝光启动结果
	 */
	bool Expose(float duration, bool light = true);
	/*!
	 * @brief 中止当前曝光过程
	 */
	void AbortExpose();
	/*!
	 * @brief 注册曝光进度回调函数
	 * @param slot 函数插槽
	 */
	void RegisterExposeProc(const ExpProcSlot &slot);
	/*!
	 * @brief 设置制冷温度
	 * @param onoff  启动/停止制冷功能
	 * @param set    制冷温度
	 * @return
	 * 成功标志
	 */
	bool SetCooler(bool onoff = false, float set = -40.0);
	/*!
	 * @brief 改变A/D转换通道
	 * @param index A/D通道索引
	 * @return
	 * 成功标志
	 */
	bool SetADChannel(uint16_t index);
	/*!
	 * @brief 改变读出端口
	 * @param index 读出端口索引
	 * @return
	 * 成功标志
	 */
	bool SetReadPort(uint16_t index);
	/*!
	 * @brief 改变读出速度
	 * @param index 读出速度索引
	 * @return
	 * 成功标志
	 */
	bool SetReadRate(uint16_t index);
	/*!
	 * @brief 设置行转移速度
	 * @param index 行转移速度索引
	 * @return
	 * 成功标志
	 */
	bool SetVSRate(uint16_t index);
	/*!
	 * @brief 改变增益
	 * @param index 增益索引
	 * @return
	 * 成功标志
	 */
	bool SetGain(uint16_t index);
	/*!
	 * @brief 改变基准偏压, 使得本底图像统计均值为offset
	 * @param offset 本底平均期望值
	 * @return
	 * 成功标志
	 */
	bool SetADCOffset(uint16_t offset);
	/*!
	 * @brief 改变ROI区
	 * @param xb  X轴合并因子
	 * @param yb  Y轴合并因子
	 * @param x   X轴起始位置, 相对原始图像起始位置
	 * @param y   Y轴起始位置, 相对原始图像起始位置
	 * @param w   ROI区宽度
	 * @param h   ROI区高度
	 */
	bool SetROI(int &xb, int &yb, int &x, int &y, int &w, int &h);
	/*!
	 * @brief 设置相机IP地址
	 * @return
	 * 操作结果
	 */
	virtual bool SetIP(string const ip, string const mask, string const gw);

/////////////////////////////////////////////////////////////////////////////
protected:
	/* 纯虚函数, 继承类实现 */
	/*!
	 * @brief 继承类实现与相机的真正连接
	 * @return
	 * 连接结果
	 */
	virtual bool open_camera() = 0;
	/*!
	 * @brief 继承类实现真正与相机断开连接
	 */
	virtual void close_camera() = 0;
	/*!
	 * @brief 改变制冷状态
	 */
	virtual void cooler_onoff(bool &onoff, float &coolset) = 0;
	/*!
	 * @brief 采集探测器芯片温度
	 */
	virtual float sensor_temperature() = 0;
	/*!
	 * @brief 启动曝光
	 */
	virtual bool start_expose(float duration, bool light) = 0;
	/*!
	 * @brief 中止曝光
	 */
	virtual void stop_expose() = 0;
	/*!
	 * @brief 检测曝光状态
	 */
	virtual CAMERA_STATUS camera_state() = 0;
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
	virtual CAMERA_STATUS download_image() = 0;
};
typedef boost::shared_ptr<CameraBase> cambase;

/*!
 * @brief 将CameraBase继承类的boost::shared_ptr型指针转换为cambase类型
 * @param camera 相机控制接口指针
 * @return
 * cambase类型指针
 */
template <class T>
cambase to_cambase(T camera) {
	return boost::static_pointer_cast<CameraBase>(camera);
}

/*!
 * @brief 将cambase类型指针转换为其继承类的boost::shared_ptr型指针
 * @param camera 相机控制接口指针
 * @return
 * cambase继承类指针
 */
template <class T>
boost::shared_ptr<T> from_cambase(cambase camera) {
	return boost::static_pointer_cast<T>(camera);
}

#endif /* SRC_CAMERABASE_H_ */
