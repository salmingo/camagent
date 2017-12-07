/*!
 * @file AsciiProtocol.h 声明文件, 声明GWAC/GFT系统中字符串型通信协议
 * @version 0.1
 * @date 2017-11-17
 * - 通信协议采用Struct声明
 * - 通信协议继承自ascii_protocol_base
 */

#ifndef ASCIIPROTOCOL_H_
#define ASCIIPROTOCOL_H_

#include <list>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include "devicedef.h"
#include "AsciiProtocolBase.h"

using std::list;

//////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 声明通信协议 ---------------------------------*/
struct ascii_proto_reg : public ascii_proto_base {// 注册设备
	int result;	//< 注册结果. 0: 失败; != 0: 成功

public:
	ascii_proto_reg() {
		type = "register";
		result = 0;
	}
};
typedef boost::shared_ptr<ascii_proto_reg> apreg;
extern apreg makeap_reg();

/* 转台 */
struct ascii_proto_find_home : public ascii_proto_base {// 搜索零点
public:
	ascii_proto_find_home() {
		type = "find_home";
	}
};
typedef boost::shared_ptr<ascii_proto_find_home> apfindhome;
extern apfindhome makeap_findhome();

struct ascii_proto_home_sync : public ascii_proto_base {// 同步零点
	double ra;		//< 赤经, 量纲: 角度
	double dc;		//< 赤纬, 量纲: 角度
	double epoch;	//< 历元

public:
	ascii_proto_home_sync() {
		type = "home_sync";
		ra = dc = 1E30;
		epoch = 2000.0;
	}
};
typedef boost::shared_ptr<ascii_proto_home_sync> aphomesync;
extern aphomesync makeap_homesync();

struct ascii_proto_slewto : public ascii_proto_base {// 指向
	double ra;		//< 赤经, 量纲: 角度
	double dc;		//< 赤纬, 量纲: 角度
	double epoch;	//< 历元

public:
	ascii_proto_slewto() {
		type = "slewto";
		ra = dc = 1E30;
		epoch = 2000.0;
	}
};
typedef boost::shared_ptr<ascii_proto_slewto> apslewto;
extern apslewto makeap_slewto();

struct ascii_proto_park : public ascii_proto_base {// 复位
public:
	ascii_proto_park() {
		type = "park";
	}
};
typedef boost::shared_ptr<ascii_proto_park> appark;
extern appark makeap_park();

struct ascii_proto_guide : public ascii_proto_base {// 导星
	double ra;		//< 指向位置对应的天球坐标-赤经, 或赤经偏差, 量纲: 角度
	double dc;		//< 指向位置对应的天球坐标-赤纬, 或赤纬偏差, 量纲: 角度
	double objra;	//< 目标赤经, 量纲: 角度
	double objdc;	//< 目标赤纬, 量纲: 角度

public:
	ascii_proto_guide() {
		type = "guide";
		ra = dc = 1E30;
		objra = objdc = 1E30;
	}
};
typedef boost::shared_ptr<ascii_proto_guide> apguide;
extern apguide makeap_guide();

struct ascii_proto_abort_slew : public ascii_proto_base {// 中止指向
public:
	ascii_proto_abort_slew() {
		type = "abort_slew";
	}
};
typedef boost::shared_ptr<ascii_proto_abort_slew> apabortslew;
extern apabortslew makeap_abortslew();

struct ascii_proto_telescope_info : public ascii_proto_base {// 望远镜信息
	int state;		//< 工作状态
	int ec;			//< 错误代码
	double ra;		//< 指向赤经, 量纲: 角度
	double dc;		//< 指向赤纬, 量纲: 角度
	double objra;	//< 目标赤经, 量纲: 角度
	double objdc;	//< 目标赤纬, 量纲: 角度

public:
	ascii_proto_telescope_info() {
		type = "telescope_info";
		state = TELESCOPE_ERROR;
		ec = 0;
		ra = dc = 1E30;
		objra = objdc = 1E30;
	}
};
typedef boost::shared_ptr<ascii_proto_telescope_info> apnftele;
extern apnftele makeap_telescopeinfo();

struct ascii_proto_fwhm : public ascii_proto_base {// 半高全宽
	double value;	//< 半高全宽, 量纲: 像素

public:
	ascii_proto_fwhm() {
		type = "fwhm";
		value = -1E30;
	}
};
typedef boost::shared_ptr<ascii_proto_fwhm> apfwhm;
extern apfwhm makeap_fwhm();

