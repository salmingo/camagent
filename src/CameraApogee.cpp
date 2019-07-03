/*
 * @file CameraApogee.cpp Apogee CCD相机控制接口
 */

#include <apogee/FindDeviceUsb.h>
#include <apogee/FindDeviceEthernet.h>
#include <apogee/CameraInfo.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <stdio.h>
#include "apgSampleCmn.h"
#include "CameraApogee.h"

using namespace std;
using namespace boost::property_tree;

const char apogee_conf[] = "/usr/local/etc/apogee.xml";	// 相机性能参数列表

CameraApogee::CameraApogee() {

}

CameraApogee::~CameraApogee() {
	data_.clear();
}

bool CameraApogee::open_camera() {
	if (!load_parameters()) return false;

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

bool CameraApogee::update_adchannel(uint16_t &index, uint16_t &bitpix) {
	bitpix = 16;
	return true;
}

bool CameraApogee::update_readport(uint16_t &index, string &readport) {
	index = 0;
	readport = "Conventional";
	return true;
}

bool CameraApogee::update_readrate(uint16_t &index, string &readrate) {
	readrate = "Normal";
	return true;
}

bool CameraApogee::update_vsrate(uint16_t &index, float &vsrate) {
	vsrate = 0.0;
	return true;
}

bool CameraApogee::update_gain(uint16_t &index, float &gain) {
	gain = nfptr_->gain;
	return true;
}

bool CameraApogee::update_adoffset(uint16_t &index) {
	return true;
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
	if (find_camera_usb() || find_camera_lan()) {// 找到相机时, 生成配置文件

	}
	return false;
}

bool CameraApogee::find_camera_usb() {
	return false;
}

bool CameraApogee::find_camera_lan() {
	return false;
}

/*
 * 查找相机并初始化参数
 */
void CameraApogee::init_parameters() {
	ptree pt;
	uint16_t id0, id1(0xFFFF);
	float pixelX, pixelY, gain;

	read_xml(apogee_conf, pt, xml_parser::trim_whitespace);
	id0 = pt.get("ID.<xmlattr>.value", 0);
	// 解析相机出厂配置文件
	string filepath = "/usr/local/etc/Apogee/camera/apnmatrix.txt"; // 厂商配置文件
	FILE *fp;
	boost::shared_array<char> line;
	char *token;
	char seps[] = "\t";
	int szline(2100), pos(0);

	if (NULL == (fp = fopen(filepath.c_str(), "r")))
		return ;
	line.reset(new char[szline]);
	fgets(line.get(), szline, fp); // 空读一行
	while (!feof(fp) && id0 != id1) {
		if (NULL == fgets(line.get(), szline, fp)) continue;
		pos = 0;
		token = strtok(line.get(), seps);
		while (token && ++pos <= 28) {
			if (pos == 3) {
				if (id0 != (id1 = uint16_t(atoi(token)))) break;
			}
			else if (pos == 25) pixelX = atof(token);
			else if (pos == 26) pixelY = atof(token);
			else if (pos == 28) gain   = atof(token);

			token = strtok(NULL, seps);
		}
	}
	fclose(fp);
	// 构建ptree并保存文件
	pt.add("PixelSize.<xmlattr>.x",     pixelX);
	pt.add("PixelSize.<xmlattr>.y",     pixelY);
	pt.add("Gain.<xmlattr>.value",      gain);

	xml_writer_settings<std::string> settings(' ', 4);
	write_xml(apogee_conf, pt, std::locale(), settings);
}

bool CameraApogee::load_parameters() {
	try {
		string interface, addr;
		uint16_t frmwr, id;
		// 访问配置文件加载参数
		ptree pt;

		read_xml(apogee_conf, pt, xml_parser::trim_whitespace);
		nfptr_->model  = pt.get("Camera.<xmlattr>.model", "");
		nfptr_->serno  = pt.get("Camera.<xmlattr>.serno", "");
		nfptr_->pixelX = pt.get("PixelSize.<xmlattr>.x",  0.0);
		nfptr_->pixelY = pt.get("PixelSize.<xmlattr>.y",  0.0);
		nfptr_->gain   = pt.get("Gain.<xmlattr>.value",   0.0);
		interface = pt.get("Interface.<xmlattr>.value", "usb");
		addr      = pt.get("Address.<xmlattr>.value",   "");
		frmwr     = pt.get("FrmwrRev.<xmlattr>.value",  0);
		id        = pt.get("ID.<xmlattr>.value",        0);
		// 尝试连接相机
		altacam_ = boost::make_shared<Alta>();
		altacam_->OpenConnection(interface, addr, frmwr, id);
		return altacam_->IsConnected();
	}
	catch(xml_parser_error &ex) {
		if (find_camera()) {
			init_parameters();
			return true;
		}
		return false;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}
