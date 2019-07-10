/*
 * @file cameracs.cpp 相机控制软件声明文件, 实现天文相机工作流程
 */
#include <boost/filesystem.hpp>
#include <algorithm>
#include "cameracs.h"
#include "globaldef.h"
#include "GLog.h"
#include "CameraAndorCCD.h"
#include "CameraFLICCD.h"
#include "CameraApogee.h"
#include "CameraGY.h"

using namespace boost::filesystem;

cameracs::cameracs(boost::asio::io_service* ios) {
	bufrcv_.reset(new char[TCP_PACK_SIZE]);
}

cameracs::~cameracs() {
}

bool cameracs::StartService() {
	/* 启动消息队里 */
	string mqname = "msgque_";
	mqname += DAEMON_NAME;
	if (!Start(mqname.c_str())) return false;
	register_message();
	/* 尝试建立与各设备的连接 */
	param_ = boost::make_shared<ConfigParameter>();
	if (!param_->Load(gConfigPath)) {
		_gLog.Write(LOG_FAULT, NULL, "failed to load configured parameters");
		return false;
	}
	if (!connect_server_gtoaes()) return false;
	if (!connect_camera()) return false;
	if (!connect_filter()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect filter");
		return false;
	}
	if (param_->ntpenable) {// 启用NTP时钟同步
		ntp_ = make_ntp(param_->ntpip.c_str(), 123, false);
		ntp_->SetSyncLimit(param_->ntpdiff);
	}
	if (param_->imgshow) {
		ds9_ = boost::make_shared<CDs9>();
	}
	/* 平场控制参数 */
	flatsky_.tmin = param_->ffmint;
	flatsky_.tmax = param_->ffmaxt;
	flatsky_.fmin = param_->ffminv;
	flatsky_.fmax = param_->ffmaxv;
	/* 启动多线程, 在后台监测工作状态及更新工作逻辑 */
	thrd_noon_.reset(new boost::thread(boost::bind(&cameracs::thread_noon, this)));

	return false;
}

void cameracs::StopService() {
	/* 终止消息队列, 不响应各设备状态变更产生的事件 */
	Stop();

	/* 终止多线程 */
	int_thread(thrd_noon_);
	int_thread(thrd_state_);
	int_thread(thrd_cool_);
	int_thread(thrd_reconn_gtoaes_);

	/* 显式调用reset(), 以触发析构函数 */
	gtoaes_.reset();
	ds9_.reset();
	ntp_.reset();
	filter_.reset();
	camera_.reset();
}

/////////////////////////////////////////////////////////////////////////////
/* 建立并维护与硬件设备的连接 */
bool cameracs::connect_server_gtoaes() {
	const TCPClient::CBSlot &slot = boost::bind(&cameracs::receive_gtoaes, this, _1, _2);
	bool rslt;
	gtoaes_ = maketcp_client();
	gtoaes_->RegisterRead(slot);
	rslt = gtoaes_->Connect(param_->gcip, param_->gcport);
	if (!rslt) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect general-control server");
	}
	else {
		on_connect_gc();
	}
	return rslt;
}

bool cameracs::connect_server_file() {
	return false;
}

bool cameracs::connect_camera() {
	switch(param_->camType) {
	case 1: // Andor CCD
	{
		boost::shared_ptr<CameraAndorCCD> camera = boost::make_shared<CameraAndorCCD>();
		camera_ = to_cambase(camera);
	}
		break;
	case 2: // FLI CCD
	{
		boost::shared_ptr<CameraFLICCD> camera = boost::make_shared<CameraFLICCD>();
		camera_ = to_cambase(camera);
	}
		break;
	case 3: // Apogee CCD
	{
		boost::shared_ptr<CameraApogee> camera = boost::make_shared<CameraApogee>();
		camera_ = to_cambase(camera);
	}
		break;
	case 4: // GY CCD
	{
		boost::shared_ptr<CameraGY> camera = boost::make_shared<CameraGY>(param_->camIP);
		camera_ = to_cambase(camera);
	}
		break;
	default:
		_gLog.Write(LOG_FAULT, NULL, "undefined camera type");
		return false;
	}
	if (!camera_->Connect()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect camera");
		return false;
	}
	else {
		camera_->UpdateADChannel(param_->ADChannel);
		camera_->UpdateReadPort(param_->readport);
		camera_->UpdateReadRate(param_->readrate);
		camera_->UpdateGain(param_->gain);
		camera_->UpdateVSRate(param_->vsrate);
		camera_->UpdateEMGain(param_->emgain);
		if (!param_->coolalone) {// 通用相机, 相机与温控集成在一起
			camera_->UpdateCooler(true, param_->coolset);
		}
		else {// GY相机, 温控与相机控制分离. 需启动网络服务, 接收温控信息
			const UDPSession::CBSlot &slot = boost::bind(&cameracs::receive_alone_cooler, this, _1, _2);
			cool_alone_ = makeudp_session();
			cool_alone_->RegisterRead(slot);
			cool_alone_->Connect(param_->coolip.c_str(), param_->coolport);
			thrd_cool_.reset(new boost::thread(boost::bind(&cameracs::thread_cooler, this)));
		}

		CameraBase::NFCamPtr nfcam = camera_->GetCameraInfo();
		_gLog.Write("Camera Settings:\n"
				"\t BitPixel     = %d\n"
				"\t ReadPort     = %s\n"
				"\t ReadRate     = %s\n"
				"\t PreAmpGain   = %.1f\n"
				"\t VerticalRate = %.1f us/line",
				nfcam->bitpixel,
				nfcam->readport.c_str(),
				nfcam->readrate.c_str(),
				nfcam->gain,
				nfcam->vsrate);

		return true;
	}
}

