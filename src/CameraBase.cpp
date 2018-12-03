/*
 * @file CameraBase.cpp 相机基类声明文件, 实现相机统一访问接口
 */
#include <boost/make_shared.hpp>
#include "CameraBase.h"

CameraBase::CameraBase() {
	nfptr_ = boost::make_shared<DeviceCameraInfo>();
}

CameraBase::~CameraBase() {
	Disconnect();
}

const NFDevCamPtr CameraBase::GetCameraInfo() {
	return nfptr_;
}

bool CameraBase::IsConnected() {
	return nfptr_->connected;
}

bool CameraBase::Connect() {
	if (IsConnected()) return true;
	if (!open_camera()) return false;

	nfptr_->connected = true;
	nfptr_->state = CAMERA_IDLE;
	nfptr_->errcode = 0;
	nfptr_->roi.Reset(nfptr_->sensorW, nfptr_->sensorH);
	nfptr_->AllocImageBuffer();
	thrdCycle_.reset(new boost::thread(boost::bind(&CameraBase::thread_cycle, this)));
	thrdExpose_.reset(new boost::thread(boost::bind(&CameraBase::thread_expose, this)));

	return true;
}

void CameraBase::Disconnect() {
	if (IsConnected()) {
		nfptr_->connected = false;
		if (nfptr_->state >= CAMERA_EXPOSE) AbortExpose();
		exit_thread(thrdExpose_);
		exit_thread(thrdCycle_);
		SetCooler(0.0, false);
		close_camera();
	}
}

bool CameraBase::SetCooler(float coolerset, bool onoff) {
	if (!nfptr_->connected) return false;
	if (onoff != nfptr_->coolerOn && coolerset != nfptr_->coolerSet) {
		cooler_onoff(coolerset, onoff);
		nfptr_->coolerSet = coolerset;
		nfptr_->coolerOn  = onoff;
	}
	return true;
}

bool CameraBase::SetADChannel(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iADChannel) {
		uint16_t bitpix = nfptr_->ADBitPixel;
		update_adchannel(index, nfptr_->ADBitPixel);
		nfptr_->iADChannel = index;
		if (bitpix != nfptr_->ADBitPixel) nfptr_->AllocImageBuffer();
	}
	return true;
}

bool CameraBase::SetReadPort(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iReadPort) {
		update_readport(index, nfptr_->ReadPort);
		nfptr_->iReadPort = index;
	}
	return true;
}

bool CameraBase::SetReadRate(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iReadRate) {
		update_readrate(index, nfptr_->ReadRate);
		nfptr_->iReadRate = index;
	}
	return 1;
}

bool CameraBase::SetVSRate(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iVSRate) {
		update_vsrate(index, nfptr_->VSRate);
		nfptr_->iVSRate = index;
	}
	return 1;
}

bool CameraBase::SetGain(uint16_t index) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (index != nfptr_->iGain) {
		update_gain(index, nfptr_->gain);
		nfptr_->iGain = index;
	}
	return 1;
}

bool CameraBase::SetADCOffset(uint16_t offset) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	update_adoffset(offset);
	return true;
}

bool CameraBase::SetROI(int &xb, int &yb, int &x, int &y, int &w, int &h) {
	return false;
}

bool CameraBase::Expose(float duration, bool light) {
	if (!nfptr_->connected || nfptr_->state != CAMERA_IDLE) return false;
	if (!start_expose(duration, light)) return false;
	nfptr_->BeginExpose(duration);
	cvexp_.notify_one();
	nfptr_->FormatUTC();

	return true;
}

void CameraBase::AbortExpose() {
	if (nfptr_->state > CAMERA_IDLE) stop_expose();
}

bool CameraBase::SoftwareReboot() {
	return false;
}

void CameraBase::RegisterExposeProc(const CBSlot &slot) {
	if (!exproc_.empty()) exproc_.disconnect_all_slots();
	exproc_.connect(slot);
}

void CameraBase::SetIP(string const ip, string const mask, string const gw) {
	// 空, 由继承类实现
}

void CameraBase::thread_cycle() {
	boost::chrono::seconds period(10);

	while(1) {
		boost::this_thread::sleep_for(period);
		nfptr_->coolerGet = sensor_temperature();
	}
}

void CameraBase::thread_expose() {
	boost::mutex mtx;
	mutex_lock lck(mtx);
	double left, percent;
	int ms;
	CAMERA_STATUS &stat = nfptr_->state;

	while(1) {
		cvexp_.wait(lck); // 等待曝光开始时的通知
		// 监测曝光过程
		while((stat = camera_state()) == CAMERA_EXPOSE) {
			nfptr_->CheckExpose(left, percent);
			if (left > 0.1) ms = 100;
			else ms = int(left * 1000);
			exproc_(left, percent, (int) stat);
			if (ms > 0) boost::this_thread::sleep_for(boost::chrono::milliseconds(ms));
		}
		/*
		 * 此时stat有三种可能:
		 * - CAMERA_IMGRDY: 正常结束, 可以读出图像数据
		 * - CAMERA_IDLE:   异常结束, 但设备无错误. 原因可能是中止曝光
		 * - CAMERA_ERROR:  异常结束, 设备出现错误
		 */
		if (stat == CAMERA_IMGRDY) {
			nfptr_->EndExpose();
			exproc_(0.0, 100.000001, (int) CAMERA_EXPOSE);
			stat = download_image();
		}
		exproc_(0.0, 100.001, (int) stat);
		if (stat == CAMERA_IMGRDY) stat = CAMERA_IDLE;
	}
}

void CameraBase::exit_thread(threadptr &thrd) {
	if (thrd.unique()) {
		thrd->interrupt();
		thrd->join();
		thrd.reset();
	}
}
