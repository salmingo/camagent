/*
 * @file CameraGY.cpp 港宇公司定制GWAC CCD相机控制接口
 * @version 0.2
 * @date Apr 9, 2017
 * @author Xiaomeng Lu
 */

#include "CameraGY.h"
#include <JAI/Jai_Factory.h>
#include <boost/lexical_cast.hpp>
using namespace boost;

///////////////////////////////////////////////////////////////////////////////
/* definitions of Gangyu Camera Controller */
/* Nov 10, 2016 */
/* Parameter and Command */
#define NODE_NAME_WIDTH		(int8_t*) "Width"
#define NODE_NAME_HEIGHT	(int8_t*) "Height"
#define NODE_NAME_ACQSTART	(int8_t*) "AcquisitionStart"
#define NODE_NAME_ACQSTOP	(int8_t*) "AcquisitionStop"

#define CCD_GAIN_SET		0x00020008	// Gain. 0: Normal; 1: 2 times; 2: 3 times
#define CCD_EXPOSURE		0x00020010	// exposure time, in micro seconds
#define CCD_SHUTTER_MODE	0x0002000C	// shutter mode. 0: Normal; 1: Always Open; 2: Always Closed
#define CCD_EXP_CNTDWN		0x00020024	// Count down during exposure
#define CCD_TRIG_IMG		0x00020000	// Trigger one image
#define CCD_TEST_IMG		0x00020004	// Test image

bool gComplete;			//< 曝光结束标志
J_tIMAGE_INFO* gImage;	// 图像数据指针

void StreamCallback(J_tIMAGE_INFO *pInfo);
///////////////////////////////////////////////////////////////////////////////

CameraGY::CameraGY(const string camIP) {
	camIP_    = camIP;
	hFactory_ = NULL;
	hCamera_  = NULL;
	hStream_  = NULL;
	state_    = -1;
	aborted_  = false;

	duration_ = 0;
	shtrmode_ = 0;
	gain_     = 0;
}

CameraGY::~CameraGY() {
}

bool CameraGY::OpenCamera() {
	state_    = -1;
	aborted_  = false;
	J_STATUS_TYPE errcode;
	// try to connect camera
	if (J_Factory_Open(NULL, &hFactory_) != J_ST_SUCCESS) {
		nfcam_->errmsg = "J_Factory_Open() failed";
		return false;
	}
	bool8_t bChanged;
	if (J_Factory_UpdateCameraList(hFactory_, &bChanged) != J_ST_SUCCESS) {
		nfcam_->errmsg = "J_Factory_UpdateCameraList() failed";
		return false;
	}
	uint32_t i, n;
	if ((errcode = J_Factory_GetNumOfCameras(hFactory_, &n)) != J_ST_SUCCESS || n == 0) {
		nfcam_->errmsg = "J_Factory_GetNumOfCameras() failed. ErrorCode: " + lexical_cast<string>(errcode);
		return false;
	}
	int8_t cameraID[512];
	uint32_t size1 = sizeof(cameraID);
	int8_t ip[20];
	uint32_t size2 = sizeof(ip);
	for (i = 0; i < n; ++i) {
		J_Factory_GetCameraIDByIndex(hFactory_, i, cameraID, &size1);
		J_Factory_GetCameraInfo(hFactory_, cameraID, CAM_INFO_IP, ip, &size2);
		if (strcmp(camIP_.c_str(), (char*)ip) == 0) break;	// found camera of same IP
	}
	if (i == n) {
		nfcam_->errmsg = "Not found camera on IP " + camIP_;
		return false;
	}
	if ((errcode = J_Camera_Open(hFactory_, cameraID, &hCamera_)) != J_ST_SUCCESS) {
		nfcam_->errmsg = "J_Camera_Open() failed. ErrorCode: " + lexical_cast<string>(errcode);
		return false;
	}

	// update width and height
	int64_t temp1;
	nfcam_->model = "GYCAM-" + camIP_;
	J_Camera_GetValueInt64(hCamera_, NODE_NAME_WIDTH, &temp1);
	nfcam_->wsensor = (int) temp1;
	J_Camera_GetValueInt64(hCamera_, NODE_NAME_HEIGHT, &temp1);
	nfcam_->hsensor = (int) temp1;
	// try to create stream
	void *cbfunc = reinterpret_cast<void*>(StreamCallback);
	J_IMG_CALLBACK_FUNCTION *cbptr = reinterpret_cast<J_IMG_CALLBACK_FUNCTION*>(&cbfunc);
	if (J_Image_OpenStream(hCamera_, 0, NULL, *cbptr, &hStream_, nfcam_->wsensor * nfcam_->hsensor * 10) != J_ST_SUCCESS) {
		nfcam_->errmsg = "J_Image_OpenStream() failed";
		return false;
	}

	uint32_t temp2;
	if (RegDataRead(CCD_EXPOSURE, temp2))     duration_ = temp2;
	if (RegDataRead(CCD_SHUTTER_MODE, temp2)) shtrmode_ = temp2;
	if (RegDataRead(CCD_GAIN_SET, temp2))     gain_     = temp2;

	J_Camera_ExecuteCommand(hCamera_, NODE_NAME_ACQSTART);
	state_    = 0;
	gComplete = false;
	gImage    = NULL;

	return true;
}

