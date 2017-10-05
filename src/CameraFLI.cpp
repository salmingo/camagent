/*
 * @file CameraFLI.cpp FLI CCD相机控制接口
 * @version 0.2
 */

#include "CameraFLI.h"

CameraFLI::CameraFLI() {
}

CameraFLI::~CameraFLI() {
}

bool CameraFLI::OpenCamera() {
	return false;
}

void CameraFLI::CloseCamera() {

}

void CameraFLI::CoolerOnOff(double& coolerset, bool& onoff) {

}

void CameraFLI::UpdateReadPort(int& index) {

}

void CameraFLI::UpdateReadRate(int& index) {

}

void CameraFLI::UpdateGain(int& index) {

}

void CameraFLI::UpdateROI(int& xbin, int& ybin, int& xstart, int& ystart, int& width, int& height) {

}

double CameraFLI::SensorTemperature() {
	return 0.0;
}

bool CameraFLI::StartExpose(double duration, bool light) {
	return true;
}

void CameraFLI::StopExpose() {

}

int CameraFLI::CameraState() {
	return -1;
}

void CameraFLI::DownloadImage() {

}
