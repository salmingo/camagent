/*!
 * @file CameraFLICCD.cpp
 * @version 0.2
 * @date 2019-07-04
 * @note
 * FLI CCD坐标索引起点: (0, 0)
 */

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "CameraFLICCD.h"

using namespace boost::property_tree;

const char fliccd_conf[] = "/usr/local/etc/fliccd.xml";	// 相机性能参数列表

CameraFLICCD::CameraFLICCD() {
	domain_ = 0;
	hcam_   = 0;
	ulx_ = uly_ = 0;
	lrx_ = lry_ = 0;
}

CameraFLICCD::~CameraFLICCD() {
}

bool CameraFLICCD::open_camera() {
	if (!load_parameters()) return false;
	if (!FLIOpen(&hcam_, (char*) devName_.c_str(), domain_)) return false;
	FLISetHBin(hcam_, 1);
	FLISetVBin(hcam_, 1);
	FLISetImageArea(hcam_, ulx_, uly_, lrx_, lry_);
	FLISetTDI(hcam_, 0, 0);
	FLIControlBackgroundFlush(hcam_, FLI_BGFLUSH_START);

	return true;
}

bool CameraFLICCD::close_camera() {
	return !FLIClose(hcam_);
}

bool CameraFLICCD::cooler_onoff(bool &onoff, float &coolset) {
	if (!FLISetTemperature(hcam_, coolset)) return true;
	onoff = nfptr_->coolOn;
	coolset = nfptr_->coolSet;
	return false;
}

float CameraFLICCD::sensor_temperature() {
	double x;
	FLIGetTemperature(hcam_, &x);
	return float(x);
}

bool CameraFLICCD::update_adchannel(uint16_t &index, uint16_t &bitpix) {
	index  = 0;
	bitpix = 16;
	return !FLISetBitDepth(hcam_, FLI_MODE_16BIT);
}

bool CameraFLICCD::update_readport(uint16_t &index, string &readport) {
	index    = 0;
	readport = "Normal";
	return true;
}

bool CameraFLICCD::update_readrate(uint16_t &index, string &readrate) {
	index = 0;
	readrate = "Normal";
	return true;
}

bool CameraFLICCD::update_vsrate(uint16_t &index, float &vsrate) {
	index  = 0;
	vsrate = 0.0;
	return true;
}

bool CameraFLICCD::update_gain(uint16_t &index, float &gain) {
	index = 0;
	gain  = nfptr_->gain;
	return true;
}

bool CameraFLICCD::update_adoffset(uint16_t value) {
	return true;
}

bool CameraFLICCD::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {
	long ulx = ulx_ + x - 1;
	long uly = uly_ + y - 1;
	long lrx = ulx + w / xb;
	long lry = uly + h / yb;
	bool rslt = (!( FLISetHBin(hcam_, xb)
			|| FLISetVBin(hcam_, yb)
			|| FLISetImageArea(hcam_, ulx, uly, lrx, lry)));
	if (!rslt) {
		FLISetHBin(hcam_, 1);
		FLISetVBin(hcam_, 1);
		FLISetImageArea(hcam_, ulx_, uly_, lrx_, lry_);
		xb = yb = 1;
		x  = y  = 1;
		w  = lrx - ulx;
		h  = lry - uly;
	}
	return rslt;
}

bool CameraFLICCD::start_expose(float duration, bool light) {
	long rslt(0);
	rslt |= FLISetFrameType(hcam_, light ? FLI_FRAME_TYPE_NORMAL : FLI_FRAME_TYPE_DARK);
	rslt |= FLISetExposureTime(hcam_, long(duration * 1000));
	rslt |= FLIExposeFrame(hcam_);
	return !rslt;
}

bool CameraFLICCD::stop_expose() {
	bool rslt = !FLICancelExposure(hcam_);
	if (rslt) FLIControlBackgroundFlush(hcam_, FLI_BGFLUSH_START);
	return rslt;
}

CameraBase::CAMERA_STATUS CameraFLICCD::camera_state() {
	long state, remaining, rslt(0);
	CAMERA_STATUS status(CAMERA_EXPOSE);
	rslt |= FLIGetDeviceStatus(hcam_, &state);
	rslt |= FLIGetExposureStatus(hcam_, &remaining);

	if (rslt) status = CAMERA_ERROR;
	else if (state == FLI_CAMERA_STATUS_UNKNOWN && remaining == 0) status = CAMERA_IMGRDY;
	else if (state != FLI_CAMERA_STATUS_UNKNOWN && remaining == 0) status = CAMERA_IDLE;

	return status;
}

