/*
 * @file FilterCtrlFLI.h 声明FLI滤光片控制接口
 * @version 0.1
 * @date 2018-10-24
 * @note
 * SDK部署:
 * - cp libfli.h /usr/local/include
 * - cp libfli.a /usr/local/lib
 */

#ifndef SRC_FILTERCTRLFLI_H_
#define SRC_FILTERCTRLFLI_H_

#include <libfli.h>
#include "FilterCtrl.h"

class FilterCtrlFLI: public FilterCtrl {
public:
	FilterCtrlFLI();
	virtual ~FilterCtrlFLI();

public:
	/*!
	 * 连接滤光片控制器
	 * @return
	 * 与滤光片控制器的连接状态
	 */
	bool Connect();
	/*!
	 * @brief 断开与滤光片控制器的连接
	 * @return
	 * 与滤光片控制器断开连接操作的成功性
	 */
	bool Disconnect();
	/*!
	 * @brief 搜索零点
	 * @return
	 * 零点搜索结果
	 * @note
	 * 搜索零点可能不会立即返回, 即该函数可能持续一段时间直至成功或失败
	 */
	bool FindHome();
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
	bool Goto(int iSlot, int iLayer);

protected:
	virtual int get_slot(int iLayer);

protected:
	flidev_t devID_;	//< 设备句柄
};

#endif /* SRC_FILTERCTRLFLI_H_ */
