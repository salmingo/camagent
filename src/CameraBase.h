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

#include <boost/smart_ptr.hpp>
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

		int Pixels() {
			return Width() * Height();
		}

		void Reset(int w, int h) {
			startX = startY = 1;
			binX = binY = 1;
			width  = w;
			height = h;
		}
	};

	/*!
	 * @struct CameraInfo 相机信息集合
	 */
	struct CameraInfo {
		/** 基本信息 **/
		string model;		//< 相机型号
		string serno;		//< 相机序列号
		int sensorW;		//< 探测器宽度
		int sensorH;		//< 探测器高度
		float pixelX;		//< 单像素尺寸, 量纲: 微米
		float pixelY;		//< 单像素尺寸, 量纲: 微米
		/** 读出参数 **/
		/* 档位 */
		uint16_t iADChannel;	//< A/D通道
		uint16_t iReadPort;		//< 读出端口
		uint16_t iReadRate;		//< 读出速度
		uint16_t iGain;			//< 增益
		uint16_t iVSRate;		//< 行转移速度
		/* 数值或名称 */
		uint16_t bitpixel;	//< 单像素数据位数, 量纲: bit
		string readport;	//< 读出端口
		string readrate;	//< 读出速度
		float  gain;		//< 增益, 量纲: e-/DU
		float  vsrate;		//< 转移速度, 量纲: 微秒/行
		/* Electron Multiple: 电子倍增 */
		bool EMCCD;		//< 支持EM功能
		bool EMOn;		//< 启用EM功能
		int  EMGain;	//< EM增益
		/** 制冷 **/
		bool coolOn;	//< 启用制冷
		float coolSet;	//< 制冷温度, 量纲: 摄氏度
		float CoolGet;	//< 探测器温度, 量纲: 摄氏度
		/** 相机状态 **/
		bool connected;			//< 与相机连接标志
		CAMERA_STATUS state;	//< 工作状态
		int errcode;			//< 错误代码
		string errmsg;			//< 错误描述

		/** 曝光参数 **/
		ROI roi;		//< ROI区
		float exptm;	//< 积分时间, 量纲: 秒

		/** 曝光时标 **/
		bool ampm;		//< 上下午标志. true: A.M.; false: P.M.
		ptime tmobs;	//< 曝光起始时间
		ptime tmend;	//< 曝光结束时间

		/** 图像数据存储区 **/
		boost::shared_array<uint8_t> data;

	public:
		/*!
		 * @brief 设置探测器分辨率
		 * @param w 宽度
		 * @param h 高度
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
		 * - 已设置ROI区
		 * - 已采集A/D位数
		 */
		void AllocImageBuffer() {
			int n = bitpixel / 8;
			if (n * 8 < bitpixel) ++n;
			if (n > 1 && (n % 2)) ++n;
			n *= (roi.Width() * roi.Height());
			data.reset(new uint8_t[n]);
		}

		/*!
		 * @brief 开始曝光
		 * @param duration 积分时间, 量纲: 秒
		 */
		void ExposeBegin(float duration) {
			tmobs = microsec_clock::universal_time();
			exptm = duration;
			state = CAMERA_EXPOSE;
		}

		/*!
		 * @brief 完成曝光
		 */
		void ExposeEnd() {
			tmend = microsec_clock::universal_time();
		}

		/*!
		 * @brief 检查曝光进度
		 * @param left    剩余曝光时间, 量纲: 秒
		 * @param percent 曝光进度, 量纲: 百分比
		 * @return
		 * 曝光进行中标志
		 */
		void CheckExpose(double &left, double &percent) {
			time_duration elps = microsec_clock::universal_time() - tmobs;
			double gone = elps.total_microseconds() * 1E-6;
			if (exptm < 1E-6 || (left = exptm - gone) < 0.0) {
				left = 0.0;
				percent = 100.001;
			}
			else {
				percent = gone * 100.001 / exptm;
			}
		}
	};
	typedef boost::shared_ptr<CameraInfo> NFCamPtr;
	typedef boost::shared_ptr<boost::thread> threadptr;
	typedef boost::unique_lock<boost::mutex> mutex_lock;

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
	NFCamPtr nfptr_;		//< 相机参数及工作状态
	threadptr thrdexp_;		//< 线程: 监测曝光进度和结果
	threadptr thrdcool_;	//< 线程: 相机空闲时监测探测器温度
	boost::condition_variable cvexp_;	//< 事件: 曝光进度发生变化

