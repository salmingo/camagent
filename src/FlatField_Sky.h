/**
 * @struct FlatField_Sky 计算天光平场的曝光时间
 * @version 0.1
 * @date Feb 27, 2019
 * @author Xiaomeng Lu
 * @note
 *
 */

#ifndef FLATFIELD_SKY_H_
#define FLATFIELD_SKY_H_

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;

struct FlatField_Sky {
	int tmin, tmax;	//< 有效曝光时间阈值, 量纲: 秒
	int fmin, fmax;	//< 有效平场流量阈值, 量纲: 秒
	int expdur;		//< 当前曝光时间, 量纲: 秒
	bool ampm;		//< 晨昏蒙影判定. 晨光: true

public:
	/*!
	 * @brief 启动天光平场采集流程
	 * @return
	 * 经计算得到的曝光时间
	 */
	int Start() {
		ptime loc = second_clock::local_time();
		ampm = loc.time_of_day().hours() < 12;
		expdur = ampm ? tmax : tmin;
		return expdur;
	}

	/*!
	 * @brief 依据流量计算应采用的曝光时间
	 * @return
	 * 经计算得到的曝光时间
	 * @note
	 * 当返回值==0时表示曝光终止
	 */
	int Next(int flux) {
		if ((    ampm && (flux * tmin > expdur * fmax))  // 晨光: 最短曝光时间的流量超出阈值
			|| (!ampm && (flux * tmax < expdur * fmin))) // 蒙影: 最长曝光时间的流量超出阈值
		{// 终止条件
			expdur = 0;
		}
		else {
			// 计算可用曝光时间阈值
			double t0 = (double) expdur / flux;
			int exptmin = int(fmin * t0 + 1);
			int exptmax = int(fmax * t0);
			if (exptmin < tmin) exptmin = tmin;
			if (exptmax > tmax) exptmax = tmax;
		}
//		else if (ampm) {// 晨光: 当曝光时间大于最短时间时, 增加曝光时间直至获得最大流量
//			/*
//			 * 分支条件:
//			 * 1. flux < fmin: 继续t=tmax
//			 * 2. flux < fmax:
//			 */
//		}
//		else {// 昏影: 当曝光时间...
//
//		}

		return expdur;
	}
};

#endif
