/*
 * @brief parameter.h 维护管理系统参数
 * @author       卢晓猛
 * @description  维护管理系统配置参数
 * @version      1.0
 * @date         2016年12月21日
 * ===========================================================================
 * @date 2017年5月16日
 * - 使用XML文件替代INI文件
 */
#ifndef PARAMETER_H_
#define PARAMETER_H_

#include <string>
#include <vector>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>

using std::string;

struct param_config {// 软件配置参数
	// 观测系统标志
	string	group_id;	//< 组标志
	string	unit_id;	//< 单元标志
	string	camera_id;	//< 相机标志
	string	termType;	//< 终端类型
	// 相机参数
	int		camType;		//< 相机类型
	string	camIP;			//< 相机IP: 通过网络连接的相机
	int		readport;		//< 档位: 读出端口
	int		readrate;		//< 档位: 读出速度
	int		gain;			//< 档位: 增益
	double	coolerset;		//< 制冷温度
	double	tsaturate;		//< 无饱和抑制功能相机, 当饱和时数据将反转. 当图像数据统计结果小于该值时, 表明图像出现饱和
	// ROI区
	int		xstart, ystart;	//< 左上角在全幅图中的位置, 原点: (1,1)
	int		wroi, hroi;		//< 区域在全幅图中的对应尺寸, 量纲: 像素
	int		xbin, ybin;		//< XY方向合并因子
	// 平场参数
	double	tLow, tHigh, tExpect;	//< 平场均值阈值
	double	edMin, edMax;			//< 曝光时间限制
	// 总控服务器
	string	hostGC;	//< 总控服务器IP地址
	int		portGC;	//< 总控服务器对相机的服务端口
	// NTP服务器
	bool	bNTP;			//< 启用NTP时钟同步
	string	hostNTP;		//< NTP服务器地址
	int		maxClockDiff;	//< 本地时钟最大允许偏差
	// 文件服务器, 即数据处理机
	bool	bFileSrv;		//< 启用文件服务器
	string	hostFileSrv;	//< 文件服务器IP地址
	int		portFileSrv;	//< 文件服务器服务端口
	/* 文件本地存储根目录
	 * 在正午自动检测磁盘剩余空间, 并在空间不足时清除历史数据
	 */
	string	pathLocal;	//< FITS文件存储根目录
	bool	bAutoFree;	//< 自动清理磁盘空间
	int		minDiskFree;//< 最小磁盘可用空间, 量纲: GB
	// 显示图像参数(ds9)
	bool	bShowImg;	//< 是否实时显示图像

public:
	void InitFile(const string &filepath) {// 初始化配置文件
		using namespace boost::posix_time;
		using boost::property_tree::ptree;

		ptree pt;
		// 文件版本与初始日期
		pt.add("version", "0.8");
		pt.add("date", to_iso_extended_string(second_clock::universal_time()));
		// 观测系统参数
		ptree &node1 = pt.add("ObservationSystem", "");
		node1.add("ID.<xmlattr>.Group",      group_id  = "001");
		node1.add("ID.<xmlattr>.Unit",       unit_id   = "001");
		node1.add("ID.<xmlattr>.Camera",     camera_id = "001");
		node1.add("Terminal.<xmlattr>.Type", termType = "JFoV");

		// 相机参数
		ptree &node2 = pt.add("Camera", "");
		node2.add("<xmlcomment>", "Type#1: Andor CCD");
		node2.add("<xmlcomment>", "Type#2: FLI CCD");
		node2.add("<xmlcomment>", "Type#3: Apogee CCD");
		node2.add("<xmlcomment>", "Type#4: PI CCD");
		node2.add("<xmlcomment>", "Type#5: GWAC-GY CCD");
		node2.add("Type",              camType   = 5);
		node2.add("IP",                camIP     = "172.28.4.11");
		node2.add("ReadPort",          readport  = 0);
		node2.add("ReadRate",          readrate  = 0);
		node2.add("Gain",              gain      = 0);
		node2.add("CoolerSet",         coolerset = -20.0);
		node2.add("<xmlcomment>", "ReverseSaturation: reverse ADU being saturated without anti-blooming capability");
		node2.add("ReverseSaturation", tsaturate = 500.0);

		// ROI
		ptree &node3 = pt.add("RegionOfInterest", "");
		node3.add("StartX", xstart = 1);
		node3.add("StartY", ystart = 1);
		node3.add("Width",  wroi   = -1);
		node3.add("Height", hroi   = -1);
		node3.add("BinX",   xbin   = 1);
		node3.add("BinY",   ybin   = 1);

		// 平场阈值
		ptree &node4 = pt.add("AutoFlat", "");
		node4.add("LowerThreshold",  tLow    = 15000.0);
		node4.add("HigherThreshold", tHigh   = 40000.0);
		node4.add("ExpectThreshold", tExpect = 30000.0);
		node4.add("MinimumExptime",  edMin   =  2.0);
		node4.add("MaximumExptime",  edMax   = 15.0);

		// 总控服务器
		ptree &node5 = pt.add("GeneralControlServer", "");
		node5.add("IP",   hostGC = "172.28.1.11");
		node5.add("Port", portGC = 4013);

		// NTP服务器
		ptree &node6 = pt.add("NTPServer", "");
		node6.add("Enable",                 bNTP    = true);
		node6.add("IP",                     hostNTP = "172.28.1.3");
		node6.add("MaximumClockDifference", maxClockDiff = 5);

		// 文件服务器(==数据处理机)
		ptree &node7 = pt.add("FileServer", "");
		node7.add("Enable",   bFileSrv    = false);
		node7.add("IP",       hostFileSrv = "172.28.2.11");
		node7.add("Port",     portFileSrv = 4015);

		// 本地文件存储路径
		ptree &node8 = pt.add("LocalStorage", "");
		node8.add("PathName",        pathLocal = "/data");
		node8.add("AutoFreeDisk",    bAutoFree = false);
		node8.add("MinimumFreeDisk", minDiskFree = 100);
		node8.add("<xmlcomment>", "Disk capacity unit is GB");

		// 显示图像
		ptree &node9 = pt.add("DisplayImage", "");
		node9.add("Enable", bShowImg = false);

		boost::property_tree::xml_writer_settings<string> settings(' ', 4);
		write_xml(filepath, pt, std::locale(), settings);
	}

