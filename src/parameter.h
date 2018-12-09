/*
 * @file parameter.h 定义软件可配置参数
 * @version 1.0
 * @author 卢晓猛
 * @email lxm@nao.cas.cn
 * @note
 * 管理配置参数
 */

#ifndef SRC_PARAMETER_H_
#define SRC_PARAMETER_H_

#include <string>
#include <vector>
#include <iostream>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/format.hpp>

using std::string;
using std::vector;

/*!
 * @struct 管理配置参数
 */
struct Parameter {
	/* 相机 */
	string termType;	//< 终端类型
	int    camType;		//< 相机型号
	string camIP;		//< 相机IP地址
	int    ADChannel;	//< A/D通道
	int    readport;	//< 读出端口
	int    readrate;	//< 读出速度
	int    VSRate;		//< 行转移速度
	int    gain;		//< 增益
	bool   bEM;			//< 启用EM
	int    gainEM;		//< EM增益
	double coolerset;	//< 制冷温度
	double tsaturate;	//< 饱和反转值. 适用于不支持饱和抑制类型
	/* 滤光片 */
	bool   bFilter;		//< 是否支持滤光片
	int    nFilter;		//< 滤光片控制器插槽数量
	vector<string> sFilter;		//< 各槽位滤光片通用名称
	/* 本地文件存储格式 */
	string pathLocal;	//< 文件存储根路径
	bool   bFreeDisk;	//< 是否自动清理磁盘空间
	int    minFreeDisk;	//< 最小可用磁盘空间, 量纲: GB
	/*!
	 * @member pathstyle 文件路径及文件名格式
	 * 1: GWAC格式
	 * 2: GFT格式
	 * 3: 50BiN格式
	 */
	int    pathstyle;
	/* 图像显示标志 */
	bool   bShowImg;
	/* 自动平场参数 */
	double flatMin;		//< 有效平场最小值
	double flatMax;		//< 有效平场最大值
	double flatExpect;	//< 有效平场期望值
	double flatMinT;	//< 平场最短曝光时间
	double flatMaxT;	//< 平场最长曝光时间
	/* 网络标志 */
	string gid;			//< 组标志
	string uid;			//< 单元标志
	string cid;			//< 相机标志
	/* NTP守时服务器 */
	bool   bNTP;		//< 是否支持NTP守时
	string hostNTP;		//< NTP服务器IP地址
	int    maxClockDiff;//< 最大时钟偏差. 大于该值需要修正本机时钟
	/* 观测调度服务器 */
	string hostGC;		//< 调度服务器IP地址
	uint   portGC;		//< 调度服务器端口
	/* 文件服务器 */
	bool   bFile;		//< 是否支持文件上传
	string hostFile;	//< 文件服务器IP地址
	uint   portFile;	//< 文件服务器端口
	/* 版本更新 */
	bool   bUpdate;		//< 版本维护更新

public:
	/*!
	 * @brief 初始化配置文件
	 * @param filepath 文件存储路径
	 */
	void Init(const string &filepath) {
		using boost::property_tree::ptree;
		boost::property_tree::ptree pt;	//< 参数树型结构
		/* 逐项生成参数 */
		/* 相机 */
		ptree &camera = pt.add("Camera", "");
		camera.add("TerminalType", termType = "JFoV");
		camera.add("<xmlcomment>", "Camera Type#1: Andor CCD"  );
		camera.add("<xmlcomment>", "Camera Type#2: FLI CCD"    );
		camera.add("<xmlcomment>", "Camera Type#3: Apogee CCD" );
		camera.add("<xmlcomment>", "Camera Type#4: PI CCD"     );
		camera.add("<xmlcomment>", "Camera Type#5: GWAC-GY CCD");
		camera.add("CameraType",   camType  = 5);
		camera.add("IPAddress",    camIP    = "172.28.4.11");
		camera.add("Setting.<xmlattr>.ADChannel", ADChannel = 0);
		camera.add("Setting.<xmlattr>.Readport", readport  = 0);
		camera.add("Setting.<xmlattr>.Readrate", readrate  = 0);
		camera.add("Setting.<xmlattr>.VSRate",   VSRate  = 0);
		camera.add("Setting.<xmlattr>.Gain",     gain      = 1);
		camera.add("CoolerTarget",        coolerset = -40.0);
		camera.add("ReverseSaturation",   tsaturate = 500.0);
		camera.add("EM.<xmlattr>.Enable", bEM       = false);
		camera.add("EM.<xmlattr>.Gain",   gainEM    = 10);
		/* 滤光片 */
		sFilter.clear();
		ptree &filter = pt.add("Filter", "");
		filter.add("Setting.<xmlattr>.Enable",    bFilter = false);
		filter.add("Setting.<xmlattr>.SlotCount", nFilter = 5);
		filter.add("#1.<xmlattr>.Name", "U");
		filter.add("#2.<xmlattr>.Name", "B");
		filter.add("#3.<xmlattr>.Name", "V");
		filter.add("#4.<xmlattr>.Name", "R");
		filter.add("#5.<xmlattr>.Name", "I");

		sFilter.push_back("U");
		sFilter.push_back("B");
		sFilter.push_back("V");
		sFilter.push_back("R");
		sFilter.push_back("I");
		/* 本地文件存储格式 */
		ptree &fileloc = pt.add("LocalStorage", "");
		fileloc.add("PathRoot", pathLocal = "/data");
		fileloc.add("AutoFreeDisk.<xmlattr>.Enable",       bFreeDisk   = true);
		fileloc.add("AutoFreeDisk.<xmlattr>.MinFreeDisk",  minFreeDisk = 100);
		fileloc.add("FilenameStyle", pathstyle = 1);
		fileloc.add("<xmlcomment>", "Filename Style#1: GWAC" );
		fileloc.add("<xmlcomment>", "Filename Style#2: GFT"  );
		fileloc.add("<xmlcomment>", "Filename Style#3: 50BiN");
		/* 图像显示标志 */
		pt.add("ShowImage.<xmlattr>.Enable", bShowImg = false);
		/* 自动平场参数 */
		ptree &flat = pt.add("FlatField", "");
		flat.add("Threshold.<xmlattr>.Minimum",  flatMin    = 15000);
		flat.add("Threshold.<xmlattr>.Maximum",  flatMax    = 40000);
		flat.add("Threshold.<xmlattr>.Expect",   flatExpect = 30000);
		flat.add("Exposure.<xmlattr>.Minimum",   flatMinT   =  2.0);
		flat.add("Exposure.<xmlattr>.Maximum",   flatMaxT   = 15.0);
		/* 网络标志 */
		pt.add("NetworkID.<xmlattr>.Group",  gid = "001");
		pt.add("NetworkID.<xmlattr>.Unit",   uid = "001");
		pt.add("NetworkID.<xmlattr>.Camera", cid = "001");
		/* NTP守时服务器 */
		pt.add("NTP.<xmlattr>.Enable",       bNTP         = true);
		pt.add("NTP.<xmlattr>.IP",           hostNTP      = "172.28.1.3");
		pt.add("NTP.<xmlattr>.MaxClockDiff", maxClockDiff = 10);
		/* 观测调度服务器 */
		pt.add("DispatchServer.<xmlattr>.IP",   hostGC = "172.28.1.11");
		pt.add("DispatchServer.<xmlattr>.Port", portGC = 4013);
		/* 文件服务器 */
		pt.add("FileServer.<xmlattr>.Enable", bFile    = false);
		pt.add("FileServer.<xmlattr>.IP",     hostFile = "172.28.2.11");
		pt.add("FileServer.<xmlattr>.Port",   portFile = 4020);
		/* 版本维护更新 */
		pt.add("VersionUpdate.<xmlattr>.Enable", bUpdate = false);
		/* 输出xml文件 */
		boost::property_tree::xml_writer_settings<string> settings(' ', 4);
		write_xml(filepath, pt, std::locale(), settings);
	}

