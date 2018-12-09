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

#include "CameraAndorCCD.h"
using namespace std;

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
	if (Initialize(andor_dir) == DRV_SUCCESS) {

	}
	return false;
}

void CameraAndorCCD::close_camera() {
}

void CameraAndorCCD::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {
}

void CameraAndorCCD::cooler_onoff(float &coolset, bool &onoff) {
}

float CameraAndorCCD::sensor_temperature() {
	return 0.0;
}

void CameraAndorCCD::update_adchannel(uint16_t &index, uint16_t &bitpix) {
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
}

CAMERA_STATUS CameraAndorCCD::camera_state() {
	CAMERA_STATUS retv(CAMERA_ERROR);

	return retv;
}

CAMERA_STATUS CameraAndorCCD::download_image() {
	return CAMERA_IMGRDY;
}