struct ascii_proto_focus : public ascii_proto_base {// 焦点位置
	int    value;	//< 焦点位置, 量纲: 微米

public:
	ascii_proto_focus() {
		type = "focus";
		value = INT_MIN;
	}
};
typedef boost::shared_ptr<ascii_proto_focus> apfocus;
extern apfocus makeap_focus();

struct ascii_proto_focus_sync : public ascii_proto_base {// 同步焦点位置
public:
	ascii_proto_focus_sync() {
		type = "focus_sync";
	}
};
typedef boost::shared_ptr<ascii_proto_focus_sync> apfocusync;
extern apfocusync makeap_focusync();

struct ascii_proto_mcover : public ascii_proto_base {// 开关镜盖
	int value;	//< 复用字
				//< 用户/数据库=>服务器: 指令
				//< 服务器=>用户/数据库: 状态

public:
	ascii_proto_mcover() {
		type = "mcover";
		value = 0;
	}
};
typedef boost::shared_ptr<ascii_proto_mcover> apmcover;
extern apmcover makeap_mcover();

/* 相机 -- 上层 */
struct ascii_proto_takeimg : public ascii_proto_base {// 采集图像
	string obj_id;	//< 目标名
	string imgtype;	//< 图像类型
	int iimgtype;	//< 图像类型
	string imgtypeabbr;	//< 图像类型缩写
	double expdur;	//< 曝光时间, 量纲: 秒
	double delay;	//< 帧间延迟, 量纲: 秒
	int frmcnt;		//< 曝光帧数
	string filter;	//< 滤光片名称

public:
	ascii_proto_takeimg() {
		type = "take_image";
		iimgtype = IMGTYPE_ERROR;
		expdur = delay = 0.0;
		frmcnt = 0;
	}

	/*!
	 * @brief 检查/修正图像类型
	 */
	bool check_imgtype() {
		bool isValid(true);

		if (boost::iequals(imgtype, "bias")) {
			iimgtype = IMGTYPE_BIAS;
			imgtypeabbr = "bias";
			expdur = delay = 0.0;
		}
		else if (boost::iequals(imgtype, "dark")) {
			iimgtype = IMGTYPE_DARK;
			imgtypeabbr = "dark";
			delay = 0.0;
		}
		else if (boost::iequals(imgtype, "flat")) {
			iimgtype = IMGTYPE_FLAT;
			imgtypeabbr = "flat";
			delay = 0.0;
		}
		else if (boost::iequals(imgtype, "object")) {
			iimgtype = IMGTYPE_OBJECT;
			imgtypeabbr = "objt";
		}
		else if (boost::iequals(imgtype, "focus")) {
			iimgtype = IMGTYPE_FOCUS;
			imgtypeabbr = "focs";
		}
		else {
			iimgtype = IMGTYPE_ERROR;
			isValid = false;
		}
		return isValid;
	}
};
typedef boost::shared_ptr<ascii_proto_takeimg> aptakeimg;
extern aptakeimg makeap_takeimg();

struct ascii_proto_abortimg : public ascii_proto_base {// 中止采集图像

public:
	ascii_proto_abortimg() {
		type = "abort_image";
	}
};
typedef boost::shared_ptr<ascii_proto_abortimg> apabortimg;
extern apabortimg makeap_abortimg();

/* 相机 -- 底层 */
struct ascii_proto_object : public ascii_proto_base {// 目标信息与曝光参数
	int op_sn;		//< 计划编号, [0, INT_MAX - 1). INT_MAX保留用于手动
	string op_time;	//< 计划生成时间
	string op_type;	//< 计划类型
	string observer;	//< 观测者
	string obstype;	//< 观测类型
	string grid_id;	//< 天区划分模式
	string field_id;//< 天区编号
	string obj_id;	//< 目标名
	double ra;		//< 指向赤经, 量纲: 角度
	double dc;		//< 指向赤纬, 量纲: 角度
	double epoch;	//< 指向坐标系
	double objra;	//< 目标赤经, 量纲: 角度
	double objdc;	//< 目标赤纬, 量纲: 角度
	double objepoch;//< 目标坐标系
	string objerror;	//< 位置误差
	string imgtype;	//< 图像类型
	int iimgtype;	//< 图像类型
	string imgtypeabbr;	//< 图像类型缩写
	double expdur;	//< 曝光时间, 量纲: 秒
	double delay;	//< 帧间延迟, 量纲: 秒
	int frmcnt;		//< 曝光帧数
	int priority;	//< 优先级
	string tmbegin;	//< 曝光起始时间, 格式: YYYYMMDDThhmmss.sss
	string tmend;	//< 曝光结束时间
	int pair_id;		//< 分组编号
	string filter;	//< 滤光片名称

public:
	ascii_proto_object() {
		type = "object";
		op_sn = -1;
		imgtype = "unknown";
		iimgtype = IMGTYPE_ERROR;
		ra = dc = epoch = 1E30;
		objra = objdc = objepoch = 1E30;
		expdur = delay = 0.0;
		frmcnt = 0;
		priority = 0;
		pair_id = -1;
	}