	void LoadFile(const string &filepath) {// 加载配置文件
		try {
			using boost::property_tree::ptree;

			ptree pt;
			read_xml(filepath, pt, boost::property_tree::xml_parser::trim_whitespace);

			// 观测系统参数
			group_id  = pt.get("ObservationSystem.ID.<xmlattr>.Group",      "001");
			unit_id   = pt.get("ObservationSystem.ID.<xmlattr>.Unit",       "001");
			camera_id = pt.get("ObservationSystem.ID.<xmlattr>.Camera",     "001");
			termType  = pt.get("ObservationSystem.Terminal.<xmlattr>.Type", "JFoV");

			// 相机参数
			camType   = pt.get("Camera.Type",               5);
			camIP     = pt.get("Camera.IP",                 "172.28.4.11");
			readport  = pt.get("Camera.ReadPort",           0);
			readrate  = pt.get("Camera.ReadRate",           0);
			gain      = pt.get("Camera.Gain",               0);
			coolerset = pt.get("Camera.CoolerSet",         -20.0);
			tsaturate = pt.get("Camera.ReverseSaturation",  500.0);

			// ROI
			xstart = pt.get("RegionOfInterest.StartX",  1);
			ystart = pt.get("RegionOfInterest.StartY",  1);
			wroi   = pt.get("RegionOfInterest.Width",  -1);
			hroi   = pt.get("RegionOfInterest.Height", -1);
			xbin   = pt.get("RegionOfInterest.BinX",    1);
			ybin   = pt.get("RegionOfInterest.BinY",    1);

			// 平场
			tLow    = pt.get("AutoFlat.LowerThreshold",  15000.0);
			tHigh   = pt.get("AutoFlat.HigherThreshold", 40000.0);
			tExpect = pt.get("AutoFlat.ExpectThreshold", 30000.0);
			edMin   = pt.get("AutoFlat.MinimumExptime",      2.0);
			edMax   = pt.get("AutoFlat.MaximumExptime",     15.0);

			// 总控服务器
			hostGC = pt.get("GeneralControlServer.IP", "172.28.1.11");
			portGC = pt.get("GeneralControlServer.IP", 4013);

			// NTP服务器
			bNTP         = pt.get("NTPServer.Enable",                 true);
			hostNTP      = pt.get("NTPServer.IP",                     "172.28.1.3");
			maxClockDiff = pt.get("NTPServer.MaximumClockDifference", 5);

			// 文件服务器(==数据处理机)
			bFileSrv    = pt.get("FileServer.Enable", true);
			hostFileSrv = pt.get("FileServer.IP",     "172.28.2.11");
			portFileSrv = pt.get("FileServer.Port",   4015);

			// 本地文件存储路径
			pathLocal   = pt.get("LocalStorage.PathName",        "/data");
			bAutoFree   = pt.get("LocalStorage.AutoFreeDisk",    false);
			minDiskFree = pt.get("LocalStorage.MinimumFreeDisk", 100);

			// 显示图像
			bShowImg = pt.get("DisplayImage.Enable", false);
		}
		catch(boost::property_tree::xml_parser_error &ex) {
			InitFile(filepath);
		}
	}
};

#endif /* PARAMETER_H_ */
