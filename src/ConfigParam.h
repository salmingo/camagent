/*!
 * @file ConfigParameter.h  配置参数声明文件
 * @author       卢晓猛
 * @note         配置参数访问接口
 * @version      1.0
 * @date         2019年6月30日
 * @note
 * 配置参数分为几类:
 *  1. 相机参数
 *  2. 独立制冷
 *  3. 滤光片参数
 *  4. 光学系统参数
 *  5. 本地文件管理
 *  6. 图像显示
 *  7. 平场
 *  8. 网络标志
 *  9. 总控服务器
 * 10. NTP服务器
 * 11. 文件服务器
 */
#ifndef CONFIG_PARAMETER_H_
#define CONFIG_PARAMETER_H_

#include <string>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using std::string;
using std::vector;

struct ConfigParameter {
	typedef vector<string> strvec;

	// 相机参数
	string termType;	//< 终端类型
	int camType;		//< 相机类型
	string camIP;		//< 相机IP地址
	int ADChannel;		//< A/D通道的档位
	int readport;		//< 读出端口的档位
	int readrate;		//< 读出速度的档位
	int gain;			//< 增益的档位
	int vsrate;			//< 行转移速度的档位
	bool emenable;		//< 启用EM模式
	int emgain;			//< EM增益
	int tsat;			//< 饱和反转值. 饱和反转后的数值
	// 相机制冷
	bool coolalone;		//< 独立控制制冷
	int coolset;		//< 制冷温度, 量纲: 摄氏度
	uint coolport;		//< 独立控制制冷时的TCP端口
	// 滤光片参数
	bool filenable;	//< 启用滤光片
	int filn;		//< 滤光片转轮数量
	strvec filname;	//< 各位置滤光片名称
	// 光学系统参数
	string telescope;	//< 望远镜名称
	uint aptdia;		//< 主镜口径, 量纲: 厘米
	string focus;		//< 焦点类型
	uint foclen;		//< 焦距, 量纲: 毫米
	// 本地文件管理
	string pathroot;	//< 本地文件存储目录名
	uint fdmin;			//< 最小可用硬盘空间, 量纲: GB
	// 图像是否显示
	bool imgshow;	//< 图像显示标记
	// 平场
	uint ffminv;	//< 有效平场最小统计值
	uint ffmaxv;	//< 有效平场最大统计值
	uint ffmint;	//< 有效平场最短曝光时间, 量纲: 秒
	uint ffmaxt;	//< 有效平场最长曝光时间, 量纲: 秒
	// 设备在网络中的标志
	string gid;		//< 组标志
	string uid;		//< 单元标志
	string cid;		//< 相机标志
	// 总控服务器
	string gcip;	//< IP地址
	uint gcport;	//< TCP端口
	// NTP服务器
	bool ntpenable;	//<　启用NTP服务
	string ntpip;	//< IP地址
	uint ntpdiff;	//< 本地时钟与NTP时钟的最大偏差, 量纲: 毫秒
	// 文件服务器
	bool fsenable;	//< 启用文件服务
	string fsip;	//< IP地址
	uint fsport;	//< TCP端口

public:
	void Init(const string &filepath) {
		using namespace boost::posix_time;
		namespace proptree = boost::property_tree;
		proptree::ptree pt;

		pt.add("date", to_iso_string(second_clock::universal_time()));

		// 相机参数
		proptree::ptree &node1 = pt.add("Camera", "");
		node1.add("Terminal.<xmlattr>.Type", "JFoV");
		node1.add("<xmlcomment>", "Camera Type#1: Andor CCD");
		node1.add("<xmlcomment>", "Camera Type#2: FLI CCD");
		node1.add("<xmlcomment>", "Camera Type#3: Apogee CCD");
		node1.add("<xmlcomment>", "Camera Type#4: PI CCD");
		node1.add("<xmlcomment>", "Camera Type#5: GWAC-GY CCD");
		node1.add("Camera.<xmlattr>.Type", 5);
		node1.add("IPAddress", "172.28.4.11");
		node1.add("Read.<xmlattr>.ADChannel", 0);
		node1.add("Read.<xmlattr>.Port", 0);
		node1.add("Read.<xmlattr>.Rate", 0);
		node1.add("Read.<xmlattr>.Gain", 0);
		node1.add("VerticalShift.<xmlattr>.Rate", 0);
		node1.add("EM.<xmlattr>.Enable", false);
		node1.add("EM.<xmlattr>.Gain", 10);
		node1.add("ReverseSaturation", 600);
		// 相机制冷
		pt.add("Cooler.<xmlattr>.Set", -40.0);
		pt.add("Cooler.<xmlattr>.Alone", false);
		pt.add("Cooler.<xmlattr>.Port", 4021);
		// 滤光片参数
		proptree::ptree &node2 = pt.add("Filter", "");
		node2.add("<xmlattr>.Enable", false);
		node2.add("<xmlattr>.SlotNumber", 5);
		node2.add("#1.<xmlattr>.Name", "U");
		node2.add("#2.<xmlattr>.Name", "B");
		node2.add("#3.<xmlattr>.Name", "V");
		node2.add("#4.<xmlattr>.Name", "R");
		node2.add("#5.<xmlattr>.Name", "I");
		// 光学系统参数
		proptree::ptree &node3 = pt.add("Optics", "");
		node3.add("<xmlattr>.Name",         "GWAC");
		node3.add("Diameter",               18);
		node3.add("Focus.<xmlattr>.Type",   "Refractor");
		node3.add("Focus.<xmlattr>.Length", 216);
		// 本地文件管理
		proptree::ptree &node4 = pt.add("LocalStorage", "");
		node4.add("PathRoot",         "/data");
		node4.add("FreeDiskCapacity", 100);
		// 图像是否显示
		pt.add("ShowImage.<xmlattr>.Enable", false);
		// 平场
		proptree::ptree &node5 = pt.add("FlatField", "");
		node5.add("StatADU.<xmlattr>.Min",  20000);
		node5.add("StatADU.<xmlattr>.Max",  45000);
		node5.add("Exposure.<xmlattr>.Min", 2);
		node5.add("Exposure.<xmlattr>.Max", 15);
		// 设备在网络中的标志
		pt.add("NetworkID.<xmlattr>.Group",  "001");
		pt.add("NetworkID.<xmlattr>.Unit",   "001");
		pt.add("NetworkID.<xmlattr>.Camera", "001");
		// 总控服务器
		pt.add("GeneralControl.<xmlattr>.IP",   "127.0.0.1");
		pt.add("GeneralControl.<xmlattr>.Port", 4013);
		// NTP服务器
		pt.add("NTP.<xmlattr>.Enable",       true);
		pt.add("NTP.<xmlattr>.IP",           "172.28.1.3");
		pt.add("NTP.<xmlattr>.MaxClockDiff", 100);
		// 文件服务器
		pt.add("FileServer.<xmlattr>.Enable",  true);
		pt.add("FileServer.<xmlattr>.IP",      "172.28.2.11");
		pt.add("FileServer.<xmlattr>.Port",    4020);

		proptree::xml_writer_settings<std::string> settings(' ', 4);
		write_xml(filepath, pt, std::locale(), settings);
	}

