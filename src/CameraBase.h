/*
 * @file CameraBase.h 相机基类定义文件, 定义相机统一访问接口
 * @version 1.0
 * @date 2018-04-26
 * @note
 * - 实现相机工作逻辑流程
 * - 定义真实相机需实现的纯虚函数
 */

#ifndef SRC_CAMERABASE_H_
#define SRC_CAMERABASE_H_

#include <boost/smart_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <string>

using std::string;

enum CAMERA_STATUS {// 相机工作状态
	CAMERA_ERROR,	// 错误
	CAMERA_IDLE,	// 空闲
	CAMERA_EXPOSE,	// 曝光过程中
	CAMERA_IMGRDY	// 已完成曝光, 可以读出数据进入内存
};

class CameraBase {
public:
	CameraBase();
	virtual ~CameraBase();

public:
	/* 声明数据类型 */
	/*!
	 * @brief 声明曝光进度回调函数
	 * @param <1> 曝光剩余时间, 量纲: 秒
	 * @param <2> 曝光进度, 量纲: 百分比
	 * @param <3> 图像数据状态, CAMERA_STATUS
	 */
	typedef boost::signals2::signal<void (const double, const double, const int)> ExposeProcess;
	typedef ExposeProcess::slot_type CBSlot;
	/*!
	 * @struct DeviceCameraInfo 相机工作状态
	 */
	struct DeviceCameraInfo {
		string			model;			//< 相机型号
		string			serialno;		//< 序列号
		int				sensorW;		//< 探测器宽度, 量纲: 像素
		int				sensorH;		//< 探测器高度, 量纲: 像素
		float			pixelX;			//< 单像素尺寸, 量纲: 微米
		float			pixelY;			//< 单像素尺寸, 量纲: 微米
		bool			coolerOn;		//< 启用制冷
		float			coolerSet;		//< 探测器制冷温度, 量纲: 摄氏度
		//<< EM CCD
		bool			EMCCD;			//< 支持EM功能
		bool			EMOn;			//< 启用EM功能
		int				EMGainLow;		//< EM增益下限
		int				EMGainHigh;		//< EM增益上限
		int				EMGain;			//< EM增益挡位
		//>> EM CCD
		//<< 关键控制参数
		int				iADChannel;		//< A/D转换挡位
		int				iReadPort;		//< 读出端口挡位
		int				iReadRate;		//< 读出速度挡位
		int				iVSRate;		//< 行转移挡位
		int				iGain;			//< 增益挡位

		int				ADBitPixel;		//< A/D转换位数
		string			ReadPort;		//< 读出端口
		string			ReadRate;		//< 读出速度
		float			VSRate;			//< 行转移速度
		float			gain;			//< 增益, 量纲: e-/DU
		//>> 关键控制参数
		bool			connected;		//< 相机连接标志
		CAMERA_STATUS	state;			//< 工作状态
		float			expdur;			//< 曝光时间, 量纲: 秒
		boost::shared_array<uint8_t> data_;	//< 图像数据存储区
	};
	typedef boost::shared_ptr<DeviceCameraInfo> NFDevCamPtr;

public:
	const NFDevCamPtr GetCameraInfo();
	/*!
	 * @brief 相机连接标志
	 * @return
	 * 是否已经建立与相机的连接
	 */
	bool IsConnected();
	/*!
	 * @brief 尝试连接相机
	 * @return
	 * 相机连接结果
	 */
	bool Connect();
	/*!
	 * @brief 断开与相机的连接
	 */
	void Disconnect();
	/*!
	 * @brief 设置制冷温度
	 * @param coolerset 制冷温度
	 * @param onoff
	 * @return
	 * 生效的制冷温度
	 */
	double SetCooler(double coolerset = -20.0, bool onoff = false);
	/*!
	 * @brief 改变读出端口
	 * @param index 读出端口索引
	 * @return
	 * 改变后读出端口索引. 若设置成功, 则返回值等于index
	 */
	uint16_t SetReadPort(uint16_t index);
	/*!
	 * @brief 改变读出速度
	 * @param index 读出速度索引
	 * @return
	 * 改变后读出速度索引. 若设置成功, 则返回值等于index
	 */
	uint16_t SetReadRate(uint16_t index);
	/*!
	 * @brief 改变增益
	 * @param index 增益索引
	 * @return
	 * 改变后增益索引. 若设置成功, 则返回值等于index
	 */
	uint16_t SetGain(uint16_t index);
	/*!
	 * @brief 改变ROI区
	 * @param xbin   X轴合并因子
	 * @param ybin   Y轴合并因子
	 * @param xstart X轴起始位置, 相对原始图像起始位置
	 * @param ystart Y轴起始位置, 相对原始图像起始位置
	 * @param width  ROI区宽度
	 * @param height ROI区高度
	 */
	void SetROI(int &xbin, int &ybin, int &xstart, int &ystart, int &width, int &height);
	/*!
	 * @brief 改变基准偏压, 使得本底图像统计均值为offset
	 * @param offset 本底平均期望值
	 * @return
	 * 实际本底平均期望值
	 */
	uint16_t SetADCOffset(uint16_t offset);
	/*!
	 * @brief 尝试启动曝光流程
	 * @param duration  曝光周期, 量纲: 秒
	 * @param light     是否需要外界光源
	 * @return
	 * 曝光启动结果
	 */
	bool Expose(double duration, bool light = true);
	/*!
	 * @brief 中止当前曝光过程
	 */
	void AbortExpose();
	/*!
	 * @brief 软重启相机
	 * @return
	 * 相机重启结果
	 */
	virtual bool SoftwareReboot();
	/*!
	 * @brief 注册曝光进度回调函数
	 * @param slot 函数插槽
	 */
	void RegisterExposeProc(const CBSlot &slot);

protected:
	/* 纯虚函数, 继承类实现 */
	/*!
	 * @brief 继承类实现与相机的真正连接
	 * @return
	 * 连接结果
	 */
	virtual bool OpenCamera() = 0;
	/*!
	 * @brief 继承类实现真正与相机断开连接
	 */
	virtual void CloseCamera() = 0;

protected:
	/* 声明数据类型 */
	typedef boost::shared_ptr<boost::thread> threadptr;
	typedef boost::unique_lock<boost::mutex> mutex_lock;

protected:
	NFDevCamPtr nfptr_;				//< 相机参数及工作状态
	ExposeProcess exposeproc_;		//< 曝光进度回调函数
	threadptr thrdExpose_;			//< 线程: 监测曝光进度和结果

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
