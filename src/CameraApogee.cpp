/*
 * @file CameraApogee.cpp Apogee CCD相机控制接口
 */

#include <apogee/FindDeviceUsb.h>
#include <apogee/FindDeviceEthernet.h>
#include <apogee/CameraInfo.h>
#include "apgSampleCmn.h"
#include "CameraApogee.h"

using namespace std;

const char apogee_conf[] = "/usr/local/etc/apogee.xml";	// 相机性能参数列表

CameraApogee::CameraApogee() {

}

CameraApogee::~CameraApogee() {
	data_.clear();
}

bool CameraApogee::open_camera() {
	if (!(load_parameters() || find_camera())) return false;

	try {
		if (altacam_->IsConnected()) {
			boost::chrono::seconds duration(1);
			int count(0);	//< Apogee CCD连接后状态可能不对, 不能启动曝光. 后台建立尝试机制
			// 相机初始状态为Status_Flushing时, 才可以正确启动曝光流程
			do {// 尝试多次初始化
				if (count) boost::this_thread::sleep_for(duration);
				altacam_->Init();
			} while(++count <= 10 && camera_state() > CAMERA_IDLE);

			if (camera_state() == CAMERA_ERROR) {
				altacam_->CloseConnection();
				nfptr_->errmsg = "Wrong initial camera status";
			}
			else {
				nfptr_->model = altacam_->GetModel();
				nfptr_->serno = altacam_->GetSerialNumber();
				nfptr_->SetSensorDimension(altacam_->GetMaxImgCols(), altacam_->GetMaxImgRows());
				nfptr_->AllocImageBuffer();
				data_.resize(nfptr_->roi.Width() * nfptr_->roi.Height());
			}
		}
		return altacam_->IsConnected();
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
/*
	try {
		string ioInterface("usb");
		FindDeviceUsb lookcam;
		string msg, addr;
		uint16_t id, frmwr;
		int count(0);	//< Apogee CCD连接后状态可能不对, 不能启动曝光. 后台建立尝试机制
		boost::chrono::seconds duration(1);

		altacam_ = boost::make_shared<Alta>();
		msg      = lookcam.Find();
		addr     = apgSampleCmn::GetUsbAddress(msg);
		id       = apgSampleCmn::GetID(msg);
		frmwr    = apgSampleCmn::GetFrmwrRev(msg);

		altacam_->OpenConnection(ioInterface, addr, frmwr, id);
		if (altacam_->IsConnected()) {
			// 相机初始状态为Status_Flushing时, 才可以正确启动曝光流程
			do {// 尝试多次初始化
				if (count) boost::this_thread::sleep_for(duration);
				altacam_->Init();
			} while(++count <= 10 && camera_state() > CAMERA_IDLE);

			if (camera_state() == CAMERA_ERROR) {
				altacam_->CloseConnection();
				nfptr_->errmsg = "Wrong initial camera status";
			}
			else {
				nfptr_->model = altacam_->GetModel();
				nfptr_->serno = altacam_->GetSerialNumber();
				nfptr_->SetSensorDimension(altacam_->GetMaxImgCols(), altacam_->GetMaxImgRows());
				nfptr_->AllocImageBuffer();
				data_.resize(nfptr_->roi.Width() * nfptr_->roi.Height());
			}
		}
		return altacam_->IsConnected();
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
*/
}

bool CameraApogee::close_camera() {
	altacam_->CloseConnection();
	return true;
}

bool CameraApogee::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {
	try {
		altacam_->SetRoiBinCol(xb);
		altacam_->SetRoiBinRow(yb);
		altacam_->SetRoiStartCol(x - 1);
		altacam_->SetRoiStartRow(y - 1);
		altacam_->SetRoiNumCols(w);
		altacam_->SetRoiNumRows(h);

		xb = altacam_->GetRoiBinCol();
		yb = altacam_->GetRoiBinRow();
		x  = altacam_->GetRoiStartCol() + 1;
		y  = altacam_->GetRoiStartRow() + 1;
		w  = altacam_->GetRoiNumCols();
		h  = altacam_->GetRoiNumRows();
		nfptr_->AllocImageBuffer();
		return true;
	}
	catch(std::runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

bool CameraApogee::cooler_onoff(bool &onoff, float &coolset) {
	try {
		altacam_->SetCooler(onoff);
		if (onoff) altacam_->SetCoolerSetPoint(coolset);
		coolset = altacam_->GetCoolerSetPoint();
		onoff = altacam_->IsCoolerOn();
		return true;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

float CameraApogee::sensor_temperature() {
	float val(0.0);
	try {
		val = altacam_->GetTempCcd();
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
	}
	return val;
}

/*
 * AltaU支持两种A/D转换:
 * - 16bit, index自定义为0. 此为默认模式
 * - 12bit, index自定义为1
 */
bool CameraApogee::update_adchannel(uint16_t &index, uint16_t &bitpix) {
	try {
		if (index == 0) {
			altacam_->SetCcdAdcResolution(Apg::Resolution_SixteenBit);
			bitpix = 16;
			nfptr_->iReadRate = 0;
			nfptr_->readrate  = "Normal";
		}
		else if (index == 1) {
			altacam_->SetCcdAdcResolution(Apg::Resolution_TwelveBit);
			bitpix = 12;
			nfptr_->iReadRate = 1;
			nfptr_->readrate  = "Fast";
		}
		return true;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

bool CameraApogee::update_readport(uint16_t &index, string &readport) {
	index = 0;
	readport = "Conventional";
	return true;
}

bool CameraApogee::update_readrate(uint16_t &index, string &readrate) {
	try {
		if (index == 0) {
			altacam_->SetCcdAdcSpeed(Apg::AdcSpeed_Normal);
			readrate = "Normal";
			nfptr_->iADChannel = 0;
			nfptr_->bitpixel = 16;
		}
		else if (index == 1) {
			altacam_->SetCcdAdcSpeed(Apg::AdcSpeed_Fast);
			readrate = "Fast";
			nfptr_->iADChannel = 1;
			nfptr_->bitpixel = 12;
		}
		return true;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

bool CameraApogee::update_vsrate(uint16_t &index, float &vsrate) {
	index = 0;
	vsrate= 0.0;
	return true;
}

/*
 * index与16bit/12bit A/D通道对应.
 * - 0: 16bit
 * - 1: 12bit
 * 对于AltaU相机, gain有效区间为[0, 1023]
 */
bool CameraApogee::update_gain(uint16_t &index, float &gain) {
	try {
//		altacam_->SetAdcGain(index, nfptr_->iADChannel, 0);
		gain = altacam_->GetAdcGain(nfptr_->iADChannel, 0);
		return true;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

bool CameraApogee::update_adoffset(uint16_t &index) {
	try {
//		altacam_->SetAdcOffset(index, nfptr_->iADChannel, 0);
		return true;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

bool CameraApogee::start_expose(float duration, bool light) {
	try {
		altacam_->StartExposure(duration, light);
		return true;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

bool CameraApogee::stop_expose() {
	try {
		altacam_->StopExposure(false);
		return true;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

CameraBase::CAMERA_STATUS CameraApogee::camera_state() {
	CAMERA_STATUS retv(CAMERA_ERROR);
	try {
		Apg::Status status = altacam_->GetImagingStatus();

		if (status == Apg::Status_Exposing || status == Apg::Status_ImagingActive)
			retv = CAMERA_EXPOSE;
		else if (status == Apg::Status_ImageReady) retv = CAMERA_IMGRDY;
		else if (status >= 0) retv = CAMERA_IDLE;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
	}
	return retv;
}

CameraBase::CAMERA_STATUS CameraApogee::download_image() {
	try {
		int n = nfptr_->roi.Pixels();
		altacam_->GetImage(data_);
		memcpy(nfptr_->data.get(), data_.data(), n * sizeof(unsigned short));
		return nfptr_->state;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return CAMERA_ERROR;
	}
}

bool CameraApogee::find_camera() {
	return false;
}

bool CameraApogee::find_camera_usb() {
	return false;
}

bool CameraApogee::find_camera_lan() {
	return false;
}

void CameraApogee::init_parameters() {
	string filepath = "/usr/local/etc/Apogee/camera/apnmatrix.txt"; // 厂商配置文件
}

bool CameraApogee::load_parameters() {
	return false;
}