CameraBase::CAMERA_STATUS CameraFLICCD::download_image() {
	long rslt(0);
	// FLIGrabFrame()未实现
	int rows = nfptr_->roi.Height();		//< ROI区高度
	int wrow = nfptr_->roi.Width();			//< 行宽度, 量纲: 像素
	int byterow   = nfptr_->bytepix * wrow;	//< 行宽度, 量纲: 字节
	uint8_t *data = nfptr_->data.get();		//< 图像数据存储空间
	for (int r = 0; r < rows; ++r, data += byterow) {
		rslt |= FLIGrabRow(hcam_, data, wrow);
	}
	// 启动后台清空
	FLIControlBackgroundFlush(hcam_, FLI_BGFLUSH_START);
	return rslt ? CAMERA_ERROR : CAMERA_IMGRDY;
}

void CameraFLICCD::find_camera() {
	/* 查找并尝试连接相机 */
	flidomain_t domain;
	char devName[40], model[40];
	bool connected;

	// 在USB端口搜索相机
	FLICreateList(FLIDOMAIN_USB | FLIDEVICE_CAMERA);
	if(!FLIListFirst(&domain, devName, 40, model, 40))
		connected = 0 == FLIOpen(&hcam_, devName, domain);
	FLIDeleteList();
	if (!connected) {// 在网口上搜索相机
		FLICreateList(FLIDOMAIN_INET | FLIDEVICE_CAMERA);
		if(!FLIListFirst(&domain, devName, 40, model, 40))
			connected = 0 == FLIOpen(&hcam_, devName, domain);
		FLIDeleteList();
	}
	/* 写入配置文件 */
	if (connected) {
		ptree pt;
		double x, y;
		long ulx, uly, lrx, lry;

		FLIGetPixelSize(hcam_, &x, &y);
		FLIGetVisibleArea(hcam_, &ulx, &uly, &lrx, &lry);
		pt.add("DevName.<xmlattr>.value",    devName);
		pt.add("Domain.<xmlattr>.value",     domain);
		pt.add("VisibleArea.<xmlattr>.ulx",  ulx);
		pt.add("VisibleArea.<xmlattr>.uly",  uly);
		pt.add("VisibleArea.<xmlattr>.lrx",  lrx);
		pt.add("VisibleArea.<xmlattr>.lry",  lry);
		pt.add("Camera.<xmlattr>.model",     model);
		pt.add("PixelSize.<xmlattr>.x",      x * 1E6); // 米转换为微米
		pt.add("PixelSize.<xmlattr>.y",      y * 1E6);
		pt.add("Dimension.<xmlattr>.width",  lrx - ulx);
		pt.add("Dimension.<xmlattr>.height", lry - uly);

		xml_writer_settings<string> settings(' ', 4);
		write_xml(fliccd_conf, pt, std::locale(), settings);

		FLIClose(hcam_);
	}
}

bool CameraFLICCD::load_parameters() {
	try {
		ptree pt;
		read_xml(fliccd_conf, pt, xml_parser::trim_whitespace);

		devName_        = pt.get("DevName.<xmlattr>.value",    "");
		domain_         = pt.get("Domain.<xmlattr>.value",     0);
		ulx_            = pt.get("VisibleArea.<xmlattr>.ulx",  0);
		uly_            = pt.get("VisibleArea.<xmlattr>.uly",  0);
		lrx_            = pt.get("VisibleArea.<xmlattr>.lrx",  0);
		lry_            = pt.get("VisibleArea.<xmlattr>.lry",  0);
		nfptr_->model   = pt.get("Camera.<xmlattr>.model",     "");
		nfptr_->pixelX  = pt.get("PixelSize.<xmlattr>.x",      9.0);
		nfptr_->pixelY  = pt.get("PixelSize.<xmlattr>.y",      9.0);
		nfptr_->sensorW = pt.get("Dimension.<xmlattr>.width",  1024);
		nfptr_->sensorH = pt.get("Dimension.<xmlattr>.height", 1024);

		return true;
	}
	catch(xml_parser_error &ex) {
		find_camera();
		return false;
	}
}
