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
#include <boost/format.hpp>

#include <iostream>
using namespace std;

using std::string;
using std::vector;

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
	// 滤光片
	bool	bFilter;	//< 启用滤光片
	int		nFilter;	//< 滤光片插槽数量
	vector<string> nameFilter;	//< 各槽位滤光片名称
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

		// 平场阈值
		ptree &node3 = pt.add("AutoFlat", "");
		node3.add("Threshold.<xmlattr>.Low",    tLow    = 15000.0);
		node3.add("Threshold.<xmlattr>.High",   tHigh   = 40000.0);
		node3.add("Threshold.<xmlattr>.Expect", tExpect = 30000.0);
		node3.add("Exptime.<xmlattr>.Min",      edMin   =  2.0);
		node3.add("Exptime.<xmlattr>.Max",      edMax   = 15.0);

		// 总控服务器
		pt.add("GeneralControlServer.<xmlattr>.IP",   hostGC = "172.28.1.11");
		pt.add("GeneralControlServer.<xmlattr>.Port", portGC = 4013);

		// NTP服务器
		pt.add("NTPServer.<xmlattr>.Enable",       bNTP    = true);
		pt.add("NTPServer.<xmlattr>.IP",           hostNTP = "172.28.1.3");
		pt.add("NTPServer.<xmlattr>.MaxClockDiff", maxClockDiff = 10);

		// 文件服务器(==数据处理机)
		pt.add("FileServer.<xmlattr>.Enable",   bFileSrv    = false);
		pt.add("FileServer.<xmlattr>.IP",       hostFileSrv = "172.28.2.11");
		pt.add("FileServer.<xmlattr>.Port",     portFileSrv = 4015);

		// 本地文件存储路径
		ptree &node4 = pt.add("LocalStorage", "");
		node4.add("PathName",        pathLocal = "/data");
		node4.add("AutoFree.<xmlattr>.Enable",      bAutoFree = true);
		node4.add("AutoFree.<xmlattr>.MinCapacity", minDiskFree = 100);
		node4.add("<xmlcomment>", "Disk capacity unit is GB");

		// 滤光片
		nameFilter.clear();
		ptree &node5 = pt.add("Filter", "");
		node5.add("Enable", bFilter = false);
		node5.add("Number", nFilter = 5);
		node5.add("#1.<xmlattr>.Name", "U");
		node5.add("#2.<xmlattr>.Name", "B");
		node5.add("#3.<xmlattr>.Name", "V");
		node5.add("#4.<xmlattr>.Name", "R");
		node5.add("#5.<xmlattr>.Name", "I");
		string filter1 = "U";
		string filter2 = "B";
		string filter3 = "V";
		string filter4 = "R";
		string filter5 = "I";
		nameFilter.push_back(filter1);
		nameFilter.push_back(filter2);
		nameFilter.push_back(filter3);
		nameFilter.push_back(filter4);
		nameFilter.push_back(filter5);

		// 显示图像
		pt.add("DisplayImage.<xmlattr>.Enable", bShowImg = false);

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

			// 平场
			tLow    = pt.get("AutoFlat.Threshold.<xmlattr>.Low",    15000.0);
			tHigh   = pt.get("AutoFlat.Threshold.<xmlattr>.High",   40000.0);
			tExpect = pt.get("AutoFlat.Threshold.<xmlattr>.Expect", 30000.0);
			edMin   = pt.get("AutoFlat.Exptime.<xmlattr>.Min",      2.0);
			edMax   = pt.get("AutoFlat.Exptime.<xmlattr>.Max",     15.0);

			// 总控服务器
			hostGC = pt.get("GeneralControlServer.<xmlattr>.IP",   "172.28.1.11");
			portGC = pt.get("GeneralControlServer.<xmlattr>.Port", 4013);

			// NTP服务器
			bNTP         = pt.get("NTPServer.<xmlattr>.Enable",       true);
			hostNTP      = pt.get("NTPServer.<xmlattr>.IP",           "172.28.1.3");
			maxClockDiff = pt.get("NTPServer.<xmlattr>.MaxClockDiff", 10);

			// 文件服务器(==数据处理机)
			bFileSrv    = pt.get("FileServer.<xmlattr>.Enable", true);
			hostFileSrv = pt.get("FileServer.<xmlattr>.IP",     "172.28.2.11");
			portFileSrv = pt.get("FileServer.<xmlattr>.Port",   4015);

			// 本地文件存储路径
			pathLocal   = pt.get("LocalStorage.PathName",        "/data");
			bAutoFree   = pt.get("LocalStorage.AutoFree.<xmlattr>.Enable",    false);
			minDiskFree = pt.get("LocalStorage.AutoFree.<xmlattr>.MinCapacity", 100);

			// 滤光片
			nameFilter.clear();
			bFilter = pt.get("Filter.Enable", false);
			nFilter = pt.get("Filter.Number", 5);
			boost::format fmt("Filter.#%d.<xmlattr>.Name");
			for (int i = 1; i <= nFilter; ++i) {
				string name;
				fmt % i;
				name = pt.get(fmt.str(), "null");
				nameFilter.push_back(name);
			}

			// 显示图像
			bShowImg = pt.get("DisplayImage.<xmlattr>.Enable", false);
		}
		catch(boost::property_tree::xml_parser_error &ex) {
			InitFile(filepath);
		}
	}
};

#endif /* PARAMETER_H_ */
