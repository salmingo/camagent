/*
 * CameraFLICCD.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: lxm
 */

#include "CameraFLICCD.h"

CameraFLICCD::CameraFLICCD() {

}

CameraFLICCD::~CameraFLICCD() {
}

bool CameraFLICCD::connect() {
	return false;
}

void CameraFLICCD::disconnect() {

}

bool CameraFLICCD::start_expose(double duration, bool light) {
	return false;
}

void CameraFLICCD::stop_expose() {

}

int CameraFLICCD::check_state() {
	return 0;
}

int CameraFLICCD::readout_image() {
	return 0;
}

double CameraFLICCD::sensor_temperature() {
	return 0.0;
}

void CameraFLICCD::update_cooler(double &coolset, bool onoff) {

}

uint32_t CameraFLICCD::update_readport(uint32_t index) {
	return 0;
}

uint32_t CameraFLICCD::update_readrate(uint32_t index) {
	return 0;
}

uint32_t CameraFLICCD::update_gain(uint32_t index) {
	return 0;
}

void CameraFLICCD::update_roi(int &xstart, int &ystart, int &width, int &height, int &xbin, int &ybin) {

}

void CameraFLICCD::update_adcoffset(uint16_t offset) {

}
