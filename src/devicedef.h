/*!
 * @file devicedef.h 声明与光学天文望远镜观测相关的状态、指令、类型等相关的常量
 * @version 0.1
 * @date 2017-11-24
 */

#ifndef _DEVICE_DEFINE_H_
#define _DEVICE_DEFINE_H_

/* 状态与指令 */
enum TELESCOPE_STATE {// 转台状态
	TELESCOPE_FIRST = 0,		//< 占位, 无效态
	TELESCOPE_ERROR = 0,		//< 错误
	TELESCOPE_FREEZE,		//< 静止
	TELESCOPE_HOMING,		//< 找零
	TELESCOPE_HOMED,			//< 找到零点
	TELESCOPE_PARKING,		//< 复位
	TELESCOPE_PARKED,		//< 已复位
	TELESCOPE_SLEWING,		//< 指向
	TELESCOPE_TRACKING,		//< 跟踪
	TELESCOPE_LAST = 7		//< 占位, 无效态
};

enum MIRRORCOVER_STATE {// 镜盖状态
	MC_FIRST = -2,
	MC_CLOSING = -2,
	MC_CLOSED,
	MC_OPEN = 1,
	MC_OPENING,
	MC_LAST = 2
};

enum FOCUSER_STATE {// 调焦器状态
	FOCUS_FIRST = 0,	//< 占位
	FOCUS_ERROR = 0,//< 错误
	FOCUS_FREEZE,	//< 静止
	FOCUS_MOVING,	//< 定位
	FOCUS_LAST = 2	//< 占位
};

enum IMAGE_TYPE {// 图像类型
	IMGTYPE_FIRST = 1,	// 占位
	IMGTYPE_BIAS = 1,	// 本底
	IMGTYPE_DARK,		// 暗场
	IMGTYPE_FLAT,		// 平场
	IMGTYPE_OBJECT,		// 目标
	IMGTYPE_FOCUS,		// 调焦
	IMGTYPE_ERROR,		// 错误
	IMGTYPE_LAST	 = 5		// 占位
};

enum EXPOSE_COMMAND {// 相机控制指令
	EXPOSE_START = 1,	//< 开始曝光
	EXPOSE_STOP,		//< 中止曝光
	EXPOSE_PAUSE,	//< 暂停曝光
	EXPOSE_RESUME,	//< EXPOSE_START分支: 当处理暂停过程中收到开始曝光指令, 指令记录为RESUME
	EXPOSE_ERROR,	// 错误
	EXPOSE_LAST = 4	//< 占位, 无效指令
};

enum CAMCTL_STATUS {// 相机工作状态
	CAMCTL_FIRST = 0,	// 占位
	CAMCTL_ERROR = 0,	// 错误
	CAMCTL_IDLE,			// 空闲
	CAMCTL_EXPOSE,		// 曝光过程中
	CAMCTL_COMPLETE,		// 已完成曝光
	CAMCTL_ABORT,		// 已中止曝光
	CAMCTL_PAUSE,		// 已暂停曝光
	CAMCTL_WAIT_TIME,	// 等待曝光流传起始时间到达
	CAMCTL_WAIT_FLAT,	// 平场间等待--等待转台重新指向
	CAMCTL_LAST = 7		// 占位
};

#endif
