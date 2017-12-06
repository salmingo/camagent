/*
 * CameraPICCD.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: lxm
 */

#include "CameraPICCD.h"

CameraPICCD::CameraPICCD() {
}

CameraPICCD::~CameraPICCD() {
}


bool CameraPICCD::connect() {
	return false;
}

void CameraPICCD::disconnect() {

}

bool CameraPICCD::start_expose(double duration, bool light) {
	return false;
}

void CameraPICCD::stop_expose() {

}

int CameraPICCD::check_state() {
	return 0;
}

int CameraPICCD::readout_image() {
	return 0;
}

double CameraPICCD::sensor_temperature() {
	return 0.0;
}

void CameraPICCD::update_cooler(double &coolset, bool onoff) {

}

uint32_t CameraPICCD::update_readport(uint32_t index) {
	return 0;
}

uint32_t CameraPICCD::update_readrate(uint32_t index) {
	return 0;
}

uint32_t CameraPICCD::update_gain(uint32_t index) {
	return 0;
}

void CameraPICCD::update_roi(int &xstart, int &ystart, int &width, int &height, int &xbin, int &ybin) {

}

void CameraPICCD::update_adcoffset(uint16_t offset) {

}
