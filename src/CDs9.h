/*
 * @file CDs9.h 通过XPA在ds9中显示FITS图像
 * @version 0.1
 * @date 2018年12月22日
 * @author 卢晓猛
 * @note
 * - 启动ds9. 规避重复启动
 * - 在ds9中显示图像
 */

#ifndef SRC_CDS9_H_
#define SRC_CDS9_H_

#include <xpa.h>

class CDs9 {
public:
	CDs9();
	virtual ~CDs9();

protected:
	static bool showimg_;	//< 图像是否显示标志
	bool firstimg_;	//< 第一幅图
	XPA xpa_;		//< XPA客户端
	char tmpl_[10];	//< ds9模板名称

public:
	/*!
	 * @brief 启动ds9
	 * @return
	 * ds9启动结果
	 * @note
	 * 检查是否已启动ds9, 若已启动则避免重复
	 */
	static bool StartDS9();
	/*!
	 * @brief 在ds9中显示FITS图像
	 * @param filepath FITS文件路径
	 * @return
	 * 图像显示结果
	 */
	bool DisplayImage(const char *filepath);

protected:
	/*!
	 * @brief 显示FITS图像
	 * @param filepath FITS文件路径
	 * @param filename FITS文件名
	 */
	bool XPASetFile(const char *filepath, const char *filename);
	/*!
	 * @brief 设置图像对比度调节方式
	 * @param mode 对比度调节方式
	 */
	void XPASetScaleMode(const char *mode);
	/*!
	 * @brief 图像缩放比例
	 * @param zoom 图像缩放比例
	 */
	void XPASetZoom(const char *zoom);
	/*!
	 * @brief 保持图像显示相对位置
	 */
	void XPAPreservePan(bool yes = true);
	/*!
	 * @brief 保持感兴趣区位置
	 */
	void XPAPreserveRegion(bool yes = true);
};

#endif /* SRC_CDS9_H_ */
