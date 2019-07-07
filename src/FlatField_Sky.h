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
		expdur = tmin;
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
		double t0 = (double) expdur / flux;
		int min_et = int(fmin * t0 + 1);
		int max_et = int(fmax * t0);
		// 退出条件
		if ((    ampm && max_et <= tmin)  // 晨光: 最长曝光时间小于阈值
			|| (!ampm && min_et >= tmax)) { // 昏影: 最短曝光时间长于阈值
			expdur = 0;
		}
		else {// 变更曝光时间
			if (expdur < min_et || expdur >= max_et) expdur = min_et;
			else ++expdur;
			if (expdur < tmin) expdur = tmin;
			else if (expdur > tmax) expdur = tmax;
		}
		return expdur;
	}
};

/*!
 * @brief 评估平场图像
 * @param data 数据存储区
 * @param w    图像宽度
 * @param h    图像高度
 * @return
 * 平场图像中心区域统计均值
 * @note
 * 中心区域定义为图像中心宽高=100像素范围
 */
template<class T>
int asses_flat(T *data, int w, int h) {
	double sum(0.0);
	int ws = 100;	// 采用窗口宽度
	int x1 = (w  - ws) / 2;
	int x2 = x1 + ws;
	int y1 = (h  - ws) / 2;
	int y2 = y1 + ws;
	int n = (x2 - x1) * (y2 - y1);
	int x, y;
	T *ptr;
	for (y = y1, ptr = data + y1 * w; y < y2; ++y, ptr += w) {
		for (x = x1; x < x2; ++x) sum += ptr[x];
	}
	return int(sum / n);
}

#endif
