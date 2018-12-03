/*
 * @file CameraGY.cpp GWAC定制相机(重庆港宇公司研发电控系统)控制接口定义文件
 */

#include "CameraGY.h"

CameraGY::CameraGY(string const camIP)
	: portCamera_(3956)
	, portLocal_(49152)
	, idLeader_(0x1)
	, idTrailer_(0x2)
	, idPayload_(0x3) {
	camIP_ = camIP;
}

CameraGY::~CameraGY() {
}

void CameraGY::SetIP(string const ip, string const mask, string const gw) {

}

bool CameraGY::SoftwareReboot() {

}

bool CameraGY::open_camera() {

}

void CameraGY::close_camera() {

}

void CameraGY::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {

}

void CameraGY::cooler_onoff(float &coolset, bool &onoff) {

}

float CameraGY::sensor_temperature() {
	return nfptr_->coolerGet;
}

void CameraGY::update_adchannel(uint16_t &index, uint16_t &bitpix) {
	index = 0;
	bitpix= 16;
}

void CameraGY::update_readport(uint16_t &index, string &readport) {
	index = 0;
	readport = "Conventional";
}

void CameraGY::update_readrate(uint16_t &index, string &readrate) {
	index    = 0;
	readrate = "1 MHz";
}

void CameraGY::update_vsrate(uint16_t &index, float &vsrate) {
	index = 0;
	vsrate= 0.0;
}

void CameraGY::update_gain(uint16_t &index, float &gain) {
	if (index <= 2) {

	}
}

void CameraGY::update_adoffset(uint16_t &index) {

}

bool CameraGY::start_expose(float duration, bool light) {

}

void CameraGY::stop_expose() {

}

CAMERA_STATUS CameraGY::camera_state() {

}

CAMERA_STATUS CameraGY::download_image() {

}

uint16_t CameraGY::msg_count() {
	if (++msgcnt_ == 0) msgcnt_ = 1;
	return msgcnt_;
}

void CameraGY::reg_write(uint32_t addr, uint32_t val) {
	mutex_lock lck(mtx_reg_);
	boost::array<uint8_t, 16> buff1 = {0x42, 0x01, 0x00, 0x82, 0x00, 0x08};
	int n;

	((uint16_t*) &buff1)[3] = htons(msg_count());
	((uint32_t*) &buff1)[2] = htonl(addr);
	((uint32_t*) &buff1)[3] = htonl(val);
	udpcmd_->Write(buff1.c_array(), buff1.size());
	const uint8_t *buff2 = udpcmd_->BlockRead(n);

	if (n != 12 || buff2[11] != 0x01) {
		char txt[200];
		int i;
		int n1 = sprintf(txt, "length<%d> of write register<%0X>: ", n, addr);
		for (i = 0; i < n; ++i) n1 += sprintf(txt + n1, "%02X ", buff2[i]);
		throw std::runtime_error(txt);
	}
}

void CameraGY::reg_read(uint32_t addr, uint32_t &val) {
	mutex_lock lck(mtx_reg_);
	boost::array<uint8_t, 12> buff1 = {0x42, 0x01, 0x00, 0x80, 0x00, 0x04};
	int n;

	((uint16_t*) &buff1)[3] = htons(msg_count());
	((uint32_t*) &buff1)[2] = htonl(addr);
	udpcmd_->Write(buff1.c_array(), buff1.size());
	const uint8_t *buff2 = udpcmd_->BlockRead(n);
	if (n == 12) val = ntohl(((uint32_t*)buff2)[2]);
	else {
		char txt[200];
		int n1 = sprintf(txt, "length<%d> of read register<%0X>: ", n, addr);
		for (int i = 0; i < n; ++i) n1 += sprintf(txt + n1, "%02X ", buff2[i]);
		throw std::runtime_error(txt);
	}
}
