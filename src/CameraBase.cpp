/*
 * @file CameraBase.cpp 相机基类声明文件, 实现相机统一访问接口
 */
#include <boost/make_shared.hpp>
#include "CameraBase.h"

CameraBase::CameraBase() {
	nfptr_ = boost::make_shared<DeviceCameraInfo>();
}

CameraBase::~CameraBase() {
}

const CameraBase::NFDevCamPtr CameraBase::GetCameraInfo() {
	return nfptr_;
}

bool CameraBase::IsConnected() {
	return nfptr_->connected;
}

bool CameraBase::Connect() {
	if (IsConnected()) return true;
	if (!OpenCamera()) return false;

	nfptr_->connected = true;
	nfptr_->state = CAMERA_IDLE;

	return true;
}

void CameraBase::Disconnect() {
	if (IsConnected()) {

	}
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

void CameraBase::RegisterExposeProc(const CBSlot &slot) {
	if (!exposeproc_.empty()) exposeproc_.disconnect_all_slots();
	exposeproc_.connect(slot);
}
