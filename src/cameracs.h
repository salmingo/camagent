/*
 * @file cameracs.h 控制程序入口声明文件
 * @date 2017年4月9日
 * @version 0.3
 * @author Xiaomeng Lu
 * @note
 * 功能列表:
 * (1) 响应和反馈网络消息
 * (2) 控制相机工作流程
 * (3) 存储FITS文件
 * (4) 同步NTP时钟
 * (5) 上传FITS文件
 * @version 0.6
 * @date 2017年6月8日
 * - 更新相机工作状态
 * - 更新相机控制指令
 *
 * @date 2017年11月1日
 * - 更新相机工作状态, 增加异常处理
 *
 * @version 0.7
 * @date 2017年11月9日
 * - 删除互斥锁mtxsys_和mtxTcp_, 由消息队列实现互斥
 * - 在OnCompleteWait中启动曝光
 *
 * @date 2017年11月??日
 * - fits文件首先存储在内存虚拟硬盘中, 完成图像显示、网络传输和硬盘存储后删除
 * - 更改文件上传接口
 * - 增加磁盘剩余空间监测与磁盘清理
 *
 * @date 2017年11月??日
 * - 加入相机重连机制, 需要在与相机操作和访问前查看相机连接状态
 * - 重连机制:
 *   (1) 当相机出错时, 断开与相机连接
 *   (2) 间隔周期1、2、3、4、5分钟, 尝试重连相机
 *   (3) 连续5次重连失败, 退出程序
 */

#ifndef CAMERACS_H_
#define CAMERACS_H_

#include <sys/vfs.h>
#include <boost/algorithm/string.hpp>
#include "msgque_base.h"
#include "asciiproto.h"
#include "NTPClient.h"
#include "tcp_asio.h"
#include "parameter.h"
#include "CameraBase.h"
#include "FileTransferClient.h"

class cameracs : public msgque_base {
public:
	cameracs(boost::asio::io_service* ioserv);
	virtual ~cameracs();

private:
	/* 局部数据类型声明 */
	typedef boost::unique_lock<boost::mutex> mutex_lock;
	typedef boost::shared_ptr<boost::thread> threadptr;

	enum {// 定义消息
		MSG_CONNECT_GC = MSG_USER,	//< 消息: 与总控服务器网络连接结果
		MSG_RECEIVE_GC,		//< 消息: 处理来自总控服务器信息
		MSG_CLOSE_GC,		//< 消息: 总控服务器断开网络连接
		MSG_PREPARE_EXPOSE,	//< 消息: 准备开始曝光
		MSG_PROCESS_EXPOSE,	//< 消息: 曝光中
		MSG_COMPLETE_EXPOSE,//< 消息: 曝光正常结束
		MSG_ABORT_EXPOSE,	//< 消息: 完成物理曝光终止过程
		MSG_FAIL_EXPOSE,	//< 消息: 曝光过程中错误
		MSG_COMPLETE_WAIT,	//< 消息: 等待线程正常结束
		MSG_PERIOD_INFO,	//< 消息: 发送周期信息
		MSG_CONNECT_CAMERA,	//< 消息: 连接相机成功
		MSG_CCS_END
	};

	struct system_info {// 系统信息
		bool	connected;	//< 连接标志
		int		command;	//< 控制指令
		int		state;		//< 工作状态.
		int		freedisk;	//< 空闲磁盘空间, 量纲: MB

	public:
		system_info() {
			connected = false;
			command = state = -1;
			freedisk = -1;
		}

		void update_freedisk(const char* pathroot) {// 1MB = 1073741824 Bytes
			struct statfs stat;

			if (!statfs(pathroot, &stat)) freedisk = stat.f_bsize * stat.f_bavail / 1048576;
		}
	};

	struct object_info {// 观测目标信息
		int		op_sn;		//< 计划序列号
		string	op_time;	//< 观测计划生成时间
		string	op_type;	//< 观测计划类型
		string	obstype;	//< 观测类型
		string	grid_id;	//< 天区划分模式
		string	field_id;	//< 天区标志
		string	obj_id;		//< 目标名
		double	ra;			//< 指向中心赤经, 量纲: 角度
		double	dec;		//< 指向中心赤纬, 量纲: 角度
		double	epoch;		//< 指向中心位置的坐标系, 历元
		double	objra;		//< 目标赤经, 量纲: 角度
		double	objdec;		//< 目标赤纬, 量纲: 角度
		double	objepoch;	//< 目标位置的坐标系, 历元
		string	objerror;	//< 目标位置误差
		string	imgtype;	//< 图像类型
		string	imgtypeab;	//< 图像类型缩写
		int		iimgtype;	//< 图像类型
		double	expdur;		//< 曝光时间, 量纲: 秒
		double	delay;		//< 延迟时间, 量纲: 秒
		bool	light;		//< 是否需要天光
		int		frmcnt;		//< 总帧数
		int		priority;	//< 优先级
		ptime	begin_time;	//< 观测起始时间
		ptime	end_time;	//< 观测结束时间
		int		pair_id;	//< 分组标志
		string	filter;		//< 滤光片
		int		focus;		//< 焦点位置
		int		frmno;		//< 观测已完成帧数
		string  subpath;	//< 最后一个文件在设定根路径下的目录名称
		string	pathname;	//< 最后一个文件的存储目录名
		string	filename;	//< 最后一个文件的名称
		string	filepath;	//< 最后一个文件的全路径名

