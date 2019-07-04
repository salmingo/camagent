/*
 * @file CameraBase.cpp 相机基类声明文件, 实现相机统一访问接口
 */

#include <boost/make_shared.hpp>
#include "CameraBase.h"

CameraBase::CameraBase() {
	nfptr_ = boost::make_shared<CameraInfo>();
}

CameraBase::~CameraBase() {
	DisConnect();
}

/////////////////////////////////////////////////////////////////////////////
CameraBase::NFCamPtr CameraBase::GetCameraInfo() {
	return nfptr_;
}

bool CameraBase::Connect() {
	if (IsConnected()) return true;
	if (!open_camera()) return false;

	nfptr_->connected = true;
	nfptr_->state     = CAMERA_IDLE;
	nfptr_->errcode   = 0;
	nfptr_->errmsg    = "";
	nfptr_->AllocImageBuffer();
	thrdcool_.reset(new boost::thread(boost::bind(&CameraBase::thread_cool, this)));
	thrdexp_.reset(new boost::thread(boost::bind(&CameraBase::thread_expose, this)));

	return true;
}

void CameraBase::DisConnect() {
	if (IsConnected()) {
		if (nfptr_->state >= CAMERA_EXPOSE) {// 等待完成或结束曝光
			AbortExpose();
			while (nfptr_->state >= CAMERA_EXPOSE) {
				boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
			}
		}
		int_thread(thrdexp_);
		int_thread(thrdcool_);
		UpdateCooler(false);
		close_camera();
		nfptr_->connected = false;
	}
}

bool CameraBase::IsConnected() {
	return nfptr_->connected;
}

bool CameraBase::Expose(float duration, bool light) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (!start_expose(duration, light)) return false;
	nfptr_->ExposeBegin(duration);
	cvexp_.notify_one();
	return true;
}

void CameraBase::AbortExpose() {
	if (nfptr_->state > CAMERA_IDLE) stop_expose();
}

void CameraBase::RegisterExposeProc(const ExpProcSlot &slot) {
	cbexp_.connect(slot);
}

bool CameraBase::UpdateCooler(bool onoff, float set) {
	if (!nfptr_->connected) return false;
	if ((onoff != nfptr_->coolOn || (onoff && set != nfptr_->coolSet))
		&& cooler_onoff(onoff, set)) {
		nfptr_->coolSet = set;
		nfptr_->coolOn  = onoff;
		return true;
	}
	return false;
}

bool CameraBase::UpdateADChannel(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iADChannel) {
		uint16_t bitpix = nfptr_->bitpixel;
		if (update_adchannel(index, nfptr_->bitpixel)) {
			nfptr_->iADChannel = index;
			if (bitpix != nfptr_->bitpixel)
				nfptr_->AllocImageBuffer();
			return true;
		}
	}
	return false;
}

bool CameraBase::UpdateReadPort(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iReadPort
		&& update_readport(index, nfptr_->readport)) {
		nfptr_->iReadPort = index;
		return true;
	}
	return false;
}

bool CameraBase::UpdateReadRate(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iReadRate
		&& update_readrate(index, nfptr_->readrate)) {
		nfptr_->iReadRate = index;
		return true;
	}
	return false;
}

bool CameraBase::UpdateVSRate(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iVSRate
		&& update_vsrate(index, nfptr_->vsrate)) {
		nfptr_->iVSRate = index;
		return true;
	}
	return false;
}

bool CameraBase::UpdateGain(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iGain && update_gain(index, nfptr_->gain)) {
		nfptr_->iGain = index;
		return true;
	}
	return false;
}

bool CameraBase::UpdateEMGain(uint16_t gain) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	return true;
}

bool CameraBase::UpdateADCOffset(uint16_t offset) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	return update_adoffset(offset);
}

bool CameraBase::UpdateROI(int &xb, int &yb, int &x, int &y, int &w, int &h) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	/* 检查: 参数有效性 */
	int x1, y1, res;
	if (xb <= 0 || xb > 16) xb = 1;
	if (yb <= 0 || yb > 16) yb = 1;
	if (x <= 0 || x > nfptr_->sensorW) x = 1;
	else if ((res = (x - 1) % xb)) x -= xb;
	if (y <= 0 || y > nfptr_->sensorH) y = 1;
	else if ((res = (y - 1) % yb)) y -= yb;
	x1 = x + w - 1;
	y1 = y + h - 1;
	if (x1 <= 0 || (x1 - x + 1) / xb < 1 || x1 > nfptr_->sensorW) x1 = nfptr_->sensorW / xb * xb;
	else if ((res = x1 % xb)) x1 -= xb;
	if (y1 <= 0 || (y1 - y + 1) / yb < 1 || y1 > nfptr_->sensorH) y1 = nfptr_->sensorH / yb * yb;
	else if ((res = y1 % yb)) y1 -= yb;
	w = x1 - x + 1;
	h = y1 - y + 1;
	if (w == 0) { x = 1, w = nfptr_->sensorW / xb * xb; }
	if (h == 0) { y = 1, h = nfptr_->sensorH / yb * yb; }

	/* 设置ROI */
	if (update_roi(xb, yb, x, y, w, h)) {
		nfptr_->roi.binX = xb;
		nfptr_->roi.binY = yb;
		nfptr_->roi.startX = x;
		nfptr_->roi.startY = y;
		nfptr_->roi.width  = w;
		nfptr_->roi.height = h;
		nfptr_->AllocImageBuffer();
		return true;
	}

	return false;
}

bool CameraBase::UpdateIP(string const ip, string const mask, string const gw) {
	return (nfptr_->connected && nfptr_->state == CAMERA_IDLE);
}

void CameraBase::thread_cool() {
	boost::chrono::seconds T(30);

	while(1) {
		boost::this_thread::sleep_for(T);
		nfptr_->CoolGet = sensor_temperature();
	}
}

void CameraBase::thread_expose() {
	boost::mutex mtx;
	mutex_lock lck(mtx);
	double left, percent;
	int milsec;
	CAMERA_STATUS &state = nfptr_->state;

	while (1) {
		cvexp_.wait(lck); // 等待曝光开始
		/* 监测曝光过程 */
		while ((state = camera_state()) == CAMERA_EXPOSE) {
			nfptr_->CheckExpose(left, percent);
			if ((milsec = int(left * 1000)) > 100) milsec = 100;
			cbexp_(left, percent, (int) state);
			if (milsec) boost::this_thread::sleep_for(boost::chrono::milliseconds(milsec));
		}
		/*
		 * 此时state有三种可能:
		 * - CAMERA_IMGRDY: 正常结束, 可以读出图像数据
		 * - CAMERA_IDLE  : 异常结束, 但设备无错误. 原因可能是因为中止曝光
		 * - CAMERA_ERROR : 异常结束, 设备错误
		 */
		if (state == CAMERA_IMGRDY) {
			nfptr_->ExposeEnd();
			state = download_image();
		}
		cbexp_(0.0, 100.001, (int) state);
		// 图像成功读出, 将相机状态设置为空闲
		if (state == CAMERA_IMGRDY) state = CAMERA_IDLE;
	}
}

void CameraBase::int_thread(threadptr &thrd) {
	if (thrd.unique()) {
		thrd->interrupt();
		thrd->join();
		thrd.reset();
	}
}
