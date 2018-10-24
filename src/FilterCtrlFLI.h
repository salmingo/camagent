/*
 * @file FilterCtrlFLI.h 声明FLI滤光片控制接口
 * @version 0.1
 * @date 2018-10-24
 */

#ifndef SRC_FILTERCTRLFLI_H_
#define SRC_FILTERCTRLFLI_H_

#include "FilterCtrl.h"

class FilterCtrlFLI: public FilterCtrl {
public:
	FilterCtrlFLI();
	virtual ~FilterCtrlFLI();
};

#endif /* SRC_FILTERCTRLFLI_H_ */