	public:
		object_info() {
			op_sn   = -1;
			op_time = op_type = "";
			obstype = "";
			grid_id = field_id = "";
			obj_id = "";
			ra = dec = objra = objdec = -1000.0;
			epoch = objepoch = -1000.0;
			objerror = "";
			imgtype = "";
			iimgtype = IMGTYPE_LAST;
			expdur = delay = -1.0;
			light = false;
			frmcnt = 0;
			priority = INT_MAX;	// 手动曝光时默认优先级最高
			pair_id = -1;
			filter  = "";
			focus   = INT_MIN;
			frmno = 0;
			subpath  = "";
			pathname = "";
			filename = "";
			filepath = "";
		}

		object_info& operator=(const ascproto_object_info& proto) {
			op_sn		= proto.op_sn;
			op_time		= proto.op_time;
			op_type		= proto.op_type;
			obstype		= proto.obstype;
			grid_id		= proto.grid_id;
			field_id	= proto.field_id;
			obj_id		= proto.obj_id;
			ra			= proto.ra;
			dec			= proto.dec;
			epoch		= proto.epoch;
			objra		= proto.objra;
			objdec		= proto.objdec;
			objepoch	= proto.objepoch;
			objerror	= proto.objerror;
			imgtype		= proto.imgtype;
			expdur		= proto.expdur;
			delay		= proto.delay;
			frmcnt		= proto.frmcnt;
			priority	= proto.priority;
			pair_id		= proto.pair_id;
			filter		= proto.filter;
			frmno		= 0;
			subpath		= "";
			pathname	= "";
			filename	= "";
			filepath	= "";
			// 处理图像类型
			light = true;
			if      (boost::iequals(imgtype, "OBJECT")) { imgtypeab = "objt"; iimgtype = IMGTYPE_OBJECT; }
			else if (boost::iequals(imgtype, "BIAS"))   { imgtypeab = "bias"; light = false; iimgtype = IMGTYPE_BIAS; }
			else if (boost::iequals(imgtype, "DARK"))   { imgtypeab = "dark"; light = false; iimgtype = IMGTYPE_DARK; }
			else if (boost::iequals(imgtype, "FLAT"))   { imgtypeab = "flat"; iimgtype = IMGTYPE_FLAT; }
			else if (boost::iequals(imgtype, "FOCUS"))  { imgtypeab = "focs"; iimgtype = IMGTYPE_FOCUS; }
			// 处理起始/结束时间

			return *this;
		}

		bool Isvalid_begin() {// 起始时间是否有效
			return !begin_time.is_special();
		}

		bool Isvalid_end() {// 结束时间是否有效
			return !end_time.is_special();
		}
	};

	/* 局部成员变量 */
	boost::asio::io_service* io_main_;	//< 主io服务, 用于内部终止程序
	boost::shared_ptr<param_config>			param_;		//< 程序配置参数
	boost::shared_ptr<system_info>			nfsys_;		//< 系统信息
	boost::shared_ptr<object_info>			nfobj_;		//< 观测目标
	boost::shared_ptr<CameraBase>			camera_;	//< 相机操作接口
	boost::shared_ptr<tcp_client>			tcpgc_;		//< 与总控服务器的网络连接
	boost::shared_ptr<FileTransferClient>	filecli_;	//< 文件传输服务
	boost::shared_ptr<NTPClient>			ntp_;		//< NTP守时服务
	boost::shared_ptr<ascii_proto>			ascproto_;	//< 解析ASCII网络协议
	boost::shared_ptr<ascproto_camera_info> protonf_;	//< 相机信息网络协议
	boost::shared_array<char>				bufrcv_;	//< 与总控服务器之间的网络信息接收缓冲区
	threadptr	thrdCycle_;	//< 周期线程, 定时检查系统状态, 并发送网络信息
	threadptr	thrdWait_;	//< 延时等待线程, 等待曝光起始时间到达
	threadptr	thrdReCam_;	//< 线程, 重连相机