	/*!
	 * @brief 检查/修正图像类型
	 */
	bool check_imgtype() {
		bool isValid(true);

		if (boost::iequals(imgtype, "bias")) {
			iimgtype = IMGTYPE_BIAS;
			imgtypeabbr = "bias";
			expdur = delay = 0.0;
		}
		else if (boost::iequals(imgtype, "dark")) {
			iimgtype = IMGTYPE_DARK;
			imgtypeabbr = "dark";
			delay = 0.0;
		}
		else if (boost::iequals(imgtype, "flat")) {
			iimgtype = IMGTYPE_FLAT;
			imgtypeabbr = "flat";
			delay = 0.0;
		}
		else if (boost::iequals(imgtype, "object")) {
			iimgtype = IMGTYPE_OBJECT;
			imgtypeabbr = "objt";
		}
		else if (boost::iequals(imgtype, "focus")) {
			iimgtype = IMGTYPE_FOCUS;
			imgtypeabbr = "focs";
		}
		else {
			iimgtype = IMGTYPE_ERROR;
			isValid = false;
		}
		return isValid;
	}
};
typedef boost::shared_ptr<ascii_proto_object> apobject;
extern apobject makeap_object();

struct ascii_proto_expose : public ascii_proto_base {// 曝光指令
	int command; // 控制指令

public:
	ascii_proto_expose() {
		type = "expose";
		command = EXPOSE_ERROR;
	}
};
typedef boost::shared_ptr<ascii_proto_expose> apexpose;
extern apexpose makeap_expose();

struct ascii_proto_camera_info : public ascii_proto_base {// 相机信息
	uint32_t bitdepth;	//< A/D带宽
	uint32_t readport;	//< 读出端口
	uint32_t readrate;	//< 读出速度
	uint32_t vrate;		//< 转移速度
	uint32_t gain;		//< 增益
	bool connected;		//< 连接标志
	int state;			//< 工作状态
	int errcode;			//< 错误代码
	double coolget;		//< 探测器温度, 量纲: 摄氏度
	double coolset;		//< 制冷温度, 量纲: 摄氏度
	string filter;		//< 滤光片
	int frmno;			//< 帧编号
	double freedisk;		//< 可用磁盘容量, 量纲: GB

public:
	ascii_proto_camera_info() {
		type = "camera_info";
		bitdepth = 16;
		readport = 0;
		readrate = 0;
		readport = 0;
		vrate = 0;
		gain = 0;
		connected = false;
		state = CAMCTL_ERROR;
		errcode = 0;
		coolget = 1E30;
		coolset = 1E30;
		frmno = -1;
		freedisk = 0.0;
	}
};
typedef boost::shared_ptr<ascii_proto_camera_info> apnfcam;
extern apnfcam makeap_camerainfo();

/* GWAC相机辅助程序通信协议: 温度和真空度 */
struct ascii_proto_cooler : public ascii_proto_base {// 温控参数
	double voltage;	//< 工作电压.   量纲: V
	double current;	//< 工作电流.   量纲: A
	double hotend;	//< 热端温度.   量纲: 摄氏度
	double coolget;	//< 探测器温度. 量纲: 摄氏度
	double coolset;	//< 制冷温度.   量纲: 摄氏度

public:
	ascii_proto_cooler() {
		type = "cooler";
		voltage = current = hotend = coolget = coolset = 1E30;
	}
};
typedef boost::shared_ptr<ascii_proto_cooler> apcooler;
extern apcooler makeap_cooler();

struct ascii_proto_vacuum : public ascii_proto_base {// 真空度参数
	double voltage;	//< 工作电压.   量纲: V
	double current;	//< 工作电流.   量纲: A
	string pressure;	//< 气压

public:
	ascii_proto_vacuum() {
		type = "vacuum";
		voltage = current = 1E30;
	}
};
typedef boost::shared_ptr<ascii_proto_vacuum> apvacuum;
extern apvacuum makeap_vacuum();

