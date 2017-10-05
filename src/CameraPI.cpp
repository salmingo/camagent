/*
 * @file CameraPI.cpp PI CCD相机控制接口
 * @version 0.3
 */

#include "CameraPI.h"

CameraPI::CameraPI() {

}

CameraPI::~CameraPI() {
}

bool CameraPI::OpenCamera() {
	return false;
}

void CameraPI::CloseCamera() {

}

void CameraPI::CoolerOnOff(double& coolerset, bool& onoff) {

}

void CameraPI::UpdateReadPort(int& index) {

}

void CameraPI::UpdateReadRate(int& index) {

}

void CameraPI::UpdateGain(int& index) {

}

void CameraPI::UpdateROI(int& xbin, int& ybin, int& xstart, int& ystart, int& width, int& height) {

}

double CameraPI::SensorTemperature() {
	return 0.0;
}

bool CameraPI::StartExpose(double duration, bool light) {
	return true;
}

void CameraPI::StopExpose() {

}

int CameraPI::CameraState() {
	return -1;
}

void CameraPI::DownloadImage() {

}