	bool Load(const string &filepath) {
		namespace proptree = boost::property_tree;
		try {
			proptree::ptree pt;
			read_xml(filepath, pt, proptree::xml_parser::trim_whitespace);

			BOOST_FOREACH(proptree::ptree::value_type const &child, pt.get_child("")) {
				if (boost::iequals(child.first, "Camera")) {
					termType  = child.second.get("Terminal.<xmlattr>.Type",      "JFoV");
					camType   = child.second.get("Camera.<xmlattr>.Type",        5);
					camIP     = child.second.get("IPAddress",                    "172.28.4.11");
					ADChannel = child.second.get("Read.<xmlattr>.ADChannel",     0);
					readport  = child.second.get("Read.<xmlattr>.Port",          0);
					readrate  = child.second.get("Read.<xmlattr>.Rate",          0);
					gain      = child.second.get("Read.<xmlattr>.Gain",          0);
					vsrate    = child.second.get("VerticalShift.<xmlattr>.Rate", 0);
					emenable  = child.second.get("EM.<xmlattr>.Enable",          false);
					emgain    = child.second.get("EM.<xmlattr>.Gain",            10);
					tsat      = child.second.get("ReverseSaturation",            600);
				}
				else if (boost::iequals(child.first, "Cooler")) { // 相机制冷
					coolalone = child.second.get("<xmlattr>.Alone", false);
					coolset   = child.second.get("<xmlattr>.Set",   -40.0);
					coolport  = child.second.get("<xmlattr>.Port",  4021);
				}
				else if (boost::iequals(child.first, "Filter")) {
					boost::format fmt("%d.<xmlattr>.Name");
					filenable = child.second.get("<xmlattr>.Enable",     false);
					filn      = child.second.get("<xmlattr>.SlotNumber", 5);
					for (int i = 1; i <= filn; ++i) {
						fmt % i;
						string name = child.second.get(fmt.str(), "");
						filname.push_back(name);
					}
				}
				else if (boost::iequals(child.first, "Optics")) {
					telescope = child.second.get("<xmlattr>.Name",         "GWAC");
					aptdia    = child.second.get("Diameter",               18);
					focus     = child.second.get("Focus.<xmlattr>.Type",   "Refractor");
					foclen    = child.second.get("Focus.<xmlattr>.Length", 216);
				}
				else if (boost::iequals(child.first, "LocalStorage")) {
					pathroot = child.second.get("PathRoot", "/data");
					fdmin = child.second.get("FreeDiskCapacity", 100);
				}
				else if (boost::iequals(child.first, "FlatField")) {
					ffminv = child.second.get("StatADU.<xmlattr>.Min", 20000);
					ffmaxv = child.second.get("StatADU.<xmlattr>.Max", 45000);
					ffmint = child.second.get("Exposure.<xmlattr>.Min", 2);
					ffmaxt = child.second.get("Exposure.<xmlattr>.Max", 15);
				}
				else if (boost::iequals(child.first, "NetworkID")) {
					gid = child.second.get("<xmlattr>.Group",  "001");
					uid = child.second.get("<xmlattr>.Unit",   "001");
					cid = child.second.get("<xmlattr>.Camera", "001");
				}
				else if (boost::iequals(child.first, "GeneralControl")) {
					gcip   = child.second.get("<xmlattr>.IP",   "127.0.0.1");
					gcport = child.second.get("<xmlattr>.Port", 4013);
				}
				else if (boost::iequals(child.first, "NTP")) {
					ntpenable = child.second.get("<xmlattr>.Enable",       true);
					ntpip     = child.second.get("<xmlattr>.IP",           "172.28.1.3");
					ntpdiff   = child.second.get("<xmlattr>.MaxClockDiff", 100);
				}
				else if (boost::iequals(child.first, "FileServer")) {
					fsenable = child.second.get("<xmlattr>.Enable", true);
					fsip     = child.second.get("<xmlattr>.IP",     "172.28.2.11");
					fsport   = child.second.get("<xmlattr>.Port",   4020);
				}
				else if (boost::iequals(child.first, "ShowImage")) {
					imgshow = child.second.get("<xmlattr>.Enable", false);
				}
			}

			return true;
		} catch (proptree::xml_parser_error &ex) {
			Init(filepath);
			return false;
		}
	}
};

#endif