bool cameracs::connect_filter() {
	return false;
}

void cameracs::disconnect_camera() {

}

void cameracs::disconnect_filter() {

}
/////////////////////////////////////////////////////////////////////////////
void cameracs::receive_gtoaes(const long addr, const long ec) {
	PostMessage(ec ? MSG_CLOSE_GC : MSG_RECEIVE_GC);
}

void cameracs::connect_gtoaes(const long addr, const long ec) {
	if (!ec) PostMessage(MSG_CONNECT_GC);
}

void cameracs::receive_alone_cooler(const long, const long) {

}

/////////////////////////////////////////////////////////////////////////////
/* 消息机制 */
void cameracs::register_message() {
	const CBSlot& slot01 = boost::bind(&cameracs::on_receive_gc,   this, _1, _2);
	const CBSlot& slot02 = boost::bind(&cameracs::on_close_gc,     this, _1, _2);
	const CBSlot& slot03 = boost::bind(&cameracs::on_connect_gc,   this, _1, _2);

	RegisterMessage(MSG_RECEIVE_GC,      slot01);
	RegisterMessage(MSG_CLOSE_GC,        slot02);
	RegisterMessage(MSG_CONNECT_GC,      slot03);
}

void cameracs::on_receive_gc(const long, const long) {
	char term[] = "\n";	   // 换行符作为信息结束标记
	int len = strlen(term);// 结束符长度
	int pos;      // 标志符位置
	int toread;   // 信息长度

	while ((pos = gtoaes_->Lookup(term, len)) >= 0) {
		// 读取协议内容并解析执行
		gtoaes_->Read(bufrcv_.get(), toread);
		bufrcv_[pos] = 0;

//		proto = ascproto_->Resolve(bufrcv_.get());
//		// 检查: 协议有效性及设备标志基本有效性
//		if (!proto.use_count()
//				|| (!proto->uid.empty() && proto->gid.empty())
//				|| (!proto->cid.empty() && (proto->gid.empty() || proto->uid.empty()))) {
//			_gLog.Write(LOG_FAULT, "cameracs::on_receive_gc",
//					"illegal protocol. received: %s", bufrcv_.get());
//			gtoaes_->Close();
//		}
	}
}

void cameracs::on_close_gc(const long, const long ec) {
	_gLog.Write(LOG_WARN, NULL, "connection with general-control server was broken. re-connect automatically later");
	int_thread(thrd_state_);
	thrd_reconn_gtoaes_.reset(new boost::thread(boost::bind(&cameracs::thread_reconn_gtoaes, this)));
}

void cameracs::on_connect_gc(const long, const long) {
	_gLog.Write("SUCCESS: connected to general-control server");
	int_thread(thrd_reconn_gtoaes_);
	gtoaes_->UseBuffer();
	//... 在服务器上注册相机
	thrd_state_.reset(new boost::thread(boost::bind(&cameracs::thread_state, this)));
}

/////////////////////////////////////////////////////////////////////////////
/* 多线程机制 */
void cameracs::thread_state() {
	boost::mutex mtx;
	mutex_lock lck(mtx);
	boost::chrono::seconds period(10);

	while (1) {
		/*
		 * - 状态无变化时, 10秒周期
		 * - 状态变化时, 立即发送
		 */
		cv_camstate_changed_.wait_for(lck, period);
		//... 发送工作状态
	}
}

void cameracs::thread_noon() {
	while(1) {
		free_local_storage(); // 清理本次磁盘空间

		/* 计算延时等待时间 */
		ptime now(second_clock::local_time());
		ptime noon(now.date(), hours(12));
		int secs = (noon - now).total_seconds();
		if (secs < 10) secs += 86400;
		boost::this_thread::sleep_for(boost::chrono::seconds(secs));
	}
}

void cameracs::thread_reconn_gtoaes() {
	boost::chrono::minutes period(2);

	while(1) {
		boost::this_thread::sleep_for(period);

		const TCPClient::CBSlot &slot1 = boost::bind(&cameracs::connect_gtoaes, this, _1, _2);
		const TCPClient::CBSlot &slot2 = boost::bind(&cameracs::receive_gtoaes, this, _1, _2);
		gtoaes_ = maketcp_client();
		gtoaes_->RegisterConnect(slot1);
		gtoaes_->RegisterRead(slot2);
		gtoaes_->AsyncConnect(param_->gcip, param_->gcport);
	}
}

void cameracs::thread_cooler() {
	boost::chrono::seconds period(30); // 周期30秒

	while (1) {
		// ... 查询温度

		boost::this_thread::sleep_for(period);
	}
}

/////////////////////////////////////////////////////////////////////////////
void cameracs::free_local_storage() {
	typedef vector<string> strvec;
	strvec subpath; // 根目录下的子目录名称集合
	path root = param_->pathroot;
	space_info nfspace = space(root);
	if ((nfspace.available >> 30) < param_->fdmin) {
		_gLog.Write(LOG_WARN, NULL, "preparing to clean local storage for image files");
		// 遍历子目录
		for (directory_iterator x = directory_iterator(root); x != directory_iterator(); ++x) {
			subpath.push_back(x->path().filename().string());
		}
		sort(subpath.begin(), subpath.end()); // 按子目录名递增排序
		// 删除子目录
		for (strvec::iterator it = subpath.begin(); it != subpath.end(); ++it) {
			path filepath(param_->pathroot);
			filepath /= (*it);
			remove_all(filepath);

			nfspace = space(root);
			if ((nfspace.available >> 30) > param_->fdmin) break;
		}
		_gLog.Write("free storage capacity is currently %d GB", nfspace.available >> 30);
	}
}
