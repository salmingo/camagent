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