	/*!
	 * @brief 加载配置文件
	 * @param filepath 文件存储路径
	 */
	void Load(const string &filepath) {
		using boost::property_tree::ptree;
		boost::property_tree::ptree pt;	//< 参数树型结构

		read_xml(filepath, pt, boost::property_tree::xml_parser::trim_whitespace);
		/* 逐项加载参数 */
		/* 相机 */
		termType = pt.get("Camera.TerminalType", "JFoV");
		camType  = pt.get("Camera.CameraType",   5);
		camIP    = pt.get("Camera.IPAddress",    "172.28.4.11");
		ADChannel= pt.get("Camera.Setting.<xmlattr>.ADChannel", 0);
		readport = pt.get("Camera.Setting.<xmlattr>.Readport",  1);
		readrate = pt.get("Camera.Setting.<xmlattr>.Readrate",  2);
		VSRate   = pt.get("Camera.Setting.<xmlattr>.VSRate",    0);
		gain     = pt.get("Camera.Setting.<xmlattr>.Gain",      0);
		coolerset = pt.get("Camera.CoolerTarget",         -20.0);
		tsaturate = pt.get("Camera.ReverseSaturation",    600.0);
		bEM       = pt.get("Camera.EM.<xmlattr>.Enable",   true);
		gainEM    = pt.get("Camera.EM.<xmlattr>.Gain",        5);
		/* 滤光片 */
		boost::format fmt("Filter.#%d.<xmlattr>.Name");
		sFilter.clear();
		bFilter = pt.get("Filter.Setting.<xmlattr>.Enable",    true);
		nFilter = pt.get("Filter.Setting.<xmlattr>.SlotCount",   10);
		for (int i = 1; i <= nFilter; ++i) {
			fmt % i;
			string name = pt.get(fmt.str(), "");
			sFilter.push_back(name);
		}
		/* 本地文件存储格式 */
		pathLocal = pt.get("LocalStorage.PathRoot", "/data");
		bFreeDisk = pt.get("LocalStorage.AutoFreeDisk.<xmlattr>.Enable",       true);
		minFreeDisk = pt.get("LocalStorage.AutoFreeDisk.<xmlattr>.MinFreeDisk", 100);
		pathstyle   = pt.get("LocalStorage.FilenameStyle", 1);
		/* 图像显示标志 */
		bShowImg = pt.get("ShowImage.<xmlattr>.Enable", false);
		/* 自动平场参数 */
		flatMin    = pt.get("FlatField.Threshold.<xmlattr>.Minimum",  15000);
		flatMax    = pt.get("FlatField.Threshold.<xmlattr>.Maximum",  40000);
		flatExpect = pt.get("FlatField.Threshold.<xmlattr>.Expect",   30000);
		flatMinT   = pt.get("FlatField.Exposure.<xmlattr>.Minimum",     2.0);
		flatMaxT   = pt.get("FlatField.Exposure.<xmlattr>.Maximum",    15.0);
		/* 网络标志 */
		gid = pt.get("NetworkID.<xmlattr>.Group",  "001");
		uid = pt.get("NetworkID.<xmlattr>.Unit",   "001");
		cid = pt.get("NetworkID.<xmlattr>.Camera", "001");
		/* NTP守时服务器 */
		bNTP         = pt.get("NTP.<xmlattr>.Enable",      true);
		hostNTP      = pt.get("NTP.<xmlattr>.IP",           "172.28.1.3");
		maxClockDiff = pt.get("NTP.<xmlattr>.MaxClockDiff", 10);
		/* 观测调度服务器 */
		hostGC = pt.get("DispatchServer.<xmlattr>.IP",   "172.28.1.11");
		portGC = pt.get("DispatchServer.<xmlattr>.Port", 4013);
		/* 文件服务器 */
		bFile    = pt.get("FileServer.<xmlattr>.Enable", false);
		hostFile = pt.get("FileServer.<xmlattr>.IP",     "172.28.2.11");
		portFile = pt.get("FileServer.<xmlattr>.Port",   4020);
		/* 版本维护更新 */
		bUpdate = pt.get("VersionUpdate.<xmlattr>.Enable", false);
	}
};

#endif /* SRC_PARAMETER_H_ */
