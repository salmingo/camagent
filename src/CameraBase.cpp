/*
 * @file CameraBase.cpp 相机基类声明文件, 实现相机统一访问接口
 */
#include <boost/make_shared.hpp>
#include "CameraBase.h"

CameraBase::CameraBase() {
	connected_ = false;
	state_ = CAMERA_ERROR;
	wsensor_ = hsensor_ = 0;
}

CameraBase::~CameraBase() {
}

bool CameraBase::IsConnected() {
	return connected_;
}

bool CameraBase::Connect() {
	if (connected_) return true;
	if (!OpenCamera()) return false;

	connected_ = true;
	state_ = CAMERA_IDLE;

	int n = (wsensor_ * hsensor_ * 2 + 15) & ~15;
	data_.reset(new uint8_t[n]);

	return true;
}

void CameraBase::Disconnect() {

}

double CameraBase::SetCooler(double coolerset, bool onoff) {
	return coolerset;
}

uint16_t CameraBase::SetReadPort(uint16_t index) {
	return 0;
}

uint16_t CameraBase::SetReadRate(uint16_t index) {
	return 0;
}

uint16_t CameraBase::SetGain(uint16_t index) {
	return 0;
}

void CameraBase::SetROI(int &xbin, int &ybin, int &xstart, int &ystart, int &width, int &height) {
}

uint16_t CameraBase::SetADCOffset(uint16_t offset) {
	return 0;
}

bool CameraBase::Expose(double duration, bool light) {
	return false;
}

void CameraBase::AbortExpose() {

}

bool CameraBase::SoftwareReboot() {
	return false;
}