	ptime tmlastsend_;		//< 最后一次发送相机信息的时标
	ptime tmlastconnect_;	//< 最后一次尝试连接服务器的时标
	bool firstimg_;		//< 第一张图像
	int  resetdelay_;	//< 是否需要调整延时
	bool registered_;	//< 是否已在服务器中注册身份

public:
	/*!
	 * @brief 启动相关服务
	 * @return
	 * 服务启动结果
	 */
	bool Start();
	/*!
	 * @brief 中止所有服务
	 */
	void Stop();

protected:
	/*!
	 * @brief 注册消息响应函数
	 */
	void register_messages();
	/*！
	 * @brief 连接总控服务器的回调函数
	 */
	void ConnectedGC(const long client, const long ec);
	/*！
	 * @brief 处理来自总控服务器信息的回调函数
	 */
	void ReceivedGC(const long client, const long ec);

protected:
	/*!
	 * @brief 处理与总控服务器的连接结果
	 * @param ec 0: 连接成功; <>0: 连接失败
	 */
	void OnConnectGC(long ec, long);
	/*!
	 * @brief 处理来自总控服务器的网络信息
	 */
	void OnReceiveGC(long, long);
	/*!
	 * @brief 总控服务器断开网络连接
	 */
	void OnCloseGC(long, long);
	/*!
	 * @brief 曝光前检测和准备
	 */
	void OnPrepareExpose(long, long);
	/*!
	 * @brief 曝光进行中
	 * @param left    剩余曝光时间, 量纲: 微秒
	 * @param percent 曝光进度, 量纲: 0.01百分比
	 */
	void OnProcessExpose(long left, long percent);
	/*!
	 * @brief 曝光正常结束
	 */
	void OnCompleteExpose(long, long);
	/*!
	 * @brief 物理上完成终止曝光流程
	 */
	void OnAbortExpose(long, long);
	/*!
	 * @brief 曝光过程中错误
	 */
	void OnFailExpose(long, long);
	/*!
	 * @brief 延时等待线程正常结束
	 */
	void OnCompleteWait(long, long);
	/*!
	 * @brief 发送周期信息
	 */
	void OnPeriodInfo(long, long);
	/*!
	 * @brief 成功建立与相机连接
	 */
	void OnConnectCamera(long, long);

protected:
	/*!
	 * @brief 连接相机, 并准备工作环境
	 * @return
	 * 相机连接结果
	 */
	bool connect_camera();
	/*!
	 * @brief 断开与相机连接, 并释放工作环境
	 */
	void disconnect_camera();
	/*!
	 * @brief 启动NTP时钟守时服务
	 */
	void start_ntp();
	/*!
	 * @brief 停止NTP时钟守时服务
	 */
	void stop_ntp();
	/*!
	 * @brief 连接总控服务器
	 */
	void connect_gc();
	/*!
	 * @brief 断开与总控服务器的网络连接
	 */
	void disconnect_gc();
	/*!
	 * @brief 启动文件传输服务
	 */
	void start_fs();
	/*!
	 * @brief 中止文件传输服务
	 */
	void stop_fs();
	/*!
	 * @brief 曝光进度回调函数
	 * @param left    曝光剩余时间, 量纲: 秒
	 * @param percent 曝光进度, 量纲: 百分比
	 * @param state   图像数据状态
	 */
	void ExposeProcessCB(const double left, const double percent, const int state);
	/*!
	 * @brief 周期线程, 检查系统工作状态并发送网络信息
	 */
	void ThreadCycle();
	/*!
	 * @brief 触发线程, 等待到达约定曝光开始时间
	 * @param tmobs 曝光起始时间
	 */
	void ThreadWait(const boost::posix_time::ptime &tmobs);
	/*!
	 * @brief 线程, 尝试重新连接相机
	 */
	void ThreadReCamera();
	/*!
	 * @brief 从外部中断线程
	 * @param thrd 线程对象
	 */
	void ExitThread(threadptr &thrd);
	/*!
	 * @brief 向总控服务器发送相机信息
	 * @param state 相机工作状态. -1表示发送心跳信息
	 */
	void SendCameraInfo(int state = -1);
	/*!
	 * @brief 启动曝光流程
	 */
	void StartExpose();
	/*!
	 * @brief 存储FITS文件
	 */
	bool SaveFITSFile();
	/*!
	 * @brief 上传文件
	 */
	void UploadFile();
	/*!
	 * @brief 处理控制台编码协议
	 * @param proto_type 协议类型
	 * @param proto_body 协议主体
	 */
	void ProcessProtocol(string& proto_type, apbase& proto_body);
	/*!
	 * @brief 计算平场图像的统计均值
	 * @return
	 * 统计均值
	 */
	double FlatMean();

	void DisplayImage();
	// set FITS file to display
	bool XPASetFile(const char *filepath, const char *filename, bool back = false);
	void XPASetScaleMode(const char *mode);
	void XPASetZoom(const char *zoom);
	void XPAPreservePan(bool yes = true);
	void XPAPreserveRegion(bool yes = true);

};

#endif /* CAMERACS_H_ */
