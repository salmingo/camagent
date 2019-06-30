/*
 * CameraAndorCCD.cpp Andor CCD相机的实现文件
 * @date 2015-09-22
 * @version 0.1
 * @author 卢晓猛
 *
 * @date 2018-12-04
 * @version 0.2
 * @author 卢晓猛
 */

#include <unistd.h>
#include <boost/property_tree/xml_parser.hpp>
#include "CameraAndorCCD.h"

using namespace std;
using namespace boost::property_tree;

char andor_conf[] = "/usr/local/etc/andor.conf";	// 相机性能参数列表
char andor_dir[]  = "/usr/local/etc/andor";			// 初始化连接的目录

CameraAndorCCD::CameraAndorCCD() {
}

CameraAndorCCD::~CameraAndorCCD() {
}

bool CameraAndorCCD::SetIP(string const ip, string const mask, string const gw) {
	return true;
}

bool CameraAndorCCD::SoftwareReboot() {
	return false;
}

bool CameraAndorCCD::open_camera() {
	if (Initialize(andor_dir) != DRV_SUCCESS) return false;

	AndorCapabilities caps;
	caps.ulSize = sizeof(AndorCapabilities);
	GetCapabilities(&caps);

	boost::format fmt("%d");
	char str[50];
	int serial;
	GetHeadModel(str);              nfptr_->model = str;
	GetCameraSerialNumber(&serial); nfptr_->serialno = (fmt % serial).str();
	GetDetector(&nfptr_->sensorW, &nfptr_->sensorH);
	GetPixelSize(&nfptr_->pixelX, &nfptr_->pixelY);

	return true;
}

void CameraAndorCCD::close_camera() {
	ShutDown();
}

void CameraAndorCCD::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {
}

void CameraAndorCCD::cooler_onoff(float &coolset, bool &onoff) {
	if (onoff) {
		int tmin, tmax, set = int(coolset);
		GetTemperatureRange(&tmin, &tmax);
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

CAMERA_STATUS CameraAndorCCD::camera_state() {
	CAMERA_STATUS retv(CAMERA_ERROR);

	return retv;
}

CAMERA_STATUS CameraAndorCCD::download_image() {
	return CAMERA_IMGRDY;
}

///////////////////////////////////////////////////////////////////////////////
void CameraAndorCCD::load_parameters() {
	read_xml(andor_conf, ptree_, xml_parser::trim_whitespace);
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
	if (!access(andor_conf, F_OK)) return;


}