/* FITS文件传输 */
struct ascii_proto_fileinfo : public ascii_proto_base {// 文件描述信息, 客户端=>服务器
	string grid;			//< 天区划分模式
	string field;		//< 天区编号
	string tmobs;		//< 观测时间
	string subpath;		//< 子目录名称
	string filename;		//< 文件名称
	int filesize;		//< 文件大小, 量纲: 字节

public:
	ascii_proto_fileinfo() {
		type = "fileinfo";
		filesize = 0;
	}
};
typedef boost::shared_ptr<ascii_proto_fileinfo> apfileinfo;
extern apfileinfo makeap_fileinfo();

struct ascii_proto_filestat : public  ascii_proto_base {// 文件传输结果, 服务器=>客户端
	/*!
	 * @member status 文件传输结果
	 * - 1: 服务器完成准备, 通知客户端可以发送文件数据
	 * - 2: 服务器完成接收, 通知客户端可以发送其它文件
	 * - 3: 文件接收错误
	 */
	int status;	//< 文件传输结果

public:
	ascii_proto_filestat() {
		type = "filestat";
		status = 0;
	}
};
typedef boost::shared_ptr<ascii_proto_filestat> apfilestat;
extern apfilestat makeap_filestat();

//////////////////////////////////////////////////////////////////////////////
/*!
 * @class AsciiProtocol 通信协议操作接口, 封装协议解析与构建过程
 */
class AsciiProtocol {
public:
	AsciiProtocol();
	virtual ~AsciiProtocol();

public:
	/* 数据类型 */
	typedef boost::unique_lock<boost::mutex> mutex_lock;	//< 互斥锁
	typedef boost::shared_array<char> charray;	//< 字符数组
	typedef list<string> listring;	//< string列表

protected:
	/* 成员变量 */
	boost::mutex mtx_;	//< 互斥锁
	int ibuf_;			//< 存储区索引
	charray buff_;		//< 存储区

protected:
	/*!
	 * @brief 输出编码后字符串
	 * @param compacted 已编码字符串
	 * @param n         输出字符串长度, 量纲: 字节
	 * @return
	 * 编码后字符串
	 */
	const char* output_compacted(string& output, int& n);
	/*!
	 * @brief 连接关键字和对应数值, 并将键值对加入output末尾
	 * @param output   输出字符串
	 * @param keyword  关键字
	 * @param value    非字符串型数值
	 */
	template <class T1, class T2>
	void join_kv(string& output, T1& keyword, T2& value) {
		boost::format fmt("%1%=%2%,");
		fmt % keyword % value;
		output += fmt.str();
	}
	/*!
	 * @brief 解析关键字和对应数值
	 * @param kv       keyword=value对
	 * @param keyword  关键字
	 * @param value    对应数值
	 * @return
	 * 关键字和数值非空
	 */
	bool resolve_kv(string& kv, string& keyword, string& value);

public:
	/*---------------- 封装通信协议 ----------------*/
	/* 注册设备与注册结果 */
	/**
	 * @note 协议封装说明
	 * 输入参数: 结构体形式协议
	 * 输出按时: 封装后字符串, 封装后字符串长度
	 */
	/**
	 * @brief 封装设备注册和注册结果
	 */
	const char *CompactRegister(apreg proto, int &n);
	/* 望远镜 */
	/**
	 * @brief 封装搜索零点指令
	 */
	const char *CompactFindHome(apfindhome proto, int &n);
	/**
	 * @brief 封装同步零点指令
	 */
	const char *CompactHomeSync(aphomesync proto, int &n);
	/**
	 * @brief 封装指向指令
	 */
	const char *CompactSlewto(apslewto proto, int &n);
	/**
	 * @brief 封装复位指令
	 */
	const char *CompactPark(appark proto, int &n);
	/**
	 * @brief 封装导星指令
	 */
	const char *CompactGuide(apguide proto, int &n);
	/**
	 * @brief 封装中止指向指令
	 */
	const char *CompactAbortSlew(apabortslew proto, int &n);
	/**
	 * @brief 封装望远镜实时信息
	 */
	const char *CompactTelescopeInfo(apnftele proto, int &n);
	/**
	 * @brief 封装半高全宽指令和数据
	 */
	const char *CompactFWHM(apfwhm proto, int &n);
	/**
	 * @brief 封装调焦指令和数据
	 */
	const char *CompactFocus(apfocus proto, int &n);
	/**
	 * @brief 封装同步调焦器零点指令
	 */
	const char *CompactFocusSync(apfocusync proto, int &n);
	/**
	 * @brief 封装镜盖指令和状态
	 */
	const char *CompactMirrorCover(apmcover proto, int &n);
	/**
	 * @brief 封装目标信息, 用于写入FITS头
	 */
	const char *CompactObject(apobject proto, int &n);
	/**
	 * @brief 封装曝光指令
	 */
	const char *CompactExpose(apexpose proto, int &n);
	/**
	 * @brief 封装相机实时信息
	 */
	const char *CompactCameraInfo(apnfcam proto, int &n);

