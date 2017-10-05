/*
 * @file ATimeSpace.cpp 天文常用时空转换类定义文件
 * @author         卢晓猛
 * @description    封装实现常用时空转换函数
 * @version        2.0
 * @date           2016年12月31日
 */
#include "ADefine.h"
#include "ATimeSpace.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

namespace AstroUtil {
///////////////////////////////////////////////////////////////////////////////
ATimeSpace::ATimeSpace() {
	m_lgt = m_lat = m_alt = 0.0;
	m_mjd = 0.0;

	invalid_values();
}

ATimeSpace::~ATimeSpace() {
}

///////////////////////////////////////////////////////////////////////////////
void ATimeSpace::Hour2HMS(double hour, int &h, int &m, double &s) {
	h = (int) hour;
	m = (int) ((hour - h) * 60.0);
	s = ((hour - h) * 60.0 - m) * 60.0;
}

void ATimeSpace::Deg2DMS(double deg, char &sign, int &d, int &m, double &s) {
	if (deg >= 0.0) sign = '+';
	else {
		sign = '-';
		deg = -deg;
	}
	d = (int) deg;
	m = (int) ((deg - d) * 60.0);
	s = ((deg - d) * 60.0 - m) * 60.0;
}

const char* ATimeSpace::HourDbl2Str(double hour) {
	static char str[20];
	int h, m;
	double s;
	Hour2HMS(hour, h, m, s);
	sprintf(str, "%02d:%02d:%06.3f", h, m, s);
	return str;
}

const char* ATimeSpace::DegDbl2Str(double deg) {
	static char str[20];
	int d, m;
	double s;
	char sign;
	Deg2DMS(deg, sign, d, m, s);
	sprintf(str, "%c%02d:%02d:%05.2f", sign, d, m, s);

	return str;
}

double ATimeSpace::HourStr2Dbl(const char *str) {
	int n;
	double h(0.0), m(0.0);
	char ch;	// 单个字符
	char buff[20];
	int i, j;
	int part(0);	// 标志时分秒区. 0: 时; 1: 分; 2: 秒
	int dot(0);

	if (!str || !(n = strlen(str))) return 0.0;

	for (i = 0, j = 0; i < n; ++i) {
		ch = str[i];
		if (ch >= '0' && ch <= '9') {// 有效数字
			if (!dot && (part <= 1) && j == 2) {// 时分位置, 已够两位该区域数字
				buff[j] = 0;
				j = 0;
				if (part == 0) h = atof(buff);
				else           m = atof(buff);
				++part;
			}
			buff[j++] = ch;
		}
		else if (ch == ' ' || ch == ':') {// 分隔符
			if (dot > 0 || part >= 2) return -1.0;	// 出现小数点后禁止出现分隔符
			if (j > 0) {// 判断缓冲数据长度. j>0判断用于允许误连续输入的空格符和冒号
				buff[j] = 0;
				j = 0;
				if (part == 0)      h = atof(buff);
				else if (part == 1) m = atof(buff);
				++part;
			}
		}
		else if (ch == '.') {// 小数点
			if (dot > 0) return -1.0;	// 禁止出现多次小数点
			++dot;
			buff[j++] = ch;
		}
		else return -1.0;
	}
	buff[j] = 0;

	return part == 0 ? atof(buff) :
			(part == 1 ? (h + atof(buff) / 60.0) :
					(h + (m + atof(buff) / 60.0) / 60.0));
}

double ATimeSpace::DegStr2Dbl(const char *str) {
	int n;
	double d(0.0), m(0.0), sign(1.0);
	char ch;	// 单个字符
	char buff[20];
	int i(0), j;
	int part(0);	// 标志度分秒区. 0: 度; 1: 分; 2: 秒
	int dot(0);

	if (!str || !(n = strlen(str))) return 0.0;

	if ((ch = str[0]) == '+' || ch == '-') {
		i = 1;
		if (ch == '-') sign = -1.0;
	}
	for (j = 0; i < n; ++i) {
		ch = str[i];
		if (ch >= '0' && ch <= '9') {// 有效数字
			if (!dot && ((part == 0 && j == 3) || (part == 1 && j == 2))) {// 度分位置
				buff[j] = 0;
				j = 0;
				if (part == 0) d = atof(buff);
				else           m = atof(buff);
				++part;
			}
			buff[j++] = ch;
		}
		else if (ch == ' ' || ch == ':') {// 分隔符
			if (dot > 0 || part >= 2) return -1.0;	// 出现小数点后禁止出现分隔符
			if (j > 0) {// 判断缓冲数据长度. j>0判断用于允许误连续输入的空格符和冒号
				buff[j] = 0;
				j = 0;
				if (part == 0)      d = atof(buff);
				else if (part == 1) m = atof(buff);
				++part;
			}
		}
		else if (ch == '.') {// 小数点
			if (dot > 0) return -1.0;	// 禁止出现多次小数点
			++dot;
			buff[j++] = ch;
		}
		else return -1.0;
	}
	buff[j] = 0;

	return part == 0 ? atof(buff) * sign :
			(part == 1 ? (d + atof(buff) / 60.0) * sign :
					(d + (m + atof(buff) / 60.0) / 60.0) * sign);
}
///////////////////////////////////////////////////////////////////////////////
void ATimeSpace::invalid_values() {
	bzero(m_value, sizeof(double) * ATS_END);
	bzero(m_valid, sizeof(bool) * ATS_END);
}

double ATimeSpace::reduce(double x, double T) {
	double y = fmod(x, T);
	return (y < 0 ? y + T : y);
}

// 设置测站地理位置
void ATimeSpace::SetSite(double lgt, double lat, double alt) {
	m_lgt = lgt * DtoR;
	m_lat = lat * DtoR;
	m_alt = alt;
}

// 设置UTC时钟
void ATimeSpace::SetUTC(int iy, int im, int id, double vh) {
	invalid_values();

	m_mjd = calc_mjd(iy, im, id, vh);
	m_value[DAT] = calc_dat(iy, im, id, vh);
	m_valid[DAT] = true;
}

void ATimeSpace::SetJD(double jd) {
	if (jd != m_value[JD]) {
		invalid_values();

		m_value[JD] = jd;
		m_valid[JD] = true;
		m_mjd = jd - MJD0;

		int iy, im, id;
		double vh;
		JD2Date(iy, im, id, vh);
		m_value[DAT] = calc_dat(iy, im, id, vh);
		m_valid[DAT] = true;
	}
}
///////////////////////////////////////////////////////////////////////////////
/*----------------------- 时间相关转换 -----------------------*/
// 计算儒略日
double ATimeSpace::calc_mjd(int iy, int im, int id, double vh) {
	int ly, my;
	long iypmy;
	double mjd;

	// 闰年
	ly = (im == 2) && !(iy%4) && (iy%100 || !(iy%400));
	my = (im - 14) / 12;
	iypmy = (long) (iy + my);
	mjd = (double)((1461L * (iypmy + 4800L)) / 4L
			+ (367L * (long) (im - 2 - 12 * my)) / 12L
			- (3L * ((iypmy + 4900L) / 100L)) / 4L
			+ (long) id - 2432076L) + vh / 24.0;

	return mjd;
}

// 计算国际原子时与世界协调时的偏差: DAT=TAI-UTC
double ATimeSpace::calc_dat(int iy, int im, int id, double vh) {
	// 闰秒最新发布日期: Dec 31, 2016. 1972(含)年之后, 在每年的6月30日或12月31日发布闰秒调节量
	// 每次的闰秒调节量为0或1秒
	// 在定义闰秒之前采用漂移速度方法计算偏差, 修正儒略日与漂移速度(秒/日)的关系
	static const double drift[][2] = {
			{ 37300.0, 0.0012960 },
			{ 37300.0, 0.0012960 },
			{ 37300.0, 0.0012960 },
			{ 37665.0, 0.0011232 },
			{ 37665.0, 0.0011232 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 38761.0, 0.0012960 },
			{ 39126.0, 0.0025920 },
			{ 39126.0, 0.0025920 }
	};
	const int NERA1 = (int) (sizeof(drift) / sizeof (double) / 2);

	// 闰秒发布日期与调节量
	static const struct {
		int iyear, month;
		double delat;
	} changes[] = {
			{ 1960,  1,  1.4178180 },
			{ 1961,  1,  1.4228180 },
			{ 1961,  8,  1.3728180 },
			{ 1962,  1,  1.8458580 },
			{ 1963, 11,  1.9458580 },
			{ 1964,  1,  3.2401300 },
			{ 1964,  4,  3.3401300 },
			{ 1964,  9,  3.4401300 },
			{ 1965,  1,  3.5401300 },
			{ 1965,  3,  3.6401300 },
			{ 1965,  7,  3.7401300 },
			{ 1965,  9,  3.8401300 },
			{ 1966,  1,  4.3131700 },
			{ 1968,  2,  4.2131700 },
			{ 1972,  1, 10.0       },
			{ 1972,  7, 11.0       },
			{ 1973,  1, 12.0       },
			{ 1974,  1, 13.0       },
			{ 1975,  1, 14.0       },
			{ 1976,  1, 15.0       },
			{ 1977,  1, 16.0       },
			{ 1978,  1, 17.0       },
			{ 1979,  1, 18.0       },
			{ 1980,  1, 19.0       },
			{ 1981,  7, 20.0       },
			{ 1982,  7, 21.0       },
			{ 1983,  7, 22.0       },
			{ 1985,  7, 23.0       },
			{ 1988,  1, 24.0       },
			{ 1990,  1, 25.0       },
			{ 1991,  1, 26.0       },
			{ 1992,  7, 27.0       },
			{ 1993,  7, 28.0       },
			{ 1994,  7, 29.0       },
			{ 1996,  1, 30.0       },
			{ 1997,  7, 31.0       },
			{ 1999,  1, 32.0       },
			{ 2006,  1, 33.0       },
			{ 2009,  1, 34.0       },
			{ 2012,  7, 35.0       },
			{ 2015,  7, 36.0       },
			{ 2016, 12, 37.0       }
	};
	const int NDAT = sizeof (changes) / sizeof changes[0];

	// 若输入时间的年份在闰秒定义之前, 则不计算偏差量
	if (iy < changes[0].iyear) return 0.0;

	double t(0.0);
	int m = 12*iy + im; // 查找对应的闰秒
	int i;

	for (i = NDAT-1; i >=0; i--) {
		if (m >= (12 * changes[i].iyear + changes[i].month)) break;
	}
	t = changes[i].delat;
	// 计算1972年之前的漂移量
	if (i < NERA1) t += (m_mjd + vh / 24.0 - drift[i][0]) * drift[i][1];

	return t;
}

void ATimeSpace::JD2Date(int &iy, int &im, int &id, double &vh) {
	double jd = JulianDay() + 0.5;
	int jd0 = (int) jd;
	long l, n, i, k;

	vh = fmod(m_mjd, 1.0) * 24;
	l = jd0 + 68569L;
	n = (4L * l) / 146097L;
	l -= (146097L * n + 3L) / 4L;
	i = (4000L * (l + 1L)) / 1461001L;
	l -= (1461L * i) / 4L - 31L;
	k = (80L * l) / 2447L;
	id = (int) (l - (2447L * k) / 80L);
	l = k / 11L;
	im = (int) (k + 2L - 12L * l);
	iy = (int) (100L * (n - 49L) + i + l);
}

double ATimeSpace::JulianDay() {
	if (!m_valid[JD]) {
		m_value[JD] = m_mjd + MJD0;
		m_valid[JD] = true;
	}
	return m_value[JD];
}

double ATimeSpace::ModifiedJulianDay() {
	return m_mjd;
}

// 相对J2000的儒略世界
double ATimeSpace::JulianCentury() {
	if (!m_valid[JC]) {
		m_value[JC] = (m_mjd - 51544.5) / 36525;
		m_valid[JC] = true;
	}

	return m_value[JC];
}

// 历元
double ATimeSpace::Epoch() {
	if (!m_valid[EPOCH]) {
		m_value[EPOCH] = JulianCentury() + 2000.0;
		m_valid[EPOCH] = true;
	}

	return m_value[EPOCH];
}

// 地球自转角
double ATimeSpace::EarthRotationAngle() {
	if (!m_valid[ERA]) {

		m_valid[ERA] = true;
	}

	return m_value[ERA];
}

// 章动
void ATimeSpace::Nutation(double &nl, double &no) {
	if (!m_valid[NL]) {
		double t = JulianCentury();
		double L1 = (280.4665 + 36000.7698 * t) * DtoR;
		double L2 = (218.3165 + 481267.8813 * t) * DtoR;
		double Q  = (125.04452 - t * 1934.136261) * DtoR;

		m_value[NL] = (-17.2 * sin(Q) + 1.32 * sin(2 * L1) - 0.23 * sin(2 * L2) + 0.21 * sin(2 * Q)) * AStoR;
		m_value[NO] = (9.2 * cos(Q) + 0.57 * cos(2 * L1) + 0.1 * cos(2 * L2) - 0.09 * cos(2 * Q)) * AStoR;
		m_valid[NL] = true;
		m_valid[NO] = true;
	}

	nl = m_value[NL];
	no = m_value[NO];
}

double ATimeSpace::MeanEclipObliquity() {
	if (!m_valid[MEO]) {
		double u = JulianCentury() * 0.01;
		double t(0.0);
		double coef[] = {
				2.45, 5.79, 27.87, 7.12, -39.05,
				-249.67, -51.38, 1999.25, -1.55, -4680.93, 84381.448
		};
		int n = sizeof(coef) / sizeof(double);
		for (int i = 0; i < n; ++i) t = t * u + coef[i];
		m_value[MEO] = t * AStoR;
		m_valid[MEO] = true;
	}

	return m_value[MEO];
}
/*!
 * @brief 计算真黄赤交角
 * @return
 * 真黄赤交角, 量纲: 弧度
 */
double ATimeSpace::TrueEclipObliquity() {
	if (!m_valid[TEO]) {
		double nl, no;
		double meo = MeanEclipObliquity();
		Nutation(nl, no);
		m_value[TEO] = meo + no;
		m_valid[TEO] = true;
	}

	return m_value[TEO];
}

// 格林尼治平恒星时
double ATimeSpace::GreenwichMeanSiderealTime() {
	if (!m_valid[GMST]) {
		double t  = JulianCentury();
		double gmst = 280.46061837 + t * (13185000.77005374225 + t * (3.87933E-4 - t / 38710000.0));
		m_value[GMST] = reduce(gmst, 360.0) / 15.0;
		m_valid[GMST] = true;
	}

	return m_value[GMST];
}

// 格林尼治真恒星时
double ATimeSpace::GreenwichSiderealTime() {
	if (!m_valid[GST]) {

		m_valid[GST] = true;
	}

	return m_value[GST];
}

// 本地平恒星时
double ATimeSpace::LocalMeanSiderealTime() {
	if (!m_valid[LMST]) {
		m_value[LMST] = reduce(GreenwichMeanSiderealTime() - m_lgt * RtoD / 15.0, 24.0);
		m_valid[LMST] = true;
	}

	return m_value[LMST];
}

// 本地真恒星时
double ATimeSpace::LocalSiderealTime() {
	if (!m_valid[LST]) {

		m_valid[LST] = true;
	}

	return m_value[LST];
}
///////////////////////////////////////////////////////////////////////////////
/*----------------------- 坐标系相关转换 -----------------------*/
// 赤道坐标系转换为地平坐标系
void ATimeSpace::Eq2Horizon(double ha, double dec, double &azi, double &ele) {
	azi = atan2(sin(ha), cos(ha) * sin(m_lat) - tan(dec) * cos(m_lat));
	ele = asin(sin(m_lat) * sin(dec) + cos(m_lat) * cos(dec) * cos(ha));
	if (azi < 0) azi += D2PI;
}

// 地平坐标系转换为赤道坐标系
void ATimeSpace::Horizon2Eq(double azi, double ele, double &ha, double &dec) {
	ha  = atan2(sin(azi), cos(azi) * sin(m_lat) + tan(ele) * cos(m_lat));
	dec = asin(sin(m_lat) * sin(ele) - cos(m_lat) * cos(ele) * cos(azi));
	if (ha < 0) ha += D2PI;
}

// 赤道坐标系转换为黄道坐标系
void ATimeSpace::Eq2Eclip(double ra, double dec, double eo, double &l, double &b) {
	l = atan2(sin(ra) * cos(eo) + tan(dec) * sin(eo), cos(ra));
	b = asin(sin(dec) * cos(eo) - cos(dec) * sin(eo) * sin(ra));
	if (l < 0) l += D2PI;
}

// 黄道坐标系转换为赤道坐标系
void ATimeSpace::Eclip2Eq(double l, double b, double eo, double &ra, double &dec) {
	ra  = atan2(sin(l) * cos(eo) - tan(b) * sin(eo), cos(l));
	dec = asin(sin(b) * cos(eo) + cos(b) * sin(eo) * sin(l));
	if (ra < 0) ra += D2PI;
}

// 由周日运动带来的视觉误差
double ATimeSpace::ParallacticAngle(double ha, double dec) {
	return (atan2(sin(ha), tan(m_lat) * cos(dec) - sin(dec) * cos(ha)));
}

// 由真高度角计算蒙气差
double ATimeSpace::TrueRefract(double h0, double airp, double temp) {
	double k = (airp / 1010.0) * (283.0 / (273.0 + temp));
	double R0;

	h0 *= RtoD;
	R0 = 1.02 / tan((h0 + 10.3 / (h0 + 5.11)) * DtoR) + 1.9279E-3;

	return (R0 * k);
}

// 由视高度角计算蒙气差
double ATimeSpace::VisualRefract(double h, double airp, double temp) {
	double k = (airp / 1010.0) * (283.0 / (273.0 + temp));
	double R0;

	h *= RtoD;
	R0 = 1.0 / tan((h + 7.31 / (h + 4.4)) * DtoR) + 1.3515E-3;

	return (R0 * k);
}

// 两点大圆距离
double ATimeSpace::SphereAngle(double l1, double b1, double l2, double b2) {
	return acos(sin(b1) * sin(b2) + cos(b1) * cos(b2) * cos(l1 - l2));
}
///////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