/////////////////////////////////////////////////////////////////////////////
public:
	/*!
	 * @brief 查看相机参数及工作状态
	 */
	NFCamPtr GetCameraInfo();

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
	bool UpdateCooler(bool onoff = false, float set = -40.0);
	/*!
	 * @brief 改变A/D转换通道
	 * @param index A/D通道索引
	 * @return
	 * 成功标志
	 */
	bool UpdateADChannel(uint16_t index);
	/*!
	 * @brief 改变读出端口
	 * @param index 读出端口索引
	 * @return
	 * 成功标志
	 */
	bool UpdateReadPort(uint16_t index);
	/*!
	 * @brief 改变读出速度
	 * @param index 读出速度索引
	 * @return
	 * 成功标志
	 */
	bool UpdateReadRate(uint16_t index);
	/*!
	 * @brief 设置行转移速度
	 * @param index 行转移速度索引
	 * @return
	 * 成功标志
	 */
	bool UpdateVSRate(uint16_t index);
	/*!
	 * @brief 改变前置增益
	 * @param index 增益索引
	 * @return
	 * 成功标志
	 */
	bool UpdateGain(uint16_t index);
	/*!
	 * @brief 改变EM增益
	 * @param gain 增益
	 * @return
	 * 成功标志
	 */
	virtual bool UpdateEMGain(uint16_t gain);
	/*!
	 * @brief 改变基准偏压, 使得本底图像统计均值为offset
	 * @param offset 本底平均期望值
	 * @return
	 * 成功标志
	 */
	bool UpdateADCOffset(uint16_t offset);
	/*!
	 * @brief 改变ROI区
	 * @param xb  X轴合并因子
	 * @param yb  Y轴合并因子
	 * @param x   X轴起始位置, 相对原始图像起始位置
	 * @param y   Y轴起始位置, 相对原始图像起始位置
	 * @param w   ROI区宽度
	 * @param h   ROI区高度
	 */
	bool UpdateROI(int &xb, int &yb, int &x, int &y, int &w, int &h);
	/*!
	 * @brief 设置相机IP地址
	 * @return
	 * 操作结果
	 */
	virtual bool UpdateIP(string const ip, string const mask, string const gw);

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
	virtual bool close_camera() = 0;
	/*!
	 * @brief 改变制冷状态
	 */
	virtual bool cooler_onoff(bool &onoff, float &coolset) = 0;
	/*!
	 * @brief 采集探测器芯片温度
	 */
	virtual float sensor_temperature() = 0;
	/*!
	 * @brief 设置AD通道
	 */
	virtual bool update_adchannel(uint16_t &index, uint16_t &bitpix) = 0;
	/*!
	 * @brief 设置读出端口
	 */
	virtual bool update_readport(uint16_t &index, string &readport) = 0;
	/*!
	 * @brief 设置读出速度
	 */
	virtual bool update_readrate(uint16_t &index, string &readrate) = 0;
	/*!
	 * @brief 设置行转移速度
	 */
	virtual bool update_vsrate(uint16_t &index, float &vsrate) = 0;
	/*!
	 * @brief 设置前置增益
	 */
	virtual bool update_gain(uint16_t &index, float &gain) = 0;
	/*!
	 * @brief 设置A/D基准偏压
	 */
	virtual bool update_adoffset(uint16_t value) = 0;
	/*!
	 * @brief 更新ROI区域
	 * @param xb  X轴合并因子
	 * @param yb  Y轴合并因子
	 * @param x   X轴起始位置, 相对原始图像起始位置
	 * @param y   Y轴起始位置, 相对原始图像起始位置
	 * @param w   ROI区宽度
	 * @param h   ROI区高度
	 */
	virtual bool update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) = 0;
	/*!
	 * @brief 启动曝光
	 */
	virtual bool start_expose(float duration, bool light) = 0;
	/*!
	 * @brief 中止曝光
	 */
	virtual bool stop_expose() = 0;
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

protected:
	/*!
	 * @brief 线程: 采集探测器温度
	 */
	void thread_cool();
	/*!
	 * @brief 线程: 监测曝光进度
	 */
	void thread_expose();
	/*!
	 * @brief 中断线程
	 * @param thrd 线程指针
	 */
	void int_thread(threadptr &thrd);
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