void CameraGY::CloseCamera() {
	if (hCamera_) J_Camera_ExecuteCommand(hCamera_, NODE_NAME_ACQSTOP);
	if (hStream_) J_Image_CloseStream(hStream_);
	if (hCamera_) J_Camera_Close(hCamera_);
	if (hFactory_) J_Factory_Close(hFactory_);
}

void CameraGY::CoolerOnOff(double& coolerset, bool& onoff) {
	coolerset = -40.0;
}

void CameraGY::UpdateReadPort(int& index) {
	//...无
}

void CameraGY::UpdateReadRate(int& index) {
	//...无
}

void CameraGY::UpdateGain(int& index) {
	if (index != gain_ && RegDataWrite(CCD_GAIN_SET, index)) gain_ = index;
}

void CameraGY::UpdateROI(int& xbin, int& ybin, int& xstart, int& ystart, int& width, int& height) {
	//...暂停用ROI
	xbin = ybin = 1;
	xstart = ystart = 1;
	width = nfcam_->wsensor;
	height= nfcam_->hsensor;
}

double CameraGY::SensorTemperature() {
	return -40.0;
}

bool CameraGY::StartExpose(double duration, bool light) {
	if (!nfcam_->connected) return false;
	int mode = light ? 0 : 2;
	uint32_t t = uint32_t(duration * 1E6);
	boost::chrono::milliseconds twait(100);

	if (t != duration_) {
		if (!RegDataWrite(CCD_EXPOSURE, t)) {
			nfcam_->errmsg = "Failed to set exposure duration";
			return false;
		}
		else boost::this_thread::sleep_for(twait);
		duration_ = t;
	}
	if (shtrmode_ != mode) {
		if (!RegDataWrite(CCD_SHUTTER_MODE, (uint32_t) mode)) {
			nfcam_->errmsg = "Failed to set shutter mode";
			return false;
		}
		else boost::this_thread::sleep_for(twait);
		shtrmode_ = mode;
	}
	if (!RegDataWrite(CCD_TRIG_IMG, 1)) {
		nfcam_->errmsg = "Failed to acquire image";
		return false;
	}
	// 更新状态
	state_    = 1;
	aborted_  = false;
	gComplete = false;
	gImage    = NULL;

	return true;
}

void CameraGY::StopExpose() {
	aborted_  = true;
}

int CameraGY::CameraState() {
	if (gComplete) state_ = aborted_ ? -1 : 2;
	return state_;
}

void CameraGY::DownloadImage() {
	if (state_ == 2) memcpy(nfcam_->data.get(), gImage->pImageBuffer, sizeof(uint16_t) * nfcam_->wsensor * nfcam_->hsensor);
	state_ = 0;
}

uint32_t CameraGY::swap32(uint32_t val) {// big-endian <-> little-endian
	uint32_t v0 = (val & 0x000000FF) << 24;
	uint32_t v1 = (val & 0x0000FF00) << 8;
	uint32_t v2 = (val & 0x00FF0000) >> 8;
	uint32_t v3 = (val & 0xFF000000) >> 24;

	return (uint32_t) (v0 | v1 | v2 | v3);
}

bool CameraGY::RegDataRead(uint64_t addr, uint32_t &value) {// Read data from camera address
	if (!nfcam_->connected) return false;

	uint32_t toread;
	uint32_t size = sizeof(uint32_t);
	J_STATUS_TYPE retcode;

	if ((retcode = J_Camera_ReadData(hCamera_, addr, (void*)&toread, &size)) == J_ST_SUCCESS)
		value = swap32(toread);
	else nfcam_->errmsg = "J_Camera_ReadData() failed";

	return retcode == J_ST_SUCCESS;
}

bool CameraGY::RegDataWrite(uint64_t addr, uint32_t value) {// Write data to camera address
	if (!nfcam_->connected) return false;

	uint32_t towrite = swap32(value);
	uint32_t size = sizeof(uint32_t);
	J_STATUS_TYPE retcode;

	if ((retcode = J_Camera_WriteData(hCamera_, addr, (const void*)&towrite, &size)) != J_ST_SUCCESS)
		nfcam_->errmsg = "J_Camera_WriteData() failed";

	return retcode == J_ST_SUCCESS;
}

void StreamCallback(J_tIMAGE_INFO *pInfo) {// callback function
	gComplete = true;
	gImage = pInfo;
}

