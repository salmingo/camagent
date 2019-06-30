/*
 * @brief FitsHandler.h 基于cfitsio的FITS文件访问接口
 * @version 0.1
 * @author Xiaomeng Lu
 * @note
 * - 以读模式打开文件
 */

#ifndef FITSHANDLER_H_
#define FITSHANDLER_H_

#include <longnam.h>
#include <fitsio.h>
#include <string>

using std::string;

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
class FitsHandler {
public:
	FitsHandler();
	virtual ~FitsHandler();

protected:
	fitsfile *fileptr_;	//< 文件访问指针
	int rows_, cols_;	//< 行列数
	char errmsg[100];	//< 错误提示

protected:
	/*!
	 * @brief 关闭文件
	 */
	void close();
	/*!
	 * @brief 生成错误提示
	 * @param code cfitsio错误代码
	 */
	void fill_errmsg(int code);

public:
	/*!
	 * @brief 重载()
	 * @return
	 * FITS文件句柄
	 */
	fitsfile *operator()();
	/*!
	 * @brief 打开文件
	 * @param filepath 文件路径
	 * @return
	 * 文件打开结果
	 */
	bool Open(const char *filepath);
	/*!
	 * @brief 创建图像类型FITS文件
	 * @param filepath 文件路径
	 * @param bitpix   像素数据位数
	 * @param width    图像宽度
	 * @param height   图像高度
	 * @return
	 */
	bool CreateImage(const char *filepath, int bitpix, int width, int height);
	/*!
	 * @brief 数据纬度
	 * @param cols 列数
	 * @param rows 行数
	 */
	void GetDimension(int &cols, int &rows);
	/*!
	 * @brief 查询曝光时间
	 * @return
	 * 当查询失败时, 返回值小于0
	 */
	float GetExptime();
	/*!
	 * @brief 查询曝光起始时间/日期
	 * @return
	 * 查询结果
	 */
	bool GetDateobs(string &dateobs);
	/*!
	 * @brief 查询曝光起始时间
	 * @param timeobs
	 * @return
	 * 查询结果
	 */
	bool GetTimeobs(string &timeobs);
	/*!
	 * @brief 从图像型FITS中加载一行数据
	 * @param data    数据缓存区
	 * @param pixels  加载数据的像素数. == 0时加载cols_个
	 * @param row     行编号. 从0开始
	 * @param col     列编号. 从0开始
	 * @return
	 * 数据加载结果
	 */
	bool LoadPixels(float *data, int pixels = 0, int row = 0, int col = 0);
	/*!
	 * @brief 从图像型FITS中加载图像数据
	 * @param data 数据缓存区
	 * @return
	 * 数据加载结果
	 */
	bool LoadImage(float *data);
	/*!
	 * @brief 将图像数据写入FITS文件
	 * @param data     数据缓存区
	 * @param datatype 数据类型
	 * @return
	 */
	bool WriteImage(float *data, int datatype);
};
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */

#endif /* FITSHANDLER_H_ */
