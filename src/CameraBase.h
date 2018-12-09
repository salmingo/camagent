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
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>

using std::string;
using namespace boost::posix_time;

/* 声明数据类型 */
enum CAMERA_STATUS {// 相机工作状态
	CAMERA_ERROR,	// 错误
	CAMERA_IDLE,	// 空闲
	CAMERA_EXPOSE,	// 曝光过程中
	CAMERA_IMGRDY	// 已完成曝光, 可以读出数据进入内存
};

/*!
 * @brief 声明曝光进度回调函数
 * @param <1> 曝光剩余时间, 量纲: 秒
 * @param <2> 曝光进度, 量纲: 百分比
 * @param <3> 图像数据状态, CAMERA_STATUS
 */
typedef boost::signals2::signal<void (const double, const double, const int)> ExposeProcess;
typedef ExposeProcess::slot_type ExpProcCBSlot;

/*!
 * @struct ROI 定义ROI区
 */
struct ROI {
	int binX, binY;		//< 合并因子
	int startX, startY;	//< ROI区左上角在全帧中的位置, 起始坐标: [1, 1]
	int width, height;	//< ROI区在全帧中的尺寸

public:
	int Width() {
		return (width / binX);
	}

	int Height() {
		return (height / binY);
	}

	void Reset(int w, int h) {
		startX = startY = 1;
		binX = binY = 1;
		width  = w;
		height = h;
	}
};

/*!
 * @struct DeviceCameraInfo 相机工作状态
 */
struct DeviceCameraInfo {
	string	model;		//< 相机型号
	string	serialno;	//< 序列号
	int		sensorW;	//< 探测器宽度, 量纲: 像素
	int		sensorH;	//< 探测器高度, 量纲: 像素
	float	pixelX;		//< 单像素尺寸, 量纲: 微米
	float	pixelY;		//< 单像素尺寸, 量纲: 微米
	bool	coolerOn;	//< 启用制冷
	float	coolerSet;	//< 探测器制冷温度, 量纲: 摄氏度
	float	coolerGet;	//< 探测器温度, 量纲: 摄氏度
	//<< EM CCD
	bool	EMCCD;		//< 支持EM功能
	bool	EMOn;		//< 启用EM功能
	int		EMGainLow;	//< EM增益下限
	int		EMGainHigh;	//< EM增益上限
	int		EMGain;		//< EM增益挡位
	//>> EM CCD
	//<< 关键控制参数
	uint16_t	iADChannel;	//< A/D转换挡位
	uint16_t	iReadPort;	//< 读出端口挡位
	uint16_t	iReadRate;	//< 读出速度挡位
	uint16_t	iVSRate;	//< 行转移挡位
	uint16_t	iGain;		//< 增益挡位

	uint16_t	ADBitPixel;	//< A/D转换位数
	string		ReadPort;	//< 读出端口
	string		ReadRate;	//< 读出速度
	float		VSRate;		//< 行转移速度
	float		gain;		//< 增益, 量纲: e-/DU
	//>> 关键控制参数
	bool			connected;	//< 相机连接标志
	CAMERA_STATUS	state;		//< 工作状态
	int				errcode;	//< 错误代码
	string			errmsg;		//< 错误描述
	ROI				roi;		//< ROI区
	float			expdur;		//< 曝光时间, 量纲: 秒
	ptime	tmobs;	//< 曝光起始时间
	bool	ampm;	//< true: A.M.; false: P.M.
	double	jd;		//< 曝光起始时间对应的儒略日
	string	dateobs;	//< 曝光起始时间对应的日期, 用于fits头和目录名, 格式: CCYY-MM-DD
	string	timeobs;	//< 曝光起始时间对应的时间, 格式: hh:mm:ss.ssssss
	string	timeend;	//< 曝光结束时间对应的时间, 格式: hh:mm:ss.ssssss
	string	utctime;	//< 曝光起始时间对应的时间, 用于生成文件名, 格式: YYMMDDThhmmssss
	boost::shared_array<uint8_t> data;	//< 图像数据存储区

public:
	DeviceCameraInfo() {
		sensorW = sensorH = 0;
		pixelX = pixelY = 0.0;
		coolerOn	= false;
		coolerSet	= -50.0;
		coolerGet	= 0.0;
		EMCCD		= false;
		EMOn		= false;
		EMGainLow	= 1;
		EMGainHigh	= 100;
		EMGain		= 1;
		iADChannel	= -1;
		iReadPort	= -1;
		iReadRate	= -1;
		iVSRate		= -1;
		iGain		= -1;
		ADBitPixel	= 16;
		VSRate		= 0.0;
		gain		= 0.0;
		connected	= false;
		state		= CAMERA_ERROR;
		errcode		= 0;
		expdur		= 0.0;
		ampm		= false;
		jd			= 0.0;
	}

