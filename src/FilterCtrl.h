/*
 * @file FilterCtrl.h 声明滤光片控制接口
 * @version 0.1
 * @date 2018-10-24
 */

#ifndef SRC_FILTERCTRL_H_
#define SRC_FILTERCTRL_H_

#include <string>
#include <boost/smart_ptr.hpp>

using std::string;

class FilterCtrl {
public:
	FilterCtrl();
	virtual ~FilterCtrl();

public:
	/*!
	 * @brief 检查与控制器的连接状态
	 * @return
	 * 与滤光片控制器的连接状态
	 */
	bool IsConnected();
	/*!
	 * 连接滤光片控制器
	 * @return
	 * 与滤光片控制器的连接状态
	 */
	virtual bool Connect();
	/*!
	 * @brief 断开与滤光片控制器的连接
	 * @return
	 * 与滤光片控制器断开连接操作的成功性
	 */
	virtual bool Disconnect();
	/*!
	 * @brief 查看滤光片控制器名称/型号
	 * @return
	 * 滤光片控制器名称/型号
	 */
	const string & GetDeviceName();
	/*!
	 * @brief 设置滤光片转轮的层数
	 * @param n
	 * 滤光片转轮的层数
	 * @return
	 * 操作结果
	 */
	bool SetLayerCount(int n = 1);
	/*!
	 * @brief 设置每层转轮的插槽数量
	 * @param n 每层转轮的插槽数量
	 * @return
	 * 操作结果
	 */
	bool SetSlotCount(int n);
	/*!
	 * @brief 设置不同槽位的滤光片名称
	 * @param iLayer 层编号, 从0开始
	 * @param iSlot  插槽编号, 从0开始
	 * @param name   滤光片名称
	 * @return
	 * 操作结果
	 */
	bool SetFilterName(int iLayer, int iSlot, const string &name);
	/*!
	 * @brief 设置每层缺省滤光片的名称
	 * @param name   滤光片名称
	 * @param iLayer 层编号
	 * @return
	 * 操作结果
	 */
	bool SetDefaultFilter(const string &name, int iLayer = 0);
	/*!
	 * @brief 设置每层缺省滤光片的槽位编号
	 * @param name   插槽编号
	 * @param iLayer 层编号
	 * @return
	 * 操作结果
	 */
	bool SetDefaultFilter(int iSlot = 0, int iLayer = 0);
	/*!
	 * @brief 搜索零点
	 * @return
	 * 零点搜索结果
	 * @note
	 * 搜索零点可能不会立即返回, 即该函数可能持续一段时间直至成功或失败
	 */
	virtual bool FindHome();
	/*!
	 * @brief 查看当前使用的滤光片名称
	 * @param name 当前使用的滤光片名称
	 * @return
	 * 查询结果
	 * @note
	 * - 当不能确定转轮层编号或插槽编号时返回空滤光片名称
	 * - 多层转轮时, 缺省滤光片不构成滤光片名称
	 */
	bool GetFilterName(string &name);
	/*!
	 * @brief 定位滤光片
	 * @param iSlot  插槽编号, 从0开始
	 * @param iLayer 层编号, 从0开始
	 * @parma name   滤光片名称
	 * @return
	 * 定位结果
	 * @note
	 * - 定位操作需等待一段时间直至成功或失败
	 * - 多层转轮时, 非定位滤光片的一层转至该层缺省滤光片
	 */
	virtual bool Goto(int iSlot, int iLayer = 0);
	bool Goto(const string &name);

protected:
	virtual int get_slot(int iLayer = 0);

protected:
	bool connected_;	//< 滤光片连接标志
	string devname_;	//< 滤光片控制器名称/型号
	int nLayer_;	//< 转轮层数
	int nSlot_;		//< 单层插槽数量
	boost::shared_array<string> filters_; // 滤光片名称
	boost::shared_array<int> default_;	// 每层转轮的缺省滤光片位置
	boost::shared_array<int> position_;	// 每层转轮的滤光片位置
};
typedef boost::shared_ptr<FilterCtrl> FilterCtrlPtr;

#endif /* SRC_FILTERCTRL_H_ */
