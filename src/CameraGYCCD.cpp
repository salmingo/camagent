/*
 * CameraGYCCD.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: lxm
 */

#include "CameraGYCCD.h"

CameraGYCCD::CameraGYCCD(const char *camIP) {

}

CameraGYCCD::~CameraGYCCD() {
}

bool CameraGYCCD::connect() {
	return false;
}

void CameraGYCCD::disconnect() {

}

bool CameraGYCCD::start_expose(double duration, bool light) {
	return false;
}

void CameraGYCCD::stop_expose() {

}

int CameraGYCCD::check_state() {
	return 0;
}

int CameraGYCCD::readout_image() {
	return 0;
}

double CameraGYCCD::sensor_temperature() {
	return 0.0;
}

void CameraGYCCD::update_cooler(double &coolset, bool onoff) {

}

uint32_t CameraGYCCD::update_readport(uint32_t index) {
	return 0;
}

uint32_t CameraGYCCD::update_readrate(uint32_t index) {
	return 0;
}

uint32_t CameraGYCCD::update_gain(uint32_t index) {
	return 0;
}

void CameraGYCCD::update_roi(int &xstart, int &ystart, int &width, int &height, int &xbin, int &ybin) {

}

void CameraGYCCD::update_adcoffset(uint16_t offset) {

}