	/*!
	 * @brief 设置探测器尺寸
	 * @param w 宽度, 量纲: 像素
	 * @param h 高度, 量纲: 像素
	 */
	void SetSensorDimension(int w, int h) {
		sensorW = w;
		sensorH = h;
		roi.Reset(w, h);
	}

	/*!
	 * @brief 为图像数据分配存储区
	 * @note
	 * 前提:
	 * - 已设置探测器尺寸
	 * - 已设置A/D转换位数
	 */
	void AllocImageBuffer() {
		int n = ADBitPixel / 8;
		if (n * 8 < ADBitPixel) ++n;
		if (n > 1 && (n % 2)) ++n;
		n = n * roi.Width() * roi.Height();
		if (n > 0) data.reset(new uint8_t[n]);
	}

	void BeginExpose(float duration) {
		tmobs = microsec_clock::universal_time();
		expdur = duration;
		state  = CAMERA_EXPOSE;
	}

	void FormatUTC() {
		ptime::date_type day = tmobs.date();
		ptime::time_duration_type td = tmobs.time_of_day();
		boost::format fmt("%02d%02d%02dT%02d%02d%02d%02d");

		dateobs = to_iso_extended_string(day);
		timeobs = to_simple_string(td);
		fmt % (day.year() - 2000) % day.month() % day.day()
				% td.hours() % td.minutes()
				% td.seconds() % (td.fractional_seconds() / 10000);
		utctime = fmt.str();
		jd = day.julian_day() + td.total_microseconds() * 1E-6 / 86400.0 - 0.5;

		ampm = second_clock::local_time().time_of_day().hours() < 12;
	}

	void EndExpose() {
		timeend = to_simple_string(microsec_clock::universal_time().time_of_day());
	}

	void CheckExpose(double &left, double &percent) {
		time_duration elps = microsec_clock::universal_time() - tmobs;
		double t = elps.total_microseconds() * 1E-6;
		if ((left = expdur - t) < 0.0 || expdur < 1E-6) {
			left    = 0.0;
			percent = 100.000001;
		}
		else percent = t * 100.000001 / expdur;
	}
};
typedef boost::shared_ptr<DeviceCameraInfo> NFDevCamPtr;

class CameraBase {
protected:
	/* 声明数据类型 */
	typedef boost::shared_ptr<boost::thread> threadptr;
	typedef boost::unique_lock<boost::mutex> mutex_lock;

protected:
	NFDevCamPtr nfptr_;		//< 相机参数及工作状态
	ExposeProcess exproc_;	//< 曝光进度回调函数
	boost::condition_variable cvexp_;	//< 事件变量: 通知开始曝光进度
	threadptr thrdCycle_;		//< 线程: 采集探测器温度
	threadptr thrdExpose_;		//< 线程: 监测曝光进度和结果

public:
	CameraBase();
	virtual ~CameraBase();

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
	 * 成功标志
	 */
	bool SetCooler(float coolerset = -20.0, bool onoff = false);
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
	/*!
	 * @brief 软重启相机
	 * @return
	 * 相机重启结果
	 */
	virtual bool SoftwareReboot();
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
	void RegisterExposeProc(const ExpProcCBSlot &slot);

protected:
	/*!
	 * @brief 线程: 定时采集探测器温度
	 */
	void thread_cycle();
	/*!
	 * @brief 线程: 监测曝光进度
	 */
	void thread_expose();
	/*!
	 * @brief 结束线程
	 */
	void exit_thread(threadptr &thrd);

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
	 * @brief 更新ROI区域
	 * @param xb  X轴合并因子
	 * @param yb  Y轴合并因子
	 * @param x   X轴起始位置, 相对原始图像起始位置
	 * @param y   Y轴起始位置, 相对原始图像起始位置
	 * @param w   ROI区宽度
	 * @param h   ROI区高度
	 */
	virtual void update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) = 0;
	/*!
	 * @brief 改变制冷状态
	 */
	virtual void cooler_onoff(float &coolset, bool &onoff) = 0;
	/*!
	 * @brief 采集探测器芯片温度
	 */
	virtual float sensor_temperature() = 0;
	/*!
	 * @brief 设置AD通道
	 */
	virtual void update_adchannel(uint16_t &index, uint16_t &bitpix) = 0;
	/*!
	 * @brief 设置读出端口
	 */
	virtual void update_readport(uint16_t &index, string &readport) = 0;
	/*!
	 * @brief 设置读出速度
	 */
	virtual void update_readrate(uint16_t &index, string &readrate) = 0;
	/*!
	 * @brief 设置行转移速度
	 */
	virtual void update_vsrate(uint16_t &index, float &vsrate) = 0;
	/*!
	 * @brief 设置增益
	 */
	virtual void update_gain(uint16_t &index, float &gain) = 0;
	/*!
	 * @brief 设置A/D基准偏压
	 */
	virtual void update_adoffset(uint16_t &index) = 0;
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
