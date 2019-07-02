/*
 * CameraAndorCCD.cpp Andor CCD相机的实现文件
 * @date 2015-09-22
 * @version 0.1
 * @author 卢晓猛
 */

#include <unistd.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "CameraAndorCCD.h"

using namespace std;
using namespace boost::property_tree;

const char andor_conf[] = "/usr/local/etc/andor.xml";	// 相机性能参数列表
const char andor_dir[]  = "/usr/local/etc/andor";		// 初始化连接的目录

CameraAndorCCD::CameraAndorCCD() {
	shtropening_ = 50;
	shtrclosing_ = 50;
}

CameraAndorCCD::~CameraAndorCCD() {
}

ptree &CameraAndorCCD::GetParameters() {
	return xmlpt_;
}

bool CameraAndorCCD::UpdateEMGain(uint16_t gain) {
	if (!CameraBase::UpdateEMGain(gain)) return false;
	if (!(nfptr_->EMCCD && nfptr_->EMOn)) return false;
	if (gain == nfptr_->EMGain) return true;

	uint16_t low  = xmlpt_.get("EMCCD.<xmlattr>.gain_low",  0);
	uint16_t high = xmlpt_.get("EMCCD.<xmlattr>.gain_high", 0);
	if (gain < low || gain > high) return false;

	if (DRV_SUCCESS == SetEMCCDGain(gain)) {
		nfptr_->EMGain = gain;
		return true;
	}
	return false;
}

bool CameraAndorCCD::open_camera() {
	if (Initialize((char*) andor_dir) != DRV_SUCCESS) return false;
	if (!load_parameters()) {// 参数加载失败后将初始化配置文件. 初始化后用户需做定制修改
		ShutDown();
		return false;
	}
	int rslt = DRV_SUCCESS;
	rslt |= SetReadMode(4); // image
	rslt |= SetImage(1, 1, 1, nfptr_->sensorW, 1, nfptr_->sensorH);
	rslt |= SetAcquisitionMode(1); // single scan

	return rslt == DRV_SUCCESS;
}

bool CameraAndorCCD::close_camera() {
	return DRV_SUCCESS == ShutDown();
}

bool CameraAndorCCD::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {
	int active = w * h / xb / yb != nfptr_->sensorW * nfptr_->sensorH ? 1 : 0;
	return DRV_SUCCESS == SetIsolatedCropModeEx(active, h, w, yb, xb, x, y);
}

bool CameraAndorCCD::cooler_onoff(float &coolset, bool &onoff) {
	bool cmd = onoff;
	if (onoff) {
		int set = int(coolset);
		int tmin = xmlpt_.get("Cooler.<xmlattr>.min", 0);
		int tmax = xmlpt_.get("Cooler.<xmlattr>.max", 0);

		if (set < tmin) set = tmin;
		else if (set > tmax) set = tmax;
		if (DRV_SUCCESS == SetTemperature(set)) coolset = float(set);
		onoff = DRV_SUCCESS == CoolerON();
	}
	else onoff = !(DRV_SUCCESS == CoolerOFF());
	return cmd == onoff;
}

float CameraAndorCCD::sensor_temperature() {
	int val;
	GetTemperature(&val);
	return float(val);
}

bool CameraAndorCCD::update_adchannel(uint16_t &index, uint16_t &bitpix) {
	uint16_t n = xmlpt_.get("ADChannel.<xmlattr>.number", 0);
	if (index < n
		&& DRV_SUCCESS == SetADChannel(index)) {
		boost::format fmt("ADChannel.#%d.<xmlattr>.value");
		bitpix = xmlpt_.get(fmt.str(), 0);
		return true;
	}
	return false;
}

bool CameraAndorCCD::update_readport(uint16_t &index, string &readport) {
	uint16_t n = xmlpt_.get("ReadPort.<xmlattr>.number", 0);
	if (index < n
		&& DRV_SUCCESS == SetOutputAmplifier(index)) {
		boost::format fmt("ReadPort.#%d.<xmlattr>.name");
		fmt % index;
		readport = xmlpt_.get(fmt.str(), "");
		/* EMCCD: 附加判断 */
		if (nfptr_->EMCCD) nfptr_->EMOn = index == 0;

		return true;
	}
	return false;
}

bool CameraAndorCCD::update_readrate(uint16_t &index, string &readrate) {
	boost::format fmt1("ReadRate#%d-%d.<xmlattr>.number");
	uint16_t n;
	fmt1 % nfptr_->iADChannel % nfptr_->iReadPort;
	n = xmlpt_.get(fmt1.str(), 0);
	if (index < n &&
		DRV_SUCCESS == SetHSSpeed(nfptr_->iReadPort, index)) {
		boost::format fmt2("ReadRate#d-%d.#d.<xmlattr>.value");
		fmt2 % nfptr_->iADChannel % nfptr_->iReadPort % index;
		readrate = xmlpt_.get(fmt2.str(), "");
		return true;
	}
	return false;
}

