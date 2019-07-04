/*!
 * @file CameraFLICCD.cpp
 * @version 0.2
 * @date 2019-07-04
 */

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "CameraFLICCD.h"

using namespace boost::property_tree;

const char fliccd_conf[] = "/usr/local/etc/fliccd.xml";	// 相机性能参数列表

CameraFLICCD::CameraFLICCD() {
	hcam_ = 0;
}

CameraFLICCD::~CameraFLICCD() {
}

bool CameraFLICCD::open_camera() {
	return false;
}

bool CameraFLICCD::close_camera() {
	return false;
}

bool CameraFLICCD::cooler_onoff(bool &onoff, float &coolset) {
	return false;
}

float CameraFLICCD::sensor_temperature() {
	return 0.0;
}

bool CameraFLICCD::update_adchannel(uint16_t &index, uint16_t &bitpix) {
	return false;
}

bool CameraFLICCD::update_readport(uint16_t &index, string &readport) {
	return false;
}

bool CameraFLICCD::update_readrate(uint16_t &index, string &readrate) {
	return false;
}

bool CameraFLICCD::update_vsrate(uint16_t &index, float &vsrate) {
	return false;
}

bool CameraFLICCD::update_gain(uint16_t &index, float &gain) {
	return false;
}

bool CameraFLICCD::update_adoffset(uint16_t value) {
	return false;
}

bool CameraFLICCD::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {
	return false;
}

bool CameraFLICCD::start_expose(float duration, bool light) {
	return false;
}

bool CameraFLICCD::stop_expose() {
	return false;
}

CameraBase::CAMERA_STATUS CameraFLICCD::camera_state() {
	return CAMERA_ERROR;
}

CameraBase::CAMERA_STATUS CameraFLICCD::download_image() {
	return CAMERA_ERROR;
}

bool CameraFLICCD::load_parameters() {

}

bool CameraFLICCD::init_parameters() {
	flidomain_t domain;
	char fileName[100], devName[100];

	FLICreateList(FLIDOMAIN_USB | FLIDEVICE_CAMERA);
	if(FLIListFirst(&domain, fileName, 200, devName, 200))
		return false;
	FLIDeleteList();

	ptree pt;
	pt.add("Camera.<xmlattr>.filename", fileName);
	pt.add("Camera.<xmlattr>.devName", devName);
}
