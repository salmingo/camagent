/*
 * @file CameraAndorCCD.cpp Andor CCD相机控制接口
 * @version 0.2
 */

#include "CameraAndorCCD.h"

CameraAndorCCD::CameraAndorCCD() {
}

CameraAndorCCD::~CameraAndorCCD() {
}

bool CameraAndorCCD::OpenCamera() {
	return true;
}

void CameraAndorCCD::CloseCamera() {

}

void CameraAndorCCD::CoolerOnOff(double& coolerset, bool& onoff) {

}

void CameraAndorCCD::UpdateReadPort(int& index) {

}

void CameraAndorCCD::UpdateReadRate(int& index) {

}

void CameraAndorCCD::UpdateGain(int& index) {

}

void CameraAndorCCD::UpdateROI(int& xbin, int& ybin, int& xstart, int& ystart, int& width, int& height) {

}

double CameraAndorCCD::SensorTemperature() {
	return 0.0;
}

bool CameraAndorCCD::StartExpose(double duration, bool light) {
	return false;
}

void CameraAndorCCD::StopExpose() {

}

int CameraAndorCCD::CameraState() {
	return -1;
}

void CameraAndorCCD::DownloadImage() {

}
