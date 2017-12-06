/*
 * CameraAndorCCD.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: lxm
 */

#include "CameraAndorCCD.h"

CameraAndorCCD::CameraAndorCCD() {

}

CameraAndorCCD::~CameraAndorCCD() {
}

bool CameraAndorCCD::connect() {
	return false;
}

void CameraAndorCCD::disconnect() {

}

bool CameraAndorCCD::start_expose(double duration, bool light) {
	return false;
}

void CameraAndorCCD::stop_expose() {

}

int CameraAndorCCD::check_state() {
	return 0;
}

int CameraAndorCCD::readout_image() {
	return 0;
}

double CameraAndorCCD::sensor_temperature() {
	return 0.0;
}

void CameraAndorCCD::update_cooler(double &coolset, bool onoff) {

}

uint32_t CameraAndorCCD::update_readport(uint32_t index) {
	return 0;
}

uint32_t CameraAndorCCD::update_readrate(uint32_t index) {
	return 0;
}

uint32_t CameraAndorCCD::update_gain(uint32_t index) {
	return 0;
}

void CameraAndorCCD::update_roi(int &xstart, int &ystart, int &width, int &height, int &xbin, int &ybin) {

}

void CameraAndorCCD::update_adcoffset(uint16_t offset) {

}
