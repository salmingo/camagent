/*
 * @file CameraGY.cpp GWAC定制相机(重庆港宇公司研发电控系统)控制接口定义文件
 */
#include <sys/types.h>
#include <ifaddrs.h>
#include <math.h>
#include "CameraGY.h"

using namespace std;

CameraGY::CameraGY(string const camIP)
	: portCamera_(3956)
	, portLocal_(49152)
	, idLeader_(0x1)
	, idTrailer_(0x2)
	, idPayload_(0x3)
	, headsize_(8) {
	camIP_     = camIP;
	msgcnt_    = 0;
	nfptr_->model  = "GWAC, E2V CCD";
	nfptr_->pixelX = 12.0;
	nfptr_->pixelY = 12.0;

	// 初始化通信接口
	const UDPSession::CBSlot &slot = boost::bind(&CameraGY::receive_data, this, _1, _2);
	udpdata_ = makeudp_session(portLocal_);
	udpdata_->RegisterRead(slot);
	udpcmd_ = makeudp_session();
	udpcmd_->Connect(camIP.c_str(), portCamera_);
}

CameraGY::~CameraGY() {
}

bool CameraGY::SetIP(string const ip, string const mask, string const gw) {
	if (!CameraBase::UpdateIP(ip, mask, gw)) return false;
	return (update_network(0x064C, ip.c_str())
			&& update_network(0x065C, mask.c_str())
			&& update_network(0x066C, gw.c_str()));
}

