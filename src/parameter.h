/*
 * @file parameter.h 配置文件声明文件
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
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using std::string;
using std::vector;
typedef vector<string> strvec;

/*!
 * @struct 管理配置参数
 */
struct config_parameter {
private:
	/* 相机 */
	string termType;	//< 终端类型
	int    camType;		//< 相机型号
	string camIP;		//< 相机IP地址
	int    readport;	//< 读出端口
	int    readrate;	//< 读出速度
	int    gain;		//< 增益
	bool   bEM;			//< 启用EM
	int    gainEM;		//< EM增益
	double coolerset;	//< 制冷温度
	double tsaturate;	//< 饱和反转值. 适用于不支持饱和抑制类型
	/* 滤光片 */
	bool   bFilter;		//< 是否支持滤光片
	int    nFilter;		//< 滤光片控制器插槽数量
	strvec sFilter;		//< 各槽位滤光片通用名称
	/* 本地文件存储格式 */
	string pathLocal;	//< 文件存储根路径
	bool   bFreeDisk;	//< 是否自动清理磁盘空间
	int    minDisk;		//< 最小可用磁盘空间, 量纲: GB
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
	/* 配置文件 */
	string filepath;	//< 配置文件存储路径
	bool   modified;	//< 参数修改标记
	boost::property_tree::ptree pt;	//< 参数树型结构

public:
	virtual ~config_parameter() {
		if (modified) {
			boost::property_tree::xml_writer_settings<string> settings(' ', 4);
			write_xml(filepath, pt, std::locale(), settings);
		}
	}

	/*!
	 * @brief 初始化配置文件
	 * @param filepath 文件存储路径
	 */
	void Init(const string &filepath) {
		using boost::property_tree::ptree;
		/* 初始化环境变量 */
		this->filepath = filepath;
		modified = true;
		pt.clear();
		/* 逐项生成参数 */
		// 相机
		ptree &camera = pt.add("Camera", "");
		camera.add("TerminalType", termType = "JFoV");
		camera.add("<xmlcomment>", "Type#1: Andor CCD"  );
		camera.add("<xmlcomment>", "Type#2: FLI CCD"    );
		camera.add("<xmlcomment>", "Type#3: Apogee CCD" );
		camera.add("<xmlcomment>", "Type#4: PI CCD"     );
		camera.add("<xmlcomment>", "Type#5: GWAC-GY CCD");
		camera.add("CameraType",   camType  = 5);
		camera.add("IPAddress",    camIP    = "172.28.4.11");
		camera.add("Setting.<xmlattr>.Readport", readport  = 0);
		camera.add("Setting.<xmlattr>.Readrate", readrate  = 0);
		camera.add("Setting.<xmlattr>.Gain",     gain      = 1);
		camera.add("CoolerTarget",        coolerset = -40.0);
		camera.add("ReverseSaturation",   tsaturate = 500.0);
		camera.add("EM.<xmlattr>.Enable", bEM       = false);
		camera.add("EM.<xmlattr>.Gain",   gainEM    = 10);
		// 滤光片
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

		strvec pathLocal;	//< 文件存储根路径
		bool   bFreeDisk;	//< 是否自动清理磁盘空间
		int    minDisk;		//< 最小可用磁盘空间, 量纲: GB
		/*!
		 * @member pathstyle 文件路径及文件名格式
		 * 1: GWAC格式
		 * 2: GFT格式
		 * 3: 50BiN格式
		 */
		int    pathstyle;
	}

	/*!
	 * @brief 加载配置文件
	 * @param filepath 文件存储路径
	 */
	void Load(const string &filepath) {
		using boost::property_tree::ptree;
		/* 初始化环境变量 */
		this->filepath = filepath;
		modified = false;
		pt.clear();
		/* 逐项加载参数 */
	}
};

#endif /* SRC_PARAMETER_H_ */