	/* GWAC相机辅助程序通信协议: 温度和真空度 */
	/*!
	 * @brief 封装温控信息
	 */
	const char *CompactCooler(apcooler proto, int &n);
	/*!
	 * @brief 封装真空度信息
	 */
	const char *CompactVacuum(apvacuum proto, int &n);
	/* FITS文件传输 */
	/*!
	 * @brief 封装文件描述信息
	 */
	const char *CompactFileInfo(apfileinfo proto, int &n);
	/*!
	 * @brief 封装文件传输结果
	 */
	const char *CompactFileStat(apfilestat proto, int &n);

public:
	/*---------------- 解析通信协议 ----------------*/
	/*!
	 * @brief 解析字符串生成结构化通信协议
	 * @param rcvd 待解析字符串
	 * @return
	 * 统一转换为apbase类型
	 */
	apbase Resolve(const char *rcvd);

protected:
	/*---------------- 解析通信协议 ----------------*/
	/**
	 * @note 协议解析说明
	 * 输入参数: 构成协议的字符串, 以逗号为分隔符解析后的keyword=value字符串组
	 * 输出参数: 转换为apbase类型的协议体. 当其指针为空时, 代表字符串不符合规范
	 */
	/**
	 * @brief 注册设备与注册结果
	 * */
	apbase resolve_register(listring &tokens);
	/**
	 * @brief 搜索零点
	 */
	apbase resolve_findhome(listring &tokens);
	/**
	 * @brief 同步零点, 修正转台零点偏差
	 */
	apbase resolve_homesync(listring &tokens);
	/**
	 * @brief 指向赤道坐标, 到位后保持恒动跟踪
	 */
	apbase resolve_slewto(listring &tokens);
	/**
	 * @brief 复位至安全位置, 到位后保持静止
	 */
	apbase resolve_park(listring &tokens);
	/**
	 * @brief 导星, 微量修正当前指向位置
	 */
	apbase resolve_guide(listring &tokens);
	/**
	 * @brief 中止指向过程
	 */
	apbase resolve_abortslew(listring &tokens);
	/**
	 * @brief 望远镜实时信息
	 */
	apbase resolve_telescopeinfo(listring &tokens);
	/**
	 * @brief 星象半高全宽
	 */
	apbase resolve_fwhm(listring &tokens);
	/**
	 * @brief 调焦器位置
	 */
	apbase resolve_focus(listring &tokens);
	/**
	 * @brief 同步调焦器零点, 修正调焦器零点
	 */
	apbase resolve_focusync(listring &tokens);
	/**
	 * @brief 镜盖指令与状态
	 */
	apbase resolve_mcover(listring &tokens);
	/**
	 * @brief 手动曝光指令
	 */
	apbase resolve_takeimg(listring &tokens);
	/**
	 * @brief 手动停止曝光指令
	 */
	apbase resolve_abortimg(listring &tokens);
	/**
	 * @brief 观测目标描述信息
	 */
	apbase resolve_object(listring &tokens);
	/**
	 * @brief 曝光指令
	 */
	apbase resolve_expose(listring &tokens);
	/**
	 * @brief 相机实时信息
	 */
	apbase resolve_camerainfo(listring &tokens);
	/**
	 * @brief 温控信息
	 */
	apbase resolve_cooler(listring& tokens);
	/**
	 * @brief 真空度信息
	 */
	apbase resolve_vacuum(listring& tokens);
	/**
	 * @brief FITS文件描述信息
	 */
	apbase resolve_fileinfo(listring& tokens);
	/**
	 * @brief FITS文件传输结果
	 */
	apbase resolve_filestat(listring& tokens);
};

typedef boost::shared_ptr<AsciiProtocol> AscProtoPtr;
extern AscProtoPtr make_ascproto();
//////////////////////////////////////////////////////////////////////////////

#endif /* ASCIIPROTOCOL_H_ */