bool CameraAndorCCD::update_vsrate(uint16_t &index, float &vsrate) {
	uint16_t n = xmlpt_.get("VSRate.<xmlattr>.number", 0);
	if (index < n
		&& DRV_SUCCESS == SetVSSpeed(index)) {
		boost::format fmt("VSRate.#d.<xmlattr>.value");
		fmt % index;
		vsrate = xmlpt_.get(fmt.str(), 0.0);
		return true;
	}
	return false;
}

bool CameraAndorCCD::update_gain(uint16_t &index, float &gain) {
	boost::format fmt1("PreAmpGain#%d-%d-%d.<xmlattr>.number");
	uint16_t n;
	fmt1 % nfptr_->iADChannel % nfptr_->iReadPort % nfptr_->iReadRate;
	n = xmlpt_.get(fmt1.str(), 0);
	if (index < n
		&& DRV_SUCCESS == SetPreAmpGain(index)) {
		boost::format fmt2("PreAmpGain#%d-%d-%d.#d.<xmlattr>.value");
		fmt2 % nfptr_->iADChannel % nfptr_->iReadPort % nfptr_->iReadRate % index;
		gain = xmlpt_.get(fmt2.str(), 0.0);
		return true;
	}
	return false;
}

bool CameraAndorCCD::update_adoffset(uint16_t value) {
	value = value / 100 * 100;
	return DRV_SUCCESS == SetBaselineOffset(value);
}

bool CameraAndorCCD::start_expose(float duration, bool light) {
	int mode = light ? 0 : 2;
	return (   DRV_SUCCESS == SetShutter(1, mode, shtrclosing_, shtropening_)
			&& DRV_SUCCESS == SetExposureTime(duration)
			&& DRV_SUCCESS == StartAcquisition());
}

bool CameraAndorCCD::stop_expose() {
	return DRV_SUCCESS == AbortAcquisition();
}

CameraBase::CAMERA_STATUS CameraAndorCCD::camera_state() {
	int status;
	if (DRV_SUCCESS != GetStatus(&status)) return CAMERA_ERROR;
	CAMERA_STATUS oldt(nfptr_->state);
	CAMERA_STATUS newt(CAMERA_EXPOSE);

	if (status == DRV_IDLE) {
		if (oldt == CAMERA_EXPOSE)    newt = CAMERA_IMGRDY;
	}
	else if (status != DRV_ACQUIRING) newt = CAMERA_ERROR;

	return newt;
}

CameraBase::CAMERA_STATUS CameraAndorCCD::download_image() {
	CAMERA_STATUS state(CAMERA_ERROR);
	unsigned long pixels = nfptr_->roi.Pixels();
	uint8_t *buff = nfptr_->data.get();
	if ((nfptr_->bitpixel <= 16 && DRV_SUCCESS == GetAcquiredData16((uint16_t*)buff, pixels))
		|| (nfptr_->bitpixel > 16 && DRV_SUCCESS == GetAcquiredData((int*)buff, pixels))) {
		state = CAMERA_IMGRDY;
	}
	return state;
}

///////////////////////////////////////////////////////////////////////////////
bool CameraAndorCCD::load_parameters() {
	try {
		read_xml(andor_conf, xmlpt_, xml_parser::trim_whitespace);
		BOOST_FOREACH(ptree::value_type const &child, xmlpt_.get_child("")) {
			/* 基本参数 */
			if (boost::iequals(child.first, "Basic")) {
				nfptr_->model   = child.second.get("<xmlattr>.model",  "");
				nfptr_->serno   = child.second.get("<xmlattr>.serial", "");
			}
			else if (boost::iequals(child.first, "Dimension")) {
				nfptr_->sensorW = child.second.get("<xmlattr>.width",  1024);
				nfptr_->sensorH = child.second.get("<xmlattr>.height", 1024);
			}
			else if (boost::iequals(child.first, "Pixel")) {
				nfptr_->pixelX = child.second.get("<xmlattr>.x", 12.0);
				nfptr_->pixelY = child.second.get("<xmlattr>.y", 12.0);
			}
			/* EMCCD */
			else if (boost::iequals(child.first, "EMCCD")) {
				nfptr_->EMCCD = child.second.get("<xmlattr>.support", false);
			}
		}

		return true;
	}
	catch(xml_parser_error &ex) {
		init_parameters();
		return false;
	}
}