bool CameraGY::SoftwareReboot() {
	try {
		reg_write(0x20054, 0x12AB3C4D);
		return true;
	}
	catch(runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

bool CameraGY::open_camera() {
	try {
		// 选择主机IP地址
		uint32_t addrHost = get_hostaddr();
		if (addrHost == 0x00)
			throw runtime_error("no matched host IP address");
		// 检测与相机通信是否正常
		using boost::asio::ip::address_v4;
		boost::array<uint8_t, 8> towrite = {0x42, 0x01, 0x00, 0x02, 0x00, 0x00};
		int bytercv, trycnt(0);
		address_v4 addr1, addr2;
		const uint8_t *rcvd;

		msgcnt_ = 0;
		do {
			((uint16_t*)&towrite)[3] = htons(msg_count());
			udpcmd_->Write(towrite.c_array(), towrite.size());
			rcvd = (const uint8_t *) udpcmd_->BlockRead(bytercv);
		} while (bytercv < 48 && ++trycnt < 3);
		if (bytercv < 48)
			throw runtime_error("failed to communicate with camera");
		addr1 = address_v4(rcvd[47] + uint32_t(rcvd[46] << 8) + uint32_t(rcvd[45] << 16) + uint32_t(rcvd[44] << 24));
		addr2 = address_v4::from_string(camIP_);
		if (addr1 != addr2)
			throw runtime_error("not found camera");

		// 初始化参数
		reg_write(0x0A00,     0x03);		// Set GevCCP
		reg_write(0x0D00,     portLocal_);	// Set GevSCPHostPort
		reg_write(0x0D04,     1500);		// Set PacketSize
		reg_write(0x0D08,     0);			// Set PacketDelay
		reg_write(0x0D18,     addrHost);	// Set GevSCDA
		reg_write(0xA000,     0x01);		// Start AcquisitionSequence
		reg_write(0x0938,     0x2710);		// 心跳延时0x2710==10000ms=10s
		// 初始化监测量
		uint32_t val;
		reg_read(0x00020008,  gain_);
		reg_read(0x0002000C,  shtrmode_);
		reg_read(0x00020010,  expdur_);

		reg_read(0xA004, val);
		nfptr_->sensorW = int(val);
		reg_read(0xA008, val);
		nfptr_->sensorH = int(val);
		byteimg_ = nfptr_->sensorW * nfptr_->sensorH * 2;

		reg_read(0x0D04,     packsize_);
		packsize_ -= (20 + 8 + headsize_);
		packcnt_ = int(ceil(double(byteimg_ + 64) / packsize_)); // 最后一包多出64字节
		packflag_.reset(new uint8_t[packcnt_ + 1]);
		// 启动心跳机制, 维护与相机间的网络连接
		thrdhb_.reset(new boost::thread(boost::bind(&CameraGY::thread_heartbeat, this)));
		thrdread_.reset(new boost::thread(boost::bind(&CameraGY::thread_readout, this)));

		return true;
	}
	catch(std::exception &ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

void CameraGY::close_camera() {
	exit_thread(thrdhb_);
	exit_thread(thrdread_);
}

void CameraGY::update_roi(int &xb, int &yb, int &x, int &y, int &w, int &h) {
	xb = yb = 1;
	x = y = 1;
	w = nfptr_->sensorW;
	h = nfptr_->sensorH;
}

void CameraGY::cooler_onoff(float &coolset, bool &onoff) {
	coolset = nfptr_->coolerSet;
	onoff   = true;
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
	if (index <= 2 && index != gain_) {
		try {
			reg_write(0x00020008, index);
			reg_read(0x00020008, gain_);
			index = gain_;
			gain  = gain_ == 0 ? 1.0 : (gain_ == 1 ? 1.5 : 2.0);
		}
		catch(std::runtime_error& ex) {
			nfptr_->errmsg = ex.what();
		}
	}
	else {
		gain  = gain_ == 0 ? 1.0 : (gain_ == 1 ? 1.5 : 2.0);
	}
}

void CameraGY::update_adoffset(uint16_t &index) {
}

bool CameraGY::start_expose(float duration, bool light) {
	try {
		// 设置环境参数
		bytercd_ = 0;
		idFrame_  = uint16_t(-1);
		idPack_   = uint32_t(-1);
		memset(packflag_.get(), 0, packcnt_ + 1);
		// 设置曝光参数
		uint32_t val;
		if (shtrmode_ != (val = light ? 0 : 2)) {// 设置快门状态后必须等待一定时间
			reg_write(0x0002000C, val);
			boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
			reg_read(0x0002000C, shtrmode_);
		}
		if (expdur_ != (val = uint32_t(duration * 1E6))) {
			reg_write(0x00020010, val);
			reg_read(0x00020010, expdur_);
		}
		reg_write(0x00020000, 0x01);
		cv_waitread_.notify_one();

		return true;
	}
	catch(std::runtime_error &ex) {
		nfptr_->errmsg = ex.what();
		nfptr_->state = CAMERA_ERROR;
		return false;
	}
}

void CameraGY::stop_expose() {
	CAMERA_STATUS &state = nfptr_->state;

	if (state >= CAMERA_EXPOSE) {
		try {// 成功: 状态变为空闲
			reg_write(0x20050, 0x1);
			state = CAMERA_IDLE;
		}
		catch(std::runtime_error& ex) {// 失败: 状态变为错误
			nfptr_->errmsg = ex.what();
			state = CAMERA_ERROR;
		}
		cv_imgrdy_.notify_one();
	}
}

CAMERA_STATUS CameraGY::camera_state() {
	return nfptr_->state;
}

CAMERA_STATUS CameraGY::download_image() {
	boost::mutex tmp;
	mutex_lock lck(tmp);
	cv_imgrdy_.wait(lck); // 等待图像就绪标志

	return nfptr_->state;
}

uint32_t CameraGY::get_hostaddr() {
	ifaddrs *ifaddr, *ifa;
	uint32_t addr(0), addrcam, mask;
	bool found(false);

	inet_pton(AF_INET, camIP_.c_str(), &addrcam);
	if (!getifaddrs(&ifaddr)) {
		for (ifa = ifaddr; ifa != NULL && !found; ifa = ifa->ifa_next) {// 只采用IPv4地址
			if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
			addr = ((struct sockaddr_in*) ifa->ifa_addr)->sin_addr.s_addr;
			mask = ((struct sockaddr_in*) ifa->ifa_netmask)->sin_addr.s_addr;
			found = (addr & mask) == (addrcam & mask); // 与相机IP在同一网段
		}
		freeifaddrs(ifaddr);
	}

	return found ? ntohl(addr) : 0;
}

uint16_t CameraGY::msg_count() {
	if (++msgcnt_ == 0) msgcnt_ = 1;
	return msgcnt_;
}

void CameraGY::reg_write(uint32_t addr, uint32_t val) {
	mutex_lock lck(mtx_reg_);
	boost::array<uint8_t, 16> towrite = {0x42, 0x01, 0x00, 0x82, 0x00, 0x08};
	const uint8_t *rcvd;
	int n;

	((uint16_t*) &towrite)[3] = htons(msg_count());
	((uint32_t*) &towrite)[2] = htonl(addr);
	((uint32_t*) &towrite)[3] = htonl(val);
	udpcmd_->Write(towrite.c_array(), towrite.size());
	rcvd = (const uint8_t *) udpcmd_->BlockRead(n);

	if (n != 12 || rcvd[11] != 0x01) {
		char txt[200];
		int i;
		int n1 = sprintf(txt, "length<%d> of write register<%0X>: ", n, addr);
		for (i = 0; i < n; ++i) n1 += sprintf(txt + n1, "%02X ", rcvd[i]);
		throw runtime_error(txt);
	}
}

void CameraGY::reg_read(uint32_t addr, uint32_t &val) {
	mutex_lock lck(mtx_reg_);
	boost::array<uint8_t, 12> towrite = {0x42, 0x01, 0x00, 0x80, 0x00, 0x04};
	const uint8_t *rcvd;
	int n;

	((uint16_t*) &towrite)[3] = htons(msg_count());
	((uint32_t*) &towrite)[2] = htonl(addr);
	udpcmd_->Write(towrite.c_array(), towrite.size());
	rcvd = (const uint8_t *) udpcmd_->BlockRead(n);
	if (n == 12) val = ntohl(((uint32_t*)rcvd)[2]);
	else {
		char txt[200];
		int n1 = sprintf(txt, "length<%d> of read register<%0X>: ", n, addr);
		for (int i = 0; i < n; ++i) n1 += sprintf(txt + n1, "%02X ", rcvd[i]);
		throw runtime_error(txt);
	}
}

void CameraGY::receive_data(const long udp, const long len) {
	CAMERA_STATUS &state = nfptr_->state;
	if (bytercd_ == byteimg_ || state < CAMERA_EXPOSE) return;
	if (state == CAMERA_EXPOSE) state = CAMERA_IMGRDY;
	// 更新时间戳
	tmdata_ = microsec_clock::universal_time().time_of_day().total_milliseconds();

	int n;
	const uint8_t *pack = (const uint8_t *) udpdata_->Read(n);
	uint16_t idFrm = (pack[2] << 8) | pack[3];
	uint8_t  type  = pack[4];
	uint16_t idPck = (pack[5] << 16) | (pack[6] << 8) | pack[7];

	if (type == idLeader_) {// 引导帧
		idFrame_ = idFrm;
		idPack_  = idPck;
	}
	else if (type == idPayload_) {// 图像数据包
		if (!packflag_[idPck]) {// 避免重复接收
			uint32_t pcksize = len - headsize_; // UDP包长度已减去IP+UDP包头
			uint8_t *buf = nfptr_->data.get() + (idPck - 1) * packsize_;
			// 缓存图像数据
			if (idPck == packcnt_) pcksize -= 64;
			packflag_[idPck] = 1;
			memcpy(buf, pack + headsize_, pcksize);
			bytercd_ += pcksize;

			if (bytercd_ == byteimg_) cv_imgrdy_.notify_one();
			else {
				if (idFrame_ == 0xFFFF) idFrame_ = idFrm;
				idPack_ = idPck;
			}
		}
	}
	else if (type == idTrailer_) {// 收到尾帧时图像数据接收不完整
		idPack_ = idPck;
		re_transmit();
	}
}

void CameraGY::re_transmit(uint32_t iPack0, uint32_t iPack1) {
	mutex_lock lck(mtx_reg_);
	boost::array<uint8_t, 20> towrite = {0x42, 0x00, 0x00, 0x40, 0x00, 0x0c};
	((uint16_t*)&towrite)[3] = htons(msg_count());
	((uint32_t*)&towrite)[2] = htonl(idFrame_);
	((uint32_t*)&towrite)[3] = htonl(iPack0);
	((uint32_t*)&towrite)[4] = htonl(iPack1);
	udpcmd_->Write(towrite.c_array(), towrite.size());
}

bool CameraGY::update_network(const uint32_t addr, const char *vstr) {
	try {
		uint32_t value;
		static char buff[INET_ADDRSTRLEN];

		inet_pton(AF_INET, vstr, &value);
		reg_write(addr, ntohl(value));
		reg_read(addr, value);
		value = htonl(value);

		return true;
	}
	catch(std::runtime_error& ex) {
		nfptr_->errmsg = ex.what();
		return false;
	}
}

void CameraGY::re_transmit() {
	uint32_t i, ipck0, ipck1;

	for (i = 1; i <= packcnt_ && packflag_[i]; ++i);
	ipck0 = i;
	for (; i <= packcnt_ && !packflag_[i]; ++i);
	ipck1 = i - 1;
	re_transmit(ipck0, ipck1);
}

void CameraGY::thread_heartbeat() {
	int fail(0), limit(3);
	uint32_t val;
	reg_read(0x938, val);
	boost::chrono::milliseconds period(val / 2);// 延时周期两次心跳

	while(fail < limit) {
		boost::this_thread::sleep_for(period);

		// 心跳机制
		try {
			reg_read(0x0A00, val);
			if (fail) fail = 0;
		}
		catch(runtime_error& ex) {
			nfptr_->errmsg = ex.what();
			++fail;
		}
	}
	if (fail == limit && nfptr_->state == CAMERA_IDLE)
		exproc_(0.0, 0.0, (int) CAMERA_ERROR);
}

void CameraGY::thread_readout() {
	boost::chrono::milliseconds T1(100), T2(10);
	CAMERA_STATUS &state = nfptr_->state;
	ptime &tmobs = nfptr_->tmobs;
	float &expdur = nfptr_->expdur;
	float limit_exp(10.0);	// 曝光无数据延时
	int64_t limit_read(T2.count());	// 读出无数据延时
	ptime::time_duration_type td;
	int64_t no_data;
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while (state != CAMERA_ERROR) {
		cv_waitread_.wait(lck);

		while (state >= CAMERA_EXPOSE) {
			if (state == CAMERA_IMGRDY) {// 长时间未收到数据包, 申请重传
				boost::this_thread::sleep_for(T2);

				no_data = microsec_clock::universal_time().time_of_day().total_milliseconds() - tmdata_;
				if (no_data < 0) no_data += 86400000;
				if (no_data > limit_read) re_transmit();
			}
			else {// 长时间未收到数据包, 错误
				boost::this_thread::sleep_for(T1);

				td = microsec_clock::universal_time() - tmobs;
				if ((td.total_seconds() - expdur) > limit_exp) {
					nfptr_->errmsg = "long time no data respond";
					state = CAMERA_ERROR;
				}
			}
		}
	}
}
