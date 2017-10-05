/*
 * @file ATimeSpace.h 天文常用时空转换类声明文件
 * @author         卢晓猛
 * @description    封装实现常用时空转换函数
 * @version        2.0
 * @date           2016年12月31日
 * @note
 * - 采用SOFA算法替代Jean Meeus算法
 * - 地理经度: 遵循传统而不是IAU决议, 采用西经为正, 东经为负.
 * - 地理纬度: 采用北纬为正, 南纬为负
 * - 方位角:   采用南零点
 *
 * @note
 * 功能使用注意事项
 * - 平恒星时与平位置一起使用
 * - 真恒星时与真位置一起使用
 *
 * @note
 *
 *
 * @note
 * 功能列表:
 * - 小时数转换为时分秒
 * - 角度转换为度分秒
 * - 字符串转换为小时数
 * - 字符串转换为角度
 * - UTC与TAI(国际原子时)、UT1(世界时1)、TT(地球力学时)等的转换
 * - 儒略日与修正儒略日
 * - 儒略世纪
 * - 地球旋转角
 * - 历元
 * - 格林尼治和本地平恒星时
 * - 格林尼治和本地真恒星时
 */

#ifndef ATIMESPACE_H_
#define ATIMESPACE_H_

#define JD2K		2451545.0		//< 历元2000对应的儒略日
#define MJD0		2400000.5		//< 修正儒略日零点所对应的儒略日
#define MJD2K		51544.5			//< 历元2000对应的修正儒略日

