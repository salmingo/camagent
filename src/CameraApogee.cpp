/*
 * @file CameraApogee.cpp Apogee CCD相机控制接口
 */

#include <apogee/FindDeviceUsb.h>
#include <apogee/FindDeviceEthernet.h>
#include <apogee/CameraInfo.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "CameraApogee.h"

using namespace std;
using namespace boost::property_tree;

const char apogee_conf[] = "/usr/local/etc/apogee.xml";	// 相机性能参数列表

/////////////////////////////////////////////////////////////////////////////
// MAKE	  TOKENS
vector<string> MakeTokens(const string &str, const string &separator) {
	vector<string> returnVector;
	string::size_type start = 0;
	string::size_type end = 0;

	while( (end = str.find(separator, start)) != string::npos) {
		returnVector.push_back (str.substr (start, end-start));
		start = end + separator.size();
	}
	returnVector.push_back( str.substr(start) );

	return returnVector;
}

///////////////////////////
//	GET    ITEM    FROM     FIND       STR
string GetItemFromFindStr( const string & msg, const string & item ) {
	//search the single device input string for the requested item
	vector<string> params = MakeTokens( msg, "," );
	vector<string>::iterator iter;

	for(iter = params.begin(); iter != params.end(); ++iter) {
		if( string::npos != (*iter).find( item ) ) {
			string result =  MakeTokens( (*iter), "=" ).at(1);
			return result;
		}
	} //for

	string noOp;
	return noOp;
}

////////////////////////////
//	GET		USB  ADDRESS
string GetUsbAddress( const string & msg ) {
	return GetItemFromFindStr( msg, "address=" );
}

////////////////////////////
//	GET		ETHERNET  ADDRESS
string GetEthernetAddress( const string & msg ) {
	string addr = GetItemFromFindStr( msg, "address=" );
	addr.append(":");
	addr.append( GetItemFromFindStr( msg, "port=" ) );
	return addr;
}
////////////////////////////
//	GET		ID
uint16_t GetID( const string & msg ) {
	string str = GetItemFromFindStr( msg, "id=" );
	uint16_t id = 0;
	stringstream ss;
	ss << std::hex << std::showbase << str.c_str();
	ss >> id;

    return id;
}

////////////////////////////
//	GET		FRMWR       REV
uint16_t GetFrmwrRev( const string & msg) {
	string str = GetItemFromFindStr(  msg, "firmwareRev=" );
	uint16_t rev = 0;
	stringstream ss;
	ss << std::hex << std::showbase << str.c_str();
	ss >> rev;

	return rev;
}
/////////////////////////////////////////////////////////////////////////////

CameraApogee::CameraApogee() {
	frmwr_   = 0;
	id_      = 0;
}

CameraApogee::~CameraApogee() {
	data_.clear();
}

