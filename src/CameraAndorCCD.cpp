/*
 * CameraAndorCCD.cpp Andor CCD相机的实现文件
 * @date 2015-09-22
 * @version 0.1
 * @author 卢晓猛
 */

#include <unistd.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "CameraAndorCCD.h"

using namespace std;
using namespace boost::property_tree;

const char andor_conf[] = "/usr/local/etc/andor.xml";	// 相机性能参数列表
const char andor_dir[]  = "/usr/local/etc/andor";		// 初始化连接的目录

CameraAndorCCD::CameraAndorCCD() {
}

CameraAndorCCD::~CameraAndorCCD() {
}

bool CameraAndorCCD::open_camera() {
	if (Initialize((char*) andor_dir) != DRV_SUCCESS) return false;
	load_parameters();

	return true;
}

void CameraAndorCCD::close_camera() {
	ShutDown();
}

void CameraAndorCCD::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {
}

void CameraAndorCCD::cooler_onoff(float &coolset, bool &onoff) {
	if (onoff) {
		int set = int(coolset);
		int tmin = xmlpt_.get("Cooler.<xmlattr>.min", 0);
		int tmax = xmlpt_.get("Cooler.<xmlattr>.max", 0);

		if (set < tmin) set = tmin;
		else if (set > tmax) set = tmax;
		if (DRV_SUCCESS == SetTemperature(set)) coolset = float(set);
		if (DRV_SUCCESS == CoolerON()) onoff = true;
	}
	else if (DRV_SUCCESS == CoolerOFF()) onoff = false;
}

float CameraAndorCCD::sensor_temperature() {
	int val;
	GetTemperature(&val);
	return float(val);
}

void CameraAndorCCD::update_adchannel(uint16_t &index, uint16_t &bitpix) {
	int pixel;

	if (DRV_SUCCESS == GetBitDepth(0, &pixel)) bitpix = uint16_t(pixel);
}

void CameraAndorCCD::update_readport(uint16_t &index, string &readport) {
}

void CameraAndorCCD::update_readrate(uint16_t &index, string &readrate) {
}

void CameraAndorCCD::update_vsrate(uint16_t &index, float &vsrate) {
}

void CameraAndorCCD::update_gain(uint16_t &index, float &gain) {
}

void CameraAndorCCD::update_adoffset(uint16_t &index) {
}

bool CameraAndorCCD::start_expose(float duration, bool light) {
	return false;
}

void CameraAndorCCD::stop_expose() {
	AbortAcquisition();
}

CameraBase::CAMERA_STATUS CameraAndorCCD::camera_state() {
	CAMERA_STATUS retv(CAMERA_ERROR);

	return retv;
}

CameraBase::CAMERA_STATUS CameraAndorCCD::download_image() {
	return CAMERA_IMGRDY;
}

///////////////////////////////////////////////////////////////////////////////
void CameraAndorCCD::load_parameters() {
	try {
		read_xml(andor_conf, xmlpt_, xml_parser::trim_whitespace);
		BOOST_FOREACH(ptree::value_type const &child, xmlpt_.get_child("")) {
			/* 基本参数 */
			if (boost::iequals(child.first, "Basic")) {
				nfptr_->model   = child.second.get("<xmlattr>.model",  "");
				nfptr_->serno   = child.second.get("<xmlattr>.serial", "");
			}
			else if (boost::iequals(child.first, "Dimension")) {
				nfptr_->sensorW = child.second.get("<xmlattr>.width",  1024);
				nfptr_->sensorH = child.second.get("<xmlattr>.height", 1024);
			}
			else if (boost::iequals(child.first, "Pixel")) {
				nfptr_->pixelX = child.second.get("<xmlattr>.x", 12.0);
				nfptr_->pixelY = child.second.get("<xmlattr>.y", 12.0);
			}
			/**/
		}
	}
	catch(xml_parser_error &ex) {
		init_parameters();
	}
}

/*
 * 使用Andor相机需要调用的功能函数:
 * 1. 初始化
 * Initialize(), GetDetector(), GetHardwareVersion(), GetNumberVSSpeeds(), GetVSSpeed(),
 * GetSoftwareVersion(), GetHSSpeed(), GetNumberHSSpeed()
 * 2. 制冷
 * GetTemperatureRange(), SetTemperature(), CoolerON(), GetTemperature()
 * 3. 曝光参数
 * SetAcquisitionMode(), SetReadoutMode(), SetShutter(), SetExposureTime(), SetTriggerMode(),
 * SetAccumulationCycletime(), SetNumberAccumulations(), SetNumberKinetics(),
 * SetKineticCycletime(), GetAcquisitiontimings(), SetHSSpeed(), SetVSSpeed()
 * 4. 其它
 * 增益； Baseline； Sensor Compensation; EM?
 */
void CameraAndorCCD::init_parameters() {
	int nchannel, nAmp;

	{// 基本参数
		boost::format fmt("%d");
		char str[50];
		int serial;
		GetHeadModel(str);
		GetCameraSerialNumber(&serial);
		GetDetector(&nfptr_->sensorW, &nfptr_->sensorH);
		GetPixelSize(&nfptr_->pixelX, &nfptr_->pixelY);

		xmlpt_.add("Basic.<xmlattr>.model",      nfptr_->model = str);
		xmlpt_.add("Basic.<xmlattr>.serial",     nfptr_->serno = (fmt % serial).str());
		xmlpt_.add("Dimension.<xmlattr>.width",  nfptr_->sensorW);
		xmlpt_.add("Dimension.<xmlattr>.height", nfptr_->sensorH);
		xmlpt_.add("Pixel.<xmlattr>.x",          nfptr_->pixelX);
		xmlpt_.add("Pixel.<xmlattr>.y",          nfptr_->pixelY);
	}

	{// 制冷范围
		int tmin, tmax;
		GetTemperatureRange(&tmin, &tmax);
		xmlpt_.add("Cooler.<xmlattr>.min", tmin);
		xmlpt_.add("Cooler.<xmlattr>.max", tmax);
	}

	{// A/D通道
		boost::format fmt("#%d.<xmlattr>.value");
		int i, bitpix;
		ptree node = xmlpt_.add("ADChannel", "");

		GetNumberADChannels(&nchannel);
		node.add("<xmlattr>.number", nchannel);
		for (i = 0; i < nchannel; ++i) {
			fmt % i;
			GetBitDepth(i, &bitpix);
			node.add(fmt.str(), bitpix);
		}
	}

	{

	}

	{// 行转移速度
		boost::format fmt("#%d.<xmlattr>.value");
		int n, i;
		float speed;
		ptree node = xmlpt_.add("VSRate", "");

		GetNumberVSSpeeds(&n);
		node.add("<xmlattr>.number", n);
		for (i = 0; i < n; ++i) {
			fmt % i;
			GetVSSpeed(i, &speed);
			node.add(fmt.str(), speed);
		}
	}

	{// EM CCD
		AndorCapabilities caps;
		caps.ulSize = sizeof(AndorCapabilities);
		GetCapabilities(&caps);
	}
}