namespace AstroUtil {
///////////////////////////////////////////////////////////////////////////////
class ATimeSpace {
	// 声明数据缓冲区名称与地址
	enum ATSValue {
		DAT,		//< DAT=TAI-UTC, 国际原子时与世界协调时之差, 量纲: 秒
		JD,			//< UTC对应的儒略日
		JC,			//< 相对J2000的儒略世纪
		EPOCH,		//< UTC对应的历元
		ERA,		//< 地球自转角
		NL,			//< 章动相对黄道面平行方向的分量
		NO,			//< 章动相对黄道面垂直方向的分量
		MEO,		//< 平黄赤交角
		TEO,		//< 真黄赤交角
		GMST,		//< UTC对应的格林尼治平恒星时
		GST,		//< UTC对应的格林尼治恒星时
		LMST,		//< UTC对应的本地平恒星时
		LST,		//< UTC对应的本地恒星时
		ATS_END		//< 最后一个数值, 用作判断缓冲区长度
	};

public:
	ATimeSpace();
	virtual ~ATimeSpace();

public:
	/*!
	 * @brief 将小时数转换为时分秒
	 * @param hour 小时数
	 * @param h    小时
	 * @param m    分
	 * @param s    秒
	 */
	void Hour2HMS(double hour, int &h, int &m, double &s);
	/*!
	 * @brief 将角度转换为度分秒
	 * @param deg   角度数
	 * @param sign  符号. 大于等于0的角度对应+, 小于0对应-
	 * @param d     度
	 * @param m     分
	 * @param s     秒
	 */
	void Deg2DMS(double deg, char &sign, int &d, int &m, double &s);
	/*!
	 * @brief 将小时数转换为以冒号分割时分秒的字符串
	 * @param hour 小时数
	 * @return
	 * 字符串表示的时分秒
	 */
	const char* HourDbl2Str(double hour);
	/*!
	 * @brief 将角度数转换为以冒号分割度分秒的字符串
	 * @param deg  角度数
	 * @return
	 * 字符串表示的度分秒
	 */
	const char* DegDbl2Str(double deg);
	/*!
	 * @brief 将字符串格式的小时转换为小时数
	 * @param str 字符串格式的小时. 字符串中, 可以省略分或秒, 时分秒可以冒号或空格分割
	 * @return
	 * 小时数. 若字符串格式错误返回小于0的无效数
	 */
	double HourStr2Dbl(const char *str);
	/*!
	 * @brief 将字符串格式的角度转换为角度数
	 * @param str 字符串格式的角度. 字符串中, 可以省略分或秒, 度分秒可以冒号或空格分割
	 * @return
	 * 角度数. 若字符串格式错误返回大于360的无效数
	 */
	double DegStr2Dbl(const char *str);

public:
	/*!
	 * @brief 设置测站地理位置
	 * @param lgt 地理经度, 量纲: 角度, 东经为正
	 * @param lat 地理纬度, 量纲: 角度, 北纬为正
	 * @param alt 海拔高度, 量纲: 米
	 */
	void SetSite(double lgt, double lat, double alt);
	/*!
	 * @brief 设置UTC时间
	 * @param iy   年
	 * @param im   月
	 * @param id   日
	 * @param vh   时, 当日0时为零点
	 * @note
	 * 约束条件:
	 * iy>=-4799
	 * im>=1 && im<=12
	 * idy>=1 && 小于月份最大日期数
	 */
	void SetUTC(int iy, int im, int id, double vh);
	/*!
	 * @brief 设置以儒略日表述的时间
	 * @param jd 儒略日
	 */
	void SetJD(double jd);

public:
	/*----------------------- 时间相关转换 -----------------------*/
	/*!
	 * @brief 查看JD对应的时间
	 * @param iy   年
	 * @param im   月
	 * @param id   日
	 * @param vh   时, 当日0时为零点
	 */
	void JD2Date(int &iy, int &im, int &id, double &vh);
	/*!
	 * @brief 查看与输入UTC时间对应的儒略日
	 * @return
	 * 儒略日
	 */
	double JulianDay();
	/*!
	 * @brief 查看设置时间对应的修正儒略日
	 * @return
	 * 修正儒略日
	 */
	double ModifiedJulianDay();
	/*!
	 * @brief 由儒略日计算儒略世纪
	 * @return
	 * 儒略世纪
	 */
	double JulianCentury();
	/*!
	 * @brief 由儒略日计算历元
	 * @return
	 * 历元
	 */
	double Epoch();
	/*!
	 * @brief 计算与输入UTC对应的地球自转角
	 * @return
	 * 地理自转角, 量纲: 弧度
	 */
	double EarthRotationAngle();
	/*!
	 * @brief 章动项
	 * @param nl 章动相对黄道面平行方向分量, 量纲: 弧度
	 * @param no 章动相对黄道面垂直方向分量, 量纲: 弧度
	 * @note
	 * 章动: 地轴的周期性振荡
	 */
	void Nutation(double &nl, double &no);
	/*!
	 * @brief 计算平黄赤交角
	 * @return
	 * 平黄赤交角, 量纲: 弧度
	 */
	double MeanEclipObliquity();
	/*!
	 * @brief 计算真黄赤交角
	 * @return
	 * 真黄赤交角, 量纲: 弧度
	 */
	double TrueEclipObliquity();
	/*!
	 * @brief 计算输入时间对应的格林尼治平恒星时
	 * @return
	 * 格林尼治平恒星时, 量纲: 小时
	 * @note
	 * 参考SOFA gmst06, 即采用2006地球岁差系数, 平恒星时由UT1和TT时获得
	 */
	double GreenwichMeanSiderealTime();
	/*!
	 * @brief 计算输入时间对应的格林尼治真恒星时
	 * @return
	 * 格林尼治真恒星时, 量纲: 小时
	 * @note
	 * 真恒星时是基于平恒星时修正章动项影响的结果
	 */
	double GreenwichSiderealTime();
	/*!
	 * @brief 计算输入时间对应的本地平恒星时
	 * @return
	 * 本地平恒星时, 量纲: 小时
	 */
	double LocalMeanSiderealTime();
	/*!
	 * @brief 计算输入时间对应的本地真恒星时
	 * @return
	 * 本地真恒星时, 量纲: 小时
	 */
	double LocalSiderealTime();

public:
	/*----------------------- 坐标系相关转换 -----------------------*/
	/*!
	 * @biref 赤道坐标系转换为地平坐标系
	 * @param ha   时角, 量纲: 弧度
	 * @param dec  赤纬, 量纲: 弧度
	 * @param azi  方位角, 量纲: 弧度, 南零点
	 * @param ele  仰角, 量纲: 弧度
	 */
	void Eq2Horizon(double ha, double dec, double &azi, double &ele);
	/*!
	 * @biref 地平坐标系转换为赤道坐标系
	 * @param azi  方位角, 量纲: 弧度, 南零点
	 * @param ele  仰角, 量纲: 弧度
	 * @param ha   时角, 量纲: 弧度
	 * @param dec  赤纬, 量纲: 弧度
	 */
	void Horizon2Eq(double azi, double ele, double &ha, double &dec);
	/*!
	 * @brief 赤道坐标系转换为黄道坐标系
	 * @param ra  赤经, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 * @param eo  黄赤交角, 量纲: 弧度
	 * @param l   黄经, 量纲: 弧度
	 * @param b   黄纬, 量纲: 弧度
	 */
	void Eq2Eclip(double ra, double dec, double eo, double &l, double &b);
	/*!
	 * @brief 黄道坐标系转换为赤道坐标系
	 * @param l   黄经, 量纲: 弧度
	 * @param b   黄纬, 量纲: 弧度
	 * @param eo  黄赤交角, 量纲: 弧度
	 * @param ra  赤经, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 */
	void Eclip2Eq(double l, double b, double eo, double &ra, double &dec);
	/*!
	 * @brief 计算视差角
	 * @param ha  时角, 量纲: 弧度
	 * @param dec 赤纬, 量纲: 弧度
	 * @return
	 * 视差角, 量纲: 弧度
	 * @note
	 * 由于周日运动带来的视觉偏差. 地平式望远镜需要第三轴抵消视差带来的影响
	 */
	double ParallacticAngle(double ha, double dec);
	/*!
	 * @brief 由真高度角计算蒙气差
	 * @param h0   真高度角, 由理论公式计算得到, 量纲: 弧度
	 * @param airp 大气压, 量纲: 毫巴
	 * @param temp 气温, 量纲: 摄氏度
	 * @return
	 * 蒙气差, 量纲: 角分
	 * @note
	 * h0+ref=视高度角, 望远镜采用视高度角以准确指向目标
	 */
	double TrueRefract(double h0, double airp, double temp);
	/*!
	 * @brief 由视高度角计算蒙气差
	 * @param h    视高度角, 由实测得到, 量纲: 弧度
	 * @param airp 大气压, 量纲: 毫巴
	 * @param temp 气温, 量纲: 摄氏度
	 * @return
	 * 蒙气差, 量纲: 角分
	 * @note
	 * h-ref=真高度角
	 */
	double VisualRefract(double h, double airp, double temp);
	/*!
	 * @brief 计算两点的大圆距离
	 * @param l1 位置1的经度, 量纲: 弧度
	 * @param b1 位置1的纬度, 量纲: 弧度
	 * @param l2 位置2的经度, 量纲: 弧度
	 * @param b2 位置2的纬度, 量纲: 弧度
	 * @return
	 * 两点的大圆距离, 量纲: 弧度
	 */
	double SphereAngle(double l1, double b1, double l2, double b2);

protected:
	/*!
	 * @brief 使数据缓冲区无效化
	 */
	void invalid_values();
	/*!
	 * @brief 将周期函数值调制到[0, T)区间
	 * @param x  周期函数值x
	 * @param T  周期
	 * @return
	 * x在[0, T)区间的对应值
	 */
	double reduce(double x, double T);
	/*!
	 * @brief 计算时间对应的儒略日
	 * @param iy   年
	 * @param im   月
	 * @param id   日
	 * @param vh   时, 当日0时为零点
	 * @return
	 * 与时间对应的儒略日
	 * @note
	 * 约束条件:
	 * iy>=-4799
	 * im>=1 && im<=12
	 * idy>=1 && 小于月份最大日期数
	 */
	double calc_mjd(int iy, int im, int id, double vh);
	/*!
	 * @brief 计算UTC时间对应的DAT=TAI-UTC, 即国际原子时与世界协调时之差值
	 * @param iy   年
	 * @param im   月
	 * @param id   日
	 * @param vh   时, 当日0时为零点
	 * @return
	 * 与UTC时间对应的闰秒
	 * @note
	 * 采用2015年6月30日及之前的闰秒数据
	 */
	double calc_dat(int iy, int im, int id, double vh);

protected:
	// 成员变量
	double	m_lgt;	//< 测站地理经度, 量纲: 弧度. 西经为正
	double	m_lat;	//< 测站地理纬度, 量纲: 弧度. 北纬为正
	double	m_alt;	//< 测站海拔高度, 量纲: 米
	double	m_mjd;	//< 修正儒略日

	double	m_value[ATS_END];	//< 数据缓冲区, 避免重复计算
	bool	m_valid[ATS_END];	//< 数据缓冲区有效性
};
///////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* ATIMESPACE_H_ */
