/*
 * @file ConfigurationParameter.h 配置文件声明文件
 * @version 0.1
 * @author 卢晓猛
 * @email lxm@nao.cas.cn
 * @note
 * 管理配置参数
 */

#ifndef CONFIGURATIONPARAMETER_H_
#define CONFIGURATIONPARAMETER_H_

#include <string>
#include <vector>

using std::string;
using std::vector;

class ConfigurationParameter {
public:
	ConfigurationParameter(const string &filepath);
	virtual ~ConfigurationParameter();

public:
	/*!
	 * @brief 初始化配置参数
	 */
	void Init();
	/*!
	 * @brief 加载配置参数
	 */
	void Load();

public:
	// 数据类型
	typedef vector<string> strvector;

private:
	// 成员变量
	string filepath_;	//< 配置文件路径
	// 相机基本配置参数
	int camType_;		//< 相机类型
	string camIP_;		//< 相机IP地址
	int readport_;		//< 读出端口档位
	int readrate_;		//< 读出速度档位
	int gain_;			//< 增益档位
	int emgain_;		//< EM增益倍率
	double coolerset_;	//< 制冷温度
	double tsaturate_;	//< 饱和反转值. 当低于该值时, 认为是饱和反转
	// 相机网络标志
	string gid_, uid_, cid_;	//< 设备网络标志
	string termType_;			//< 终端类型
	// 自动平场调制参数
	double tLow_, tHigh_, tExpect_;	//< 平场均值阈值
	double etMin_, etMax_;			//< 平场曝光时间阈值
	// 文件存储路径
	string pathLoc_;	//< 文件存储根路径
	bool bAutoFree_;	//< 启用磁盘自动清理
	int minDiskFree_;	//< 最小可用磁盘空间, 量纲: GB
	// 滤光片配置参数
	bool bFilter_;		//< 启用滤光片
	int nFilter_;		//< 滤光片插槽数量
	strvector sFilter_;	//< 各槽位滤光片名称
	// 图像显示参数
	bool bShowImg_;		//< 是否实时显示图像
	// 观测调度服务器
	string hostGC_;		//< 服务器IP地址
	uint16_t portGC_;	//< 服务器端口
	// 文件服务器
	bool bFileSrv_;		//< 启用文件服务器
	string hostFileSrv_;	//< 文件服务器IP地址
	uint16_t portFileSrv_;	//< 文件服务器端口
	// NTP服务器
	bool bNTP_;		//< 启用NTP时钟同步
	string hostNTP_;	//< NTP服务器IP地址
	string maxClockDiff_;	//< 时钟修正阈值
};

#endif /* CONFIGURATIONPARAMETER_H_ */
