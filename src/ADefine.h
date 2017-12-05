/*
 * @file ADefine.h 天文常数及宏定义
 */

#ifndef ADEFINE_H_
#define ADEFINE_H_

#include <math.h>
#include <string.h>
#include <stdlib.h>

namespace AstroUtil {
///////////////////////////////////////////////////////////////////////////////
// 角度<=>弧度<=>角秒<=>毫角秒
#define API		3.141592653589793238462643		//< 圆周率
#define A2PI		6.283185307179586476925287		//< 2倍圆周率

#define R2D		5.729577951308232087679815E1		//< 弧度转换为角度的乘法因子
#define D2R		1.745329251994329576923691E-2	//< 角度转换为弧度的乘法因子

#define R2H		3.819718634205488058453210		//< 弧度转换为小时的乘法因子
#define H2R		2.617993877991494365385536E-1	//< 小时转换为弧度的乘法因子

#define R2S		1.375098708313975701043156E4		//< 弧度转换为秒的乘法因子
#define S2R		7.272205216643039903848712E-5	//< 秒转换为弧度的乘法因子

#define R2AS		2.062648062470963551564734E5		//< 弧度转换为角秒的乘法因子
#define AS2R		4.848136811095359935899141E-6	//< 角秒转换为弧度的乘法因子

#define AS2D		2.777777777777777777777778E-4	//< 角秒转换为角度的乘法因子
#define D2AS		3600.0							//< 角度转换为角秒的乘法因子

/// 时间常数
#define JD2K			2451545.0	//< 历元2000对应的儒略日
#define MJD0			2400000.5	//< 修正儒略日零点所对应的儒略日
#define MJD2K		51544.5		//< 历元2000对应的修正儒略日
#define MJD77		43144.0		//< 1977年1月1日0时对应的修正儒略日
#define TTMTAI		32.184		//< TTMTAI=TT-TAI
#define DAYSEC		86400.0		//< 每日秒数
#define DJY			365.25		//< 儒略历每年天数
#define DJC			36525.0		//< 儒略历每世纪天数
#define DJM			365250.0		//< 儒略历每千年天数

/// 数字常数
#define AEPS			1E-4		//< 最小值
#define AMAX			1E30		//< 最大值

/// 计算实数的小数部分
#define frac(x)		((x) - floor(x))
/// 将数值调整到一个周期内
#define reduce(x, period)	((x) - floor((x) / (period)) * (period))
/// 余切
#define cot(x)		(tan(API * 0.5 - (x)))

/// 浮点型点
typedef struct POINT2F {
	double x, y;	//< XY坐标

public:
	POINT2F() {
		x = y = 0.0;
	}

	POINT2F(double _x, double _y) {
		x = _x;
		y = _y;
	}

	POINT2F& operator=(const POINT2F& pt) {
		if (this != &pt) {
			x = pt.x;
			y = pt.y;
		}
		return *this;
	}

	bool operator==(const POINT2F& pt) {
		double dx = x - pt.x;
		double dy = y - pt.y;
		if (dx < 0.0) dx = -dx;
		if (dy < 0.0) dy = -dy;
		return (dx < AEPS && dy < AEPS);
	}
} PT2F, * PPT2F;

typedef struct POINT3F {
	double x, y, z;	//< XYZ坐标

public:
	POINT3F() {
		x = y = z = 0.0;
	}

	POINT3F(double _x, double _y, double _z) {
		x = _x;
		y = _y;
		z = _z;
	}

	POINT3F& operator=(const POINT3F& pt) {
		if (this != &pt) {
			x = pt.x;
			y = pt.y;
			z = pt.z;
		}
		return *this;
	}

	bool operator==(const POINT3F& pt) {
		double dx = x - pt.x;
		double dy = y - pt.y;
		double dz = z - pt.z;
		if (dx < 0.0) dx = -dx;
		if (dy < 0.0) dy = -dy;
		if (dz < 0.0) dz = -dz;
		return (dx < AEPS && dy < AEPS && dz < AEPS);
	}
} PT3F, * PPT3F;
///////////////////////////////////////////////////////////////////////////////
}

#endif