bool CameraApogee::open_camera() {
	if (!load_parameters()) return false;

	try {
		/*-------------------------------------------------------------------*/
		/* 尝试连接相机 */
		altacam_ = boost::make_shared<Alta>();
		altacam_->OpenConnection(iotype_, addr_, frmwr_, id_);
		if (!altacam_->IsConnected()) return false;
		/*-------------------------------------------------------------------*/
		/* 相机连接后应为IDLE态, 但调试发现可能处于Flusing. 需等待转入IDLE */
		boost::chrono::seconds duration(1);
		int count(0);
		do {// 尝试多次初始化
			if (count) boost::this_thread::sleep_for(duration);
			altacam_->Init();
		} while(++count <= 10 && camera_state() > CAMERA_IDLE);
		/*-------------------------------------------------------------------*/
		/* 完成最后的初始化 */
		if (camera_state() == CAMERA_ERROR) {
			altacam_->CloseConnection();
			nfptr_->errmsg = "Wrong initial camera status";
		}
		else {
			nfptr_->sensorW = altacam_->GetMaxImgCols();
			nfptr_->sensorH = altacam_->GetMaxImgRows();
			data_.resize(nfptr_->sensorW * nfptr_->sensorH);
		}
		return true;
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
		data_.resize(w * h / xb / yb);
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
	float val(1E30);
	try {
		val = altacam_->GetTempCcd();
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
	}
	return val;
}

bool CameraApogee::update_adchannel(uint16_t &index, uint16_t &bitpix) {
	index  = 0;
	bitpix = 16;
	return true;
}

bool CameraApogee::update_readport(uint16_t &index, string &readport) {
	index    = 0;
	readport = "Conventional";
	return true;
}

bool CameraApogee::update_readrate(uint16_t &index, string &readrate) {
	index    = 0;
	readrate = "Normal";
	return true;
}

bool CameraApogee::update_vsrate(uint16_t &index, float &vsrate) {
	index  = 0;
	vsrate = 0.0;
	return true;
}

bool CameraApogee::update_gain(uint16_t &index, float &gain) {
	index = 0;
	gain  = nfptr_->gain;
	return true;
}

bool CameraApogee::update_adoffset(uint16_t value) {
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
	string desc;
	bool isUsb(false), found(false);

	if ((isUsb = find_camera_usb(desc)) || find_camera_lan(desc)) {
		found = true;
		// 解析相机标志字符串
		iotype_= GetItemFromFindStr(desc, "interface=");
		frmwr_ = GetFrmwrRev(desc);
		id_    = GetID(desc);
		if (isUsb) addr_ = GetUsbAddress(desc);
		else       addr_ = GetEthernetAddress(desc);
		// 记录配置文件
		ptree pt;
		pt.add("Interface.<xmlattr>.value", iotype_);
		pt.add("Address.<xmlattr>.value",   addr_);
		pt.add("FrmwrRev.<xmlattr>.value",  frmwr_);
		pt.add("ID.<xmlattr>.value",        id_);
		// 尝试连接相机
		altacam_ = boost::make_shared<Alta>();
		altacam_->OpenConnection(iotype_, addr_, frmwr_, id_);
		if (altacam_->IsConnected()) {
			altacam_->CloseConnection();
			pt.add("Camera.<xmlattr>.model", altacam_->GetModel());
		}
		xml_writer_settings<string> settings(' ', 4);
		write_xml(apogee_conf, pt, std::locale(), settings);
	}
	return found;
}

bool CameraApogee::find_camera_usb(string &desc) {
	FindDeviceUsb lookcam;
	desc = lookcam.Find();
	return desc.size();
}

bool CameraApogee::find_camera_lan(string &desc) {
	FindDeviceEthernet lookcam;
	// 构建subset
	ifaddrs *ifaddr, *ifa;
	string subnet;
	if (!getifaddrs(&ifaddr)) {
		for (ifa = ifaddr; ifa != NULL && 0 == desc.size(); ifa = ifa->ifa_next) {// 只采用IPv4地址
			if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
			if (!strcmp(ifa->ifa_name, "lo")) continue;
			subnet = inet_ntoa(((struct sockaddr_in*) ifa->ifa_broadaddr)->sin_addr);
			desc = lookcam.Find(subnet);
		}
		freeifaddrs(ifaddr);
	}
	return desc.size();
}

/*
 * 查找相机并初始化参数
 */
void CameraApogee::init_parameters() {
	// 解析相机出厂配置文件
	string filepath = "/usr/local/etc/Apogee/camera/apnmatrix.txt"; // 厂商配置文件
	FILE *fp;
	boost::shared_array<char> line;
	char *token;
	char seps[] = "\t";
	int szline(2100), pos(0);
	uint16_t id(0xFFFF);
	float pixelX, pixelY, gain;

	if (NULL == (fp = fopen(filepath.c_str(), "r")))
		return ;
	line.reset(new char[szline]);
	fgets(line.get(), szline, fp); // 空读一行
	while (!feof(fp) && id_ != id) {
		if (NULL == fgets(line.get(), szline, fp)) continue;
		pos = 0;
		token = strtok(line.get(), seps);
		while (token && ++pos <= 28) {
			if (pos == 3) {
				if (id_ != (id = uint16_t(atoi(token)))) break;
			}
			else if (pos == 25) pixelX = atof(token);
			else if (pos == 26) pixelY = atof(token);
			else if (pos == 28) gain   = atof(token);

			token = strtok(NULL, seps);
		}
	}
	fclose(fp);
	// 构建ptree并保存文件
	ptree pt;
	read_xml(apogee_conf, pt, xml_parser::trim_whitespace);
	pt.add("PixelSize.<xmlattr>.x",  pixelX);
	pt.add("PixelSize.<xmlattr>.y",  pixelY);
	pt.add("Gain.<xmlattr>.value",   gain);

	xml_writer_settings<string> settings(' ', 4);
	write_xml(apogee_conf, pt, std::locale(), settings);
}

bool CameraApogee::load_parameters() {
	try {// 访问配置文件加载参数
		ptree pt;
		read_xml(apogee_conf, pt, xml_parser::trim_whitespace);

		iotype_ = pt.get("Interface.<xmlattr>.value", "");
		addr_   = pt.get("Address.<xmlattr>.value",   "");
		frmwr_  = pt.get("FrmwrRev.<xmlattr>.value",  0);
		id_     = pt.get("ID.<xmlattr>.value",        0);
		nfptr_->model  = pt.get("Camera.<xmlattr>.model", "");
		nfptr_->pixelX = pt.get("PixelSize.<xmlattr>.x",  0.0);
		nfptr_->pixelY = pt.get("PixelSize.<xmlattr>.y",  0.0);
		nfptr_->gain   = pt.get("Gain.<xmlattr>.value",   0.0);
		return true;
	}
	catch(xml_parser_error &ex) {
		if (find_camera()) init_parameters();
		return false;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}
