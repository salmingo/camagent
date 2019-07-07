/*
 * @file cameracs.cpp 相机控制软件声明文件, 实现天文相机工作流程
 */

#include "cameracs.h"
#include "globaldef.h"
#include "GLog.h"

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
	if (!connect_camera()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect camera");
		return false;
	}
	if (!connect_filter()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect filter");
		return false;
	}
	/* 启动多线程, 在后台监测工作状态及更新工作逻辑 */

	return false;
}

void cameracs::StopService() {
	/* 终止多线程 */
	int_thread(thrd_state_);
	int_thread(thrd_reconn_gtoaes_);

	/* 终止消息队列, 不响应各设备状态变更产生的事件 */
	Stop();

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
		_gLog.Write("SUCCESS: connected to general-control server");
		gtoaes_->UseBuffer();
		//... 在服务器上注册相机
	}
	return rslt;
}

bool cameracs::connect_server_file() {
	return false;
}

bool cameracs::connect_camera() {
	return false;
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

	while (gtoaes_->IsOpen() && (pos = gtoaes_->Lookup(term, len)) >= 0) {
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
	thrd_reconn_gtoaes_.reset(new boost::thread(boost::bind(&cameracs::thread_reconn_gtoaes, this)));
}

void cameracs::on_connect_gc(const long, const long) {
	_gLog.Write("SUCCESS: connected to general-control server");
	gtoaes_->UseBuffer();
	int_thread(thrd_reconn_gtoaes_);
	//... 在服务器上注册相机
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