void CameraAndorCCD::init_parameters() {
	int nchannel, nAmp;

	{// 基本参数
		boost::format fmt("%d");
		char str[50];
		int serial;
		GetHeadModel(str);
		GetCameraSerialNumber(&serial);
		GetDetector(&nfptr_->sensorW, &nfptr_->sensorH);
		GetPixelSize(&nfptr_->pixelX, &nfptr_->pixelY);

		xmlpt_.add("Basic.<xmlattr>.model",      str);
		xmlpt_.add("Basic.<xmlattr>.serial",     (fmt % serial).str());
		xmlpt_.add("Dimension.<xmlattr>.width",  nfptr_->sensorW);
		xmlpt_.add("Dimension.<xmlattr>.height", nfptr_->sensorH);
		xmlpt_.add("Pixel.<xmlattr>.x",          nfptr_->pixelX);
		xmlpt_.add("Pixel.<xmlattr>.y",          nfptr_->pixelY);
	}

	{// 制冷范围
		int tmin, tmax;
		GetTemperatureRange(&tmin, &tmax);
		xmlpt_.add("Cooler.<xmlattr>.min", tmin);
		xmlpt_.add("Cooler.<xmlattr>.max", tmax);
	}

	{// A/D通道
		boost::format fmt("#%d.<xmlattr>.value");
		int bitpix;
		ptree node = xmlpt_.add("ADChannel", "");

		GetNumberADChannels(&nchannel);
		node.add("<xmlattr>.number", nchannel);
		for (int i = 0; i < nchannel; ++i) {
			fmt % i;
			GetBitDepth(i, &bitpix);
			node.add(fmt.str(), bitpix);
		}
	}

	{// 读出端口
		ptree node = xmlpt_.add("ReadPort", "");
		boost::format fmt("#%d.<xmlattr>.name");
		const int len = 30;
		char name[len];

		GetNumberAmp(&nAmp);
		node.add("<xmlattr>.number", nAmp);
		for (int i = 0; i < nAmp; ++i) {
			fmt % i;
			GetAmpDesc(i, name, len);
			node.add(fmt.str(), name);
		}
	}

	{// 读出速度
		int ic, ir, i, n;
		float rate;
		ptree node;
		boost::format fmt1("ReadRate#%d-%d");
		boost::format fmt2("#d.<xmlattr>.value");
		boost::format fmt3("%f MHz");

		for (ic = 0; ic < nchannel; ++ic) {// 遍历AD通道
			for (ir = 0; ir < nAmp; ++ir) {// 遍历读出端口
				GetNumberHSSpeeds(ic, ir, &n);
				fmt1 % ic % ir;
				node = xmlpt_.add(fmt1.str(), "");
				node.add("<xmlattr>.number", n);
				for (i = 0; i < n; ++i) {// 遍历读出速度
					GetHSSpeed(ic, ir, i, &rate);
					fmt2 % i;
					fmt3 % rate;
					node.add(fmt2.str(), fmt3.str());
				}
			}
		}
	}

	{// 前置增益
		boost::format fmt1("PreAmpGain#%d-%d-%d");
		boost::format fmt2("#d.<xmlattr>.avail");
		boost::format fmt3("#d.<xmlattr>.value");
		ptree node;
		int nRate, nGain, avail, ic, ip, ir, ig;
		float gain;

		GetNumberPreAmpGains(&nGain);
		xmlpt_.add("PreAmpGain.<xmlattr>.number", nGain);
		for (ic = 0; ic < nchannel; ++ic) {// 遍历: AD通道
			SetADChannel(ic); // 设置AD通道
			for (ip = 0; ip < nAmp; ++ip) {// 遍历: 读出端口
				SetOutputAmplifier(ip); // 设置读出端口
				GetNumberHSSpeeds(ic, ip, &nRate);
				for (ir = 0; ir < nRate; ++ir) {// 遍历: 读出速度
					SetVSSpeed(ir); // 设置读出速度
					fmt1 % ic % ip % ir;
					node = xmlpt_.add(fmt1.str(), "");
					for (ig = 0; ig < nGain; ++ig) {// 遍历: 前置增益
						fmt2 % ig;
						fmt3 % ig;
						IsPreAmpGainAvailable(ic, ip, ir, ig, &avail);
						GetPreAmpGain(ig, &gain);
						node.add(fmt2.str(), bool(avail));
						node.add(fmt3.str(), gain);
					}
				}
			}
		}
	}

	{// 行转移速度
		boost::format fmt("#%d.<xmlattr>.value");
		int n, i;
		float speed;
		ptree node = xmlpt_.add("VSRate", "");
		// 数量
		GetNumberVSSpeeds(&n);
		node.add("<xmlattr>.number", n);
		// 建议的最快转移速度
		GetFastestRecommendedVSSpeed(&i, &speed);
		node.add("recommend.<xmlattr>.index", i);
		node.add("recommend.<xmlattr>.value", speed);
		// 各个档位对应的速度
		for (i = 0; i < n; ++i) {
			fmt % i;
			GetVSSpeed(i, &speed);
			node.add(fmt.str(), speed);
		}
	}

	{// EM CCD
		AndorCapabilities caps;
		caps.ulSize = sizeof(AndorCapabilities);
		GetCapabilities(&caps);
		if (caps.ulCameraType == 3) {
			int low, high;
			GetEMGainRange(&low, &high);
			xmlpt_.add("EMCCD.<xmlattr>.support",   true);
			xmlpt_.add("EMCCD.<xmlattr>.gain_low",  low);
			xmlpt_.add("EMCCD.<xmlattr>.gain_high", high);
		}
	}

	{// 快门时间
		xmlpt_.add("ShutterTime.<xmlattr>.Open",  50);
		xmlpt_.add("ShutterTime.<xmlattr>.Close", 50);
	}
}
