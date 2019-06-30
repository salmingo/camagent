/*
 * @file CameraBase.cpp 相机基类声明文件, 实现相机统一访问接口
 */

#include <boost/make_shared.hpp>
#include "CameraBase.h"

CameraBase::CameraBase() {
}

CameraBase::~CameraBase() {
}

/////////////////////////////////////////////////////////////////////////////
bool CameraBase::Connect() {
	return false;
}

void CameraBase::DisConnect() {

}

bool CameraBase::IsConnected() {
	return false;
}

bool CameraBase::Expose(float duration, bool light) {
	return false;
}

void CameraBase::AbortExpose() {

}

void CameraBase::RegisterExposeProc(const ExpProcSlot &slot) {
	cbexp_.connect(slot);
}

bool CameraBase::SetCooler(bool onoff, float set) {
	return false;
}

bool CameraBase::SetADChannel(uint16_t index) {
	return false;
}

bool CameraBase::SetReadPort(uint16_t index) {
	return false;
}

bool CameraBase::SetReadRate(uint16_t index) {
	return false;
}

bool CameraBase::SetVSRate(uint16_t index) {
	return false;
}

bool CameraBase::SetGain(uint16_t index) {
	return false;
}

bool CameraBase::SetADCOffset(uint16_t offset) {
	return false;
}

bool CameraBase::SetROI(int &xb, int &yb, int &x, int &y, int &w, int &h) {
	return false;
}

bool CameraBase::SetIP(string const ip, string const mask, string const gw) {
	return false;
}
