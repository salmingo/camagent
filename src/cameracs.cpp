/*
 * @file cameracs.cpp 控制程序入口定义文件
 * @date 2017年4月9日
 * @version 0.3
 * @author Xiaomeng Lu
 * @note
 * 功能列表:
 * (1) 响应和反馈网络消息
 * (2) 控制相机工作流程
 * (3) 存储FITS文件
 * (4) 同步NTP时钟
 * (5) 上传FITS文件
 * @date 2017年5月19日
 * - CAMERA_WAIT_FLAT状态可接收object_info, 更新坐标
 * - 改变延迟时间不足时的变更策略
 */

#include <math.h>
#include <longnam.h>
#include <fitsio.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xpa.h>
#include "globaldef.h"
#include "cameracs.h"
#include "GLog.h"
//#include "CameraAndorCCD.h"
//#include "CameraFLI.h"
//#include "CameraApogee.h"
//#include "CameraPI.h"
#include "CameraGY.h"

using namespace boost;

cameracs::cameracs(boost::asio::io_service* ioserv) {
	io_main_  = ioserv;
	firstimg_ = true;
	resetdelay_ = 0;
	registered_ = false;
}

cameracs::~cameracs() {
}

bool cameracs::Start() {
	param_ = boost::make_shared<param_config>();
	nfsys_ = boost::make_shared<system_info>();
	nfobj_ = boost::make_shared<object_info>();
	ascproto_ = boost::make_shared<ascii_proto>();
	protonf_  = boost::make_shared<ascproto_camera_info>();
	bufrcv_.reset(new char[TCP_BUFF_SIZE]);

	param_->LoadFile(gConfigPath);
	// 启动消息队列
	string name = "msgque_";
	name += DAEMON_NAME;
	register_messages();
	if (!start(name.c_str())) {
		gLog.Write(NULL, LOG_FAULT, "Fail to create message queue <%s>", name.c_str());
		return false;
	}
	// 连接相机
	if (!connect_camera()) return false;
	nfsys_->state = CAMCTL_IDLE;
	start_ntp();	// 启动NTP服务
	start_fs();		// 启动后台文件传输服务
	connect_gc();	// 连接总控服务器
	// 启动周期线程
	thrdCycle_.reset(new boost::thread(boost::bind(&cameracs::ThreadCycle, this)));

	return true;
}

void cameracs::Stop() {
	ExitThread(thrdCycle_);
	ExitThread(thrdWait_);
	disconnect_gc();
	stop_ntp();
	disconnect_camera();
	stop_fs();
	stop();
}

void cameracs::register_messages() {
	const cbtype slot1 = boost::bind(&cameracs::OnConnectGC,     this, _1, _2);
	const cbtype slot2 = boost::bind(&cameracs::OnReceiveGC,     this, _1, _2);
	const cbtype slot3 = boost::bind(&cameracs::OnCloseGC,       this, _1, _2);
	const cbtype slot4 = boost::bind(&cameracs::OnPrepareExpose,  this, _1, _2);
	const cbtype slot5 = boost::bind(&cameracs::OnProcessExpose,  this, _1, _2);
	const cbtype slot6 = boost::bind(&cameracs::OnCompleteExpose, this, _1, _2);
	const cbtype slot7 = boost::bind(&cameracs::OnAbortExpose,    this, _1, _2);
	const cbtype slot8 = boost::bind(&cameracs::OnFailExpose,     this, _1, _2);
	const cbtype slot9 = boost::bind(&cameracs::OnCompleteWait,   this, _1, _2);
	const cbtype slot10= boost::bind(&cameracs::OnPeriodInfo,     this, _1, _2);

	register_message(MSG_CONNECT_GC,      slot1);
	register_message(MSG_RECEIVE_GC,      slot2);
	register_message(MSG_CLOSE_GC,        slot3);
	register_message(MSG_PREPARE_EXPOSE,  slot4);
	register_message(MSG_PROCESS_EXPOSE,  slot5);
	register_message(MSG_COMPLETE_EXPOSE, slot6);
	register_message(MSG_ABORT_EXPOSE,    slot7);
	register_message(MSG_FAIL_EXPOSE,     slot8);
	register_message(MSG_COMPLETE_WAIT,   slot9);
	register_message(MSG_PERIOD_INFO,     slot10);
}

void cameracs::ConnectedGC(const long client, const long ec) {
	post_message(MSG_CONNECT_GC, ec);
}

void cameracs::ReceivedGC(const long client, const long ec) {
	post_message(ec == 0 ? MSG_RECEIVE_GC: MSG_CLOSE_GC);
}

void cameracs::OnConnectGC(long ec, long) {
	if(!ec) {
		gLog.Write("SUCCEED: Connection with General Control Server");
		// 发送第一条消息, 声明设备在网络中的编号
		SendCameraInfo(-1);
	}
	else tcpgc_.reset();
}

void cameracs::OnReceiveGC(long, long) {
	if (!tcpgc_.unique() || !tcpgc_->is_open()) return;

	char term[] = "\n";	   // 换行符作为信息结束标记
	int len = strlen(term);// 结束符长度
	int pos;      // 标志符位置
	int toread;   // 信息长度
	string proto_type;
	apbase proto_body;

	while (tcpgc_->is_open() && (pos = tcpgc_->lookup(term, len)) >= 0) {
		/* 有效性判定 */
		if ((toread = pos + len) > TCP_BUFF_SIZE) {// 原因: 遗漏换行符作为协议结束标记; 高概率性丢包
			gLog.Write(NULL, LOG_FAULT, "protocol length is over than threshold");
			tcpgc_->close();
		}
		else {
			/* 读取协议内容 */
			tcpgc_->read(bufrcv_.get(), toread);
			bufrcv_[pos] = 0;
			/* 解析协议 */
			proto_body = ascproto_->resolve(bufrcv_.get(), proto_type);
			if (!proto_type.empty()) ProcessProtocol(proto_type, proto_body);
		}
	}
}

void cameracs::OnCloseGC(long, long) {
	gLog.Write("CLOSED: Connection with General Control Server");
	tmlastconnect_ = microsec_clock::universal_time();
	tcpgc_.reset();
	registered_ = false;
}

void cameracs::OnPrepareExpose(long, long) {
	if (param_->bNTP && ntp_.unique()) ntp_->SynchClock();

	boost::shared_ptr<devcam_info> nfcam = camera_->GetCameraInfo();
	ptime now = microsec_clock::universal_time();
	ptime tmobs(nfobj_->begin_time); // 曝光起始时间

	if (nfsys_->state == CAMCTL_IDLE) {// 新的曝光序列
		/* 建立曝光起始时间
		 * - 当约定帧间隔(代表需要等时间间隔获得图像数据), 但未定义起始时间时, 以当前时间为曝光起始时间
		 */
		if (nfobj_->delay > 0.001 && nfobj_->begin_time.is_special()) {
			nfobj_->begin_time = now;
			tmobs = nfobj_->begin_time;
		}
	}
	else if (nfobj_->delay > 0.001) {// 继续曝光
		/*
		 * 建立曝光起始时间
		 * - 下帧曝光起始时间=当前曝光起始时间+帧间隔(=曝光时间+帧间延迟)
		 * - 当约定帧间隔, 而实际过程显示帧间隔时间过短,或由于暂停曝光导致
		 *   起始时间不能满足等间隔时, 需调整曝光起始时间
		 */
		long n0 = long((nfobj_->expdur + nfobj_->delay) * 1E6);
		long n1 = (now - (tmobs = tmobs + microseconds(n0))).total_microseconds();
		long n(0);
		if (n1 <= 0 || n1 >= n0) resetdelay_ = 0;
		if (n1 > 0) {
			if (n1 >= n0) n = (n1 + n0 - 1) / n0 * n0;
			else {
				n = (n1 + 999999) / 1000000 * 1000000;// 临时调整帧间延迟
				if (++resetdelay_ == 2) {// 调整帧间延迟
					nfobj_->delay += n * 1E-6;
					resetdelay_ = 0;
				}
			}
		}
		if (n > 0) tmobs = tmobs + microseconds(n);
		nfobj_->begin_time = tmobs;
	}
	if (tmobs.is_special() || (tmobs - now).total_microseconds() <= 500) {
		/* 直接开始曝光
		 * 条件1: 未约定曝光起始时间
		 * 条件2: 约定曝光起始时间, 但未约定帧间隔
		 */
		StartExpose();
	}
	else {
		nfsys_->state = CAMCTL_WAIT_TIME;
		SendCameraInfo(nfsys_->state);
		thrdWait_.reset(new boost::thread(boost::bind(&cameracs::ThreadWait, this, tmobs)));
	}
}

void cameracs::OnProcessExpose(long left, long percent) {
	// 曝光进度不做处理
}

void cameracs::OnCompleteExpose(long, long) {
	ptime now = microsec_clock::universal_time();
	boost::shared_ptr<ascproto_camera_info> proto = boost::make_shared<ascproto_camera_info>();
	boost::shared_ptr<devcam_info> nfcam = camera_->GetCameraInfo();
	double mean, t;

	/* 处理曝光结果 */
	bool tosave(true), isflat(nfobj_->iimgtype == IMGTYPE_FLAT);
	if (isflat) {// 平场需要特殊处理
		mean = FlatMean();
		tosave = mean >= param_->tLow && mean <= param_->tHigh;
	}
	if (tosave) {// 保存FITS文件, 上传文件, 显示图像等操作
		// ... 待优化: 在内存中生成FITS文件, 当空闲(进入曝光过程后存储到硬盘)
		++nfobj_->frmno; // 有效存储累加计数
		if (SaveFITSFile()) {
			gLog.Write("FileName: %s", nfobj_->filename.c_str());
			nfsys_->update_freedisk(param_->pathLocal.c_str());
			UploadFile();   // 内存优化, 减少一次硬盘读出
			DisplayImage(); // 内存优化, 减少一次硬盘读出
		}
	}
	// 更新状态
	nfsys_->state = CAMCTL_COMPLETE;
	SendCameraInfo(nfsys_->state);

	/* 检测是否需要继续曝光 */
	bool bcont = nfsys_->command == EXPOSE_START// 指令要求继续曝光
			&& (nfobj_->frmcnt == -1 || nfobj_->frmno < nfobj_->frmcnt); // 曝光序列未结束
	if (bcont && isflat) {// 平场需要特殊处理
		if (mean < param_->tsaturate) mean += 65535;	// 特殊处理: 无饱和抑制功能相机
		t = param_->tExpect * nfobj_->expdur / mean;	// 依据当前统计结果调整曝光时间
		bcont = (nfcam->ampm && t >= param_->edMin) || (!nfcam->ampm && t <= param_->edMax);
		if (bcont) {// 调整曝光时间
			if (t < param_->edMin) t = param_->edMin;
			else if (t > param_->edMax) t = param_->edMax;
			nfobj_->expdur = t;
		}
	}
	if (bcont && nfobj_->Isvalid_end()) {// 约定了计划结束时间
		time_duration dt = nfobj_->end_time - now;
		bcont = dt.total_seconds() > 0;
	}
	if (bcont) {
		if (!isflat) post_message(MSG_PREPARE_EXPOSE);
		else {
			nfsys_->state = CAMCTL_WAIT_FLAT; // 触发转台重新指向
			SendCameraInfo(nfsys_->state);
		}
	}
	else if (nfsys_->command != EXPOSE_PAUSE) {
		gLog.Write("exposure sequence is over");
		nfsys_->state = CAMCTL_IDLE;
		SendCameraInfo(nfsys_->state);
	}
}

void cameracs::OnAbortExpose(long, long) {
	int state = nfsys_->state;
	if (nfsys_->command == EXPOSE_STOP) {
		gLog.Write("exposure sequence is aborted");
		nfsys_->state = CAMCTL_IDLE;
	}
	else if (nfsys_->command == EXPOSE_PAUSE) {
		gLog.Write("exposure sequence is suspend");
		nfsys_->state = CAMCTL_PAUSE;
	}
	else {// nfsys_->command == EXPOSE_START
		gLog.Write("OnAbortExpose", LOG_FAULT, "%s. try to start expose again",
				camera_->GetCameraInfo()->errmsg.c_str());
		nfsys_->state = CAMCTL_ABORT;
		post_message(MSG_PREPARE_EXPOSE);
	}
	// 通知服务器
	if (state != nfsys_->state) SendCameraInfo(nfsys_->state);
}

void cameracs::OnFailExpose(long, long) {
	gLog.Write("exposure failed", LOG_FAULT, "%s", camera_->GetCameraInfo()->errmsg.c_str());
	nfsys_->state = CAMCTL_ERROR;
}

void cameracs::OnCompleteWait(long, long) {
	thrdWait_.reset();

	if (nfsys_->state == CAMCTL_WAIT_TIME) StartExpose();
}

void cameracs::OnPeriodInfo(long, long) {
	SendCameraInfo(-1);
}

bool cameracs::connect_camera() {
	switch (param_->camType) {
	case 1:	// Andor CCD
	{
//		boost::shared_ptr<CameraAndorCCD> ccd = boost::make_shared<CameraAndorCCD>();
//		camera_ = boost::static_pointer_cast<CameraBase>(ccd);
	}
		break;
	case 2:	// FLI CCD
	{
//		boost::shared_ptr<CameraFLI> ccd = boost::make_shared<CameraFLI>();
//		camera_ = boost::static_pointer_cast<CameraBase>(ccd);
	}
		break;
	case 3:	// Apogee CCD
	{
//		boost::shared_ptr<CameraApogee> ccd = boost::make_shared<CameraApogee>();
//		camera_ = boost::static_pointer_cast<CameraBase>(ccd);
	}
		break;
	case 4:	// PI CCD
	{
//		boost::shared_ptr<CameraPI> ccd = boost::make_shared<CameraPI>();
//		camera_ = boost::static_pointer_cast<CameraBase>(ccd);
	}
		break;
	case 5:	// GWAC定制相机, 港宇公司
	{
		boost::shared_ptr<CameraGY> ccd = boost::make_shared<CameraGY>(param_->camIP);
		camera_ = boost::static_pointer_cast<CameraBase>(ccd);
	}
		break;
	default:
		gLog.Write("Unknown camera type: %d", param_->camType);
		return false;
	}
	boost::shared_ptr<devcam_info> nfcam = camera_->GetCameraInfo();
	if (!camera_->Connect()) {
		gLog.Write(NULL, LOG_FAULT, "Fail to connect camera: %s", nfcam->errmsg.c_str());
	}
	else {
//		camera_->SetReadPort(param_->readport);
//		camera_->SetReadRate(param_->readrate);
		camera_->SetGain(param_->gain);
		camera_->SetCooler(param_->coolerset, true);
		camera_->SetROI(param_->xbin, param_->ybin,
				param_->xstart, param_->ystart,
				param_->wroi, param_->hroi);

		// 注册回调函数
		const ExposeProcess::slot_type& slot = boost::bind(&cameracs::ExposeProcessCB, this, _1, _2, _3);
		camera_->register_expose(slot);
	}
	return nfcam->connected;
}

void cameracs::disconnect_camera() {
	if (camera_.unique()) {
		camera_->Disconnect();
		camera_.reset();
	}
}

void cameracs::start_ntp() {
	if (param_->bNTP) {
		ntp_ = boost::make_shared<NTPClient>(param_->hostNTP.c_str(), 123, param_->maxClockDiff);
		ntp_->EnableAutoSynch(false);
	}
	else gLog.Write("NTP is disabled. Local clock might be incorrect");
}

void cameracs::stop_ntp() {
	ntp_.reset();
}

void cameracs::connect_gc() {
	gLog.Write("Try to connect General Control Server<%s:%d>", param_->hostGC.c_str(), param_->portGC);
	tmlastconnect_ = microsec_clock::universal_time();
	// 创建对象并注册回调函数
	const tcp_client::cbtype& slot1 = boost::bind(&cameracs::ConnectedGC, this, _1, _2);
	const tcp_client::cbtype& slot2 = boost::bind(&cameracs::ReceivedGC, this, _1, _2);
	tcpgc_ = boost::make_shared<tcp_client>();
	tcpgc_->register_connect(slot1);
	tcpgc_->register_receive(slot2);
	tcpgc_->try_connect(param_->hostGC, param_->portGC);
}

void cameracs::disconnect_gc() {
	if (tcpgc_.unique()) {
		tcpgc_->close();
		tcpgc_.reset();
	}
}

void cameracs::start_fs() {
	if (param_->bFileSrv) {
		filecli_ = boost::make_shared<FileTransferClient>();
		filecli_->SetHost(param_->hostFileSrv, param_->portFileSrv);
		filecli_->SetDeviceID(param_->group_id, param_->unit_id, param_->camera_id);
		filecli_->Start();
	}
}

void cameracs::stop_fs() {
	if (filecli_.unique()) {
		filecli_->Stop();
		filecli_.reset();
	}
}

void cameracs::ExposeProcessCB(const double left, const double percent, const int state) {
	switch((CAMERA_STATUS) state) {
	case CAMERA_ERROR:  // 相机故障
		post_message(MSG_FAIL_EXPOSE);	// 错误: 一般发生在曝光过程中, 但也可能在空闲态
		break;
	case CAMERA_IDLE:   // 人工中止曝光; 自动中止曝光(GY相机长时间未收到数据包自动中止曝光)
		post_message(MSG_ABORT_EXPOSE);
		break;
	case CAMERA_EXPOSE: // 曝光过程中
//		post_message(MSG_PROCESS_EXPOSE, long(left * 1E6), long(percent * 100));
		break;
	case CAMERA_IMGRDY: // 已下载图像
		post_message(nfsys_->command == EXPOSE_START ? MSG_COMPLETE_EXPOSE : MSG_ABORT_EXPOSE);
		break;
	default:
		break;
	}
}

void cameracs::ThreadCycle() {
	boost::chrono::seconds period(1);	// 周期: 1秒
	boost::shared_ptr<devcam_info> nfcam = camera_->GetCameraInfo();
	double coolerget(100.0);
	ptime now;
	ptime::time_duration_type dt;
	int n;

	while(1) {
		boost::this_thread::sleep_for(period);
		now = microsec_clock::universal_time();
		dt = now - tmlastsend_;
		// 检查相机温度
		if (CAMERA_EXPOSE != nfcam->state) {
			if (fabs(coolerget - nfcam->coolerget) > 1.0) {
				coolerget = nfcam->coolerget;
				gLog.Write("CCD Temperature: %.1f", coolerget);
			}
			// 同步本机时钟
			if (param_->bNTP && ntp_.unique()) ntp_->SynchClock();
		}

		// 向总控服务器发送定时信息, 及检查网络连接有效性
		if (tcpgc_.unique() && tcpgc_->is_open()) {
			if (registered_ && dt.total_seconds() >= 1) post_message(MSG_PERIOD_INFO);
		}
		else {// 尝试重联服务器
			dt = now - tmlastconnect_;
			if (dt.total_seconds() >= 60) connect_gc();
		}
	}
}

void cameracs::ThreadWait(const ptime &tmobs) {
	/* 延时等待技术方案:
	 * - 综合对比: 线程, 定时器(deadline_timer), 条件变量(condition_variable)和消息队列(message_queue)
	 *   的资源开销与响应效率, 选择线程
	 */
	ptime now = microsec_clock::universal_time();
	ptime::time_duration_type dt = tmobs - now;
	int val = dt.total_microseconds() - 100;
	if (val > 0) boost::this_thread::sleep_for(boost::chrono::microseconds(val));
	// 线程正常结束
	post_message(MSG_COMPLETE_WAIT);
}

void cameracs::ExitThread(threadptr &thrd) {
	if (thrd.unique()) {
		thrd->interrupt();
		thrd->join();
		thrd.reset();
	}
}

void cameracs::SendCameraInfo(int state) {
	if (!(tcpgc_.unique() && tcpgc_->is_open() && camera_.unique())) return;

	int n;
	boost::shared_ptr<devcam_info> nfcam = camera_->GetCameraInfo();
	tmlastsend_ = microsec_clock::universal_time();

	protonf_->reset();
	protonf_->state	   = nfsys_->state;
	protonf_->utc      = to_iso_extended_string(tmlastsend_);
	if (nfsys_->state == CAMCTL_COMPLETE) {
		protonf_->filepath = nfobj_->filepath;
		protonf_->filename = nfobj_->filename;
		protonf_->filter   = nfobj_->filter;
		protonf_->objname  = nfobj_->obj_id;
		protonf_->expdur   = nfobj_->expdur;
		protonf_->frmtot   = nfobj_->frmcnt;
		protonf_->frmnum   = nfobj_->frmno;
	}
	else if (state == -1) {
		protonf_->group_id = param_->group_id;
		protonf_->unit_id  = param_->unit_id;
		protonf_->camera_id= param_->camera_id;
		protonf_->readport = nfcam->readport;
		protonf_->readrate = nfcam->readrate;
		protonf_->gain     = nfcam->gain;
		protonf_->coolget  = nfcam->coolerget;
		protonf_->freedisk = nfsys_->freedisk;
	}

	const char* compacted = ascproto_->compact_camera_info(protonf_.get(), n);
	tcpgc_->write(compacted, n);
}

void cameracs::StartExpose() {
	if (camera_->Expose(nfobj_->expdur, nfobj_->light)) {
		gLog.Write("New exposure: No.<%d of %d>", nfobj_->frmno + 1, nfobj_->frmcnt);
		nfsys_->state = CAMCTL_EXPOSE;
	}
	else {
		gLog.Write("StartExpose", LOG_FAULT, "failed to start exposure: %s",
				camera_->GetCameraInfo()->errmsg.c_str());
		nfsys_->state = CAMCTL_ERROR;
	}
	SendCameraInfo(nfsys_->state);
}

bool cameracs::SaveFITSFile() {
	fitsfile *fitsptr;
	int status(0);
	int naxis(2);
	boost::shared_ptr<devcam_info> nfcam = camera_->GetCameraInfo();
	long naxes[] = {nfcam->roi.get_width(), nfcam->roi.get_height()};
	long pixels = nfcam->roi.get_width() * nfcam->roi.get_height();
	char buff[300];
	int n;
	string tmstr;

	// 创建目录结构
	if (nfobj_->frmno == 1) {
		sprintf(buff, "G%s_%s_%s", param_->unit_id.c_str(),
				param_->camera_id.c_str(), nfcam->utcdate.c_str());
		nfobj_->subpath = buff;
		sprintf(buff, "%s/%s", param_->pathLocal.c_str(), nfobj_->subpath.c_str());
		nfobj_->pathname = buff;
		if (access(nfobj_->pathname.c_str(), F_OK)) mkdir(nfobj_->pathname.c_str(), 0755);
	}
	// 生成文件名
	n = sprintf(buff, "G%s", param_->camera_id.c_str());
	if (!nfobj_->obstype.empty()) n += sprintf(buff + n, "_%s", nfobj_->obstype.c_str());
	n += sprintf(buff +n, "_%s_%s.fit",
			nfobj_->imgtypeab.c_str(),
			nfcam->utctime.c_str());
	nfobj_->filename = buff;
	sprintf(buff, "%s/%s", nfobj_->pathname.c_str(), nfobj_->filename.c_str());
	nfobj_->filepath = buff;
	// 存储FITS文件并写入完整头信息
	fits_create_file(&fitsptr, nfobj_->filepath.c_str(), &status);
	fits_create_img(fitsptr, USHORT_IMG, naxis, naxes, &status);
	fits_write_img(fitsptr, TUSHORT, 1, pixels, nfcam->data.get(), &status);
	/* FITS头 */
	fits_write_key(fitsptr, TSTRING, "GROUP_ID", (void*)param_->group_id.c_str(), "group id", &status);
	fits_write_key(fitsptr, TSTRING, "UNIT_ID", (void*)param_->unit_id.c_str(), "unit id", &status);
	fits_write_key(fitsptr, TSTRING, "CAM_ID", (void*)param_->camera_id.c_str(), "camera id", &status);
	fits_write_key(fitsptr, TSTRING, "MOUNT_ID", (void*)param_->unit_id.c_str(), "mount id", &status);
	fits_write_key(fitsptr, TSTRING, "CCDTYPE", (void*)nfobj_->imgtype.c_str(), "type of image", &status);
	fits_write_key(fitsptr, TSTRING, "DATE-OBS", (void*)nfcam->dateobs.c_str(), "UTC date of begin observation", &status);
	fits_write_key(fitsptr, TSTRING, "TIME-OBS", (void*)nfcam->timeobs.c_str(), "UTC time of begin observation", &status);
	fits_write_key(fitsptr, TSTRING, "TIME-END", (void*)nfcam->timeend.c_str(), "UTC time of end observation", &status);
	fits_write_key(fitsptr, TDOUBLE, "JD", &nfcam->jd, "Julian day of begin observation", &status);
	fits_write_key(fitsptr, TDOUBLE, "EXPTIME", &nfcam->eduration, "exposure duration", &status);
	fits_write_key(fitsptr, TUINT,   "GAIN",    &nfcam->gain, "", &status);
	fits_write_key(fitsptr, TDOUBLE, "TEMPSET", &nfcam->coolerset, "cooler set point", &status);
	fits_write_key(fitsptr, TDOUBLE, "TEMPACT", &nfcam->coolerget, "cooler actual point", &status);
	fits_write_key(fitsptr, TSTRING, "TERMTYPE", (void*)param_->termType.c_str(), "terminal type", &status);

	if (nfobj_->op_sn >= 0)        fits_write_key(fitsptr, TINT,    "OP_SN",    &nfobj_->op_sn, "serial number of plan", &status);
	if (!nfobj_->op_time.empty())  fits_write_key(fitsptr, TSTRING, "OP_TIME",  (void*)nfobj_->op_time.c_str(), "time of plan generated", &status);
	if (!nfobj_->op_type.empty())  fits_write_key(fitsptr, TSTRING, "OP_TYPE",  (void*)nfobj_->op_type.c_str(), "plan type", &status);
	if (!nfobj_->obstype.empty())  fits_write_key(fitsptr, TSTRING, "OBSTYPE",  (void*)nfobj_->obstype.c_str(), "observation type", &status);
	if (!nfobj_->grid_id.empty())  fits_write_key(fitsptr, TSTRING, "GRID_ID",  (void*)nfobj_->grid_id.c_str(), "grid id", &status);
	if (!nfobj_->field_id.empty()) fits_write_key(fitsptr, TSTRING, "FIELD_ID", (void*)nfobj_->field_id.c_str(), "sky field id", &status);
	if (!nfobj_->obj_id.empty())   fits_write_key(fitsptr, TSTRING, "OBJECT",   (void*)nfobj_->obj_id.c_str(), "name of object", &status);
	if (valid_ra(nfobj_->ra) && valid_dc(nfobj_->dec)) {
		fits_write_key(fitsptr, TDOUBLE, "RA",       &nfobj_->ra,  "center RA of field", &status);
		fits_write_key(fitsptr, TDOUBLE, "DEC",      &nfobj_->dec, "center DEC of field", &status);
		fits_write_key(fitsptr, TDOUBLE, "EPOCH",    &nfobj_->epoch, "epoch of center position", &status);
	}
	if (valid_ra(nfobj_->objra) && valid_dc(nfobj_->objdec)) {
		fits_write_key(fitsptr, TDOUBLE, "OBJRA",    &nfobj_->objra,  "object RA", &status);
		fits_write_key(fitsptr, TDOUBLE, "OBJDEC",   &nfobj_->objdec, "object DEC", &status);
		fits_write_key(fitsptr, TDOUBLE, "OBJEPOCH", &nfobj_->objepoch, "epoch of object position", &status);
	}
	if (!nfobj_->objerror.empty()) fits_write_key(fitsptr, TSTRING, "OBJERR",   (void*)nfobj_->objerror.c_str(), "error bar of position", &status);
	if (nfobj_->priority > 0)      fits_write_key(fitsptr, TINT,    "PRIORITY", &nfobj_->priority, "plan priority", &status);
	if (nfobj_->focus > INT_MIN)   fits_write_key(fitsptr, TINT,    "TELFOCUS", &nfobj_->focus,    "telescope focus value in micron", &status);

	if (nfobj_->Isvalid_begin()) {
		tmstr = to_iso_extended_string(nfobj_->begin_time);
		fits_write_key(fitsptr, TSTRING, "BEGIN_T", (void*)tmstr.c_str(), "expected begin time", &status);
	}
	if (nfobj_->Isvalid_end()) {
		tmstr = to_iso_extended_string(nfobj_->end_time);
		fits_write_key(fitsptr, TSTRING, "END_T", (void*)tmstr.c_str(), "expected end time", &status);
	}
	if (nfobj_->pair_id > 0) fits_write_key(fitsptr, TINT, "PAIR_ID", &nfobj_->pair_id, "pair id", &status);
	fits_write_key(fitsptr, TINT, "FRAMENO", &nfobj_->frmno, "frame no in this run", &status);
	fits_close_file(fitsptr, &status);

	if (status) {
		char txt[200];
		fits_get_errstatus(status, txt);
		gLog.Write(NULL, LOG_FAULT, "Fail to save FITS file<%s>: %s", nfobj_->filepath.c_str(), txt);
	}
	return status == 0;
}

void cameracs::UploadFile() {
	if (filecli_.unique()) {// 启动文件传输
		FileTransferClient::upload_file file;
		file.grid_id = nfobj_->grid_id;
		file.field_id= nfobj_->field_id;
		file.timeobs = to_iso_extended_string(camera_->GetCameraInfo()->tmobs);
		file.filename = nfobj_->filename;
		file.filepath = nfobj_->filepath;
		file.subpath  = nfobj_->subpath;
		filecli_->NewFile(&file);
	}
}

void cameracs::ProcessProtocol(string& proto_type, apbase& proto_body) {
	if (iequals(proto_type, "focus")) {// 焦点位置发生变化
		boost::shared_ptr<ascproto_focus> proto = boost::static_pointer_cast<ascproto_focus>(proto_body);
		nfobj_->focus = int(proto->value);
	}
	else if (iequals(proto_type, "object_info")) {// 观测目标信息
		boost::shared_ptr<ascproto_object_info> proto = boost::static_pointer_cast<ascproto_object_info>(proto_body);
		if (nfsys_->state == CAMCTL_IDLE) {
			gLog.Write("New sequence: <name=%s, type=%s, expdur=%.3f, frmcnt=%d>",
					proto->obj_id.c_str(), proto->imgtype.c_str(), proto->expdur, proto->frmcnt);
			*nfobj_ = *proto;
		}
		else if (nfsys_->state == CAMCTL_WAIT_FLAT) {
			nfobj_->ra = proto->ra;
			nfobj_->dec= proto->dec;
		}
		else {
			gLog.Write(NULL, LOG_WARN, "<object_info> is rejected for system is busy");
		}
	}
	else if (iequals(proto_type, "expose")) {// 曝光指令
		boost::shared_ptr<ascproto_expose> proto = boost::static_pointer_cast<ascproto_expose>(proto_body);
		int state = nfsys_->state;
		switch(proto->command) {
		case EXPOSE_START:
		case EXPOSE_RESUME:
		{
			if (nfobj_->obj_id.empty()) {
				gLog.Write(NULL, LOG_WARN, "<expose start> is rejected for obj_id is empty");
			}
			else if (state == CAMCTL_ERROR) {
				gLog.Write(NULL, LOG_FAULT, "<expose start> is rejected for system is error");
			}
			else if (state == CAMCTL_IDLE || state == CAMCTL_PAUSE || state == CAMCTL_WAIT_FLAT) {
				gLog.Write("<expose start> %s exposure sequence", state == CAMCTL_IDLE ? "start" : "resume");
				nfsys_->command = EXPOSE_START;
				resetdelay_ = 0;
				post_message(MSG_PREPARE_EXPOSE);
			}
			else if (nfsys_->command == EXPOSE_PAUSE) {// PAUSE指令未执行完又收到曝光指令
				gLog.Write("<expose start> try to resume exposure sequence");
				nfsys_->command = EXPOSE_START;
			}
			else {
				gLog.Write(NULL, LOG_WARN, "<expose start> is rejected for system is busy");
			}
		}
			break;
		case EXPOSE_STOP:
		{
			if (state > CAMCTL_IDLE) {
				gLog.Write("<expose stop> abort exposure sequence");
				nfsys_->command = EXPOSE_STOP;
				if (state >= CAMCTL_ABORT) {// 暂停态: 直接结束
					gLog.Write("exposure sequence is aborted");
					nfsys_->state = CAMCTL_IDLE;
					ExitThread(thrdWait_);
					SendCameraInfo(nfsys_->state);
				}
				else if (state == CAMCTL_EXPOSE) {// 通过曝光逻辑调整系统状态
					camera_->AbortExpose();
				}
			}
		}
			break;
		case EXPOSE_PAUSE:
		{
			if (state == CAMERA_EXPOSE || state == CAMCTL_COMPLETE || state == CAMCTL_ABORT || state == CAMCTL_WAIT_TIME) {
				gLog.Write("<expose pause> suspend exposure sequence");
				nfsys_->command = EXPOSE_PAUSE;
				if (state == CAMERA_EXPOSE) camera_->AbortExpose();
				else if (state == CAMCTL_WAIT_TIME) {// 延时等待切换为等待
					gLog.Write("exposure sequence is suspended");
					nfsys_->state = CAMCTL_PAUSE;
					ExitThread(thrdWait_);
					SendCameraInfo(nfsys_->state);
				}
			}
		}
			break;
		default:
			break;
		}
	}
	else if (iequals(proto_type, "camera_info")) {// 注册相机后收到的第一条信息
		boost::shared_ptr<ascproto_camera_info> proto = boost::static_pointer_cast<ascproto_camera_info>(proto_body);
		gLog.Write("Camera<%s> registered in Observation System<%s:%s>",
				proto->camera_id.c_str(), proto->group_id.c_str(), proto->unit_id.c_str());
		registered_ = true;
	}
}

double cameracs::FlatMean() {
	boost::shared_ptr<devcam_info> nfcam = camera_->GetCameraInfo();
	int pixels = nfcam->roi.get_width() * nfcam->roi.get_height();
	int n = pixels > 10000 ? 10000 : pixels;
	boost::shared_array<uint8_t> data = nfcam->data;
	double step = (double) pixels / n;
	double sumv(0.0);
	double pos;

	for (pos = 20, n = 0; pos < pixels; pos += step, ++n) {
		sumv += data[int(pos)];
	}

	return sumv / n;
}

void cameracs::DisplayImage() {// display fits image
	if (!param_->bShowImg) return;

	if (XPASetFile(nfobj_->filepath.c_str(), nfobj_->filename.c_str(), firstimg_) && firstimg_) {
		firstimg_ = false;
		XPASetScaleMode("zscale");
		XPASetZoom("to fit");
		XPAPreservePan();
		XPAPreserveRegion();
	}
}

bool cameracs::XPASetFile(const char *filepath, const char *filename, bool back) {
	char *names, *messages;
	char params[100];
	char mode[50];

	strcpy(mode, back ? "ack=true" : "ack=false");
	sprintf(params, "fits %s", filename);
	int fd = open(filepath, O_RDONLY);
	if (fd >= 0) {
		XPASetFd(NULL, (char*) "ds9", params, mode, fd, &names, &messages, 1);
		if (names) free(names);
		if (messages) free(messages);
		close(fd);
		return true;
	}
	return false;
}

void cameracs::XPASetScaleMode(const char *mode) {
	char *names, *messages;
	char params[40];

	sprintf(params, "scale mode %s", mode);
	XPASet(NULL, (char*) "ds9", params, NULL, NULL, 0, &names, &messages, 1);
	if (names) free(names);
	if (messages) free(messages);
}

void cameracs::XPASetZoom(const char *zoom) {
	char *names, *messages;
	char params[40];

	sprintf(params, "zoom %s", zoom);
	XPASet(NULL, (char*) "ds9", params, NULL, NULL, 0, &names, &messages, 1);
	if (names) free(names);
	if (messages) free(messages);
}

void cameracs::XPAPreservePan(bool yes) {
	char *names, *messages;
	char params[40];

	sprintf(params, "preserve pan %s", yes ? "yes" : "no");
	XPASet(NULL, (char*) "ds9", params, NULL, NULL, 0, &names, &messages, 1);
	if (names) free(names);
	if (messages) free(messages);
}

void cameracs::XPAPreserveRegion(bool yes) {
	char *names, *messages;
	char params[40];

	sprintf(params, "preserve regions %s", yes ? "yes" : "no");
	XPASet(NULL, (char*) "ds9", params, NULL, NULL, 0, &names, &messages, 1);
	if (names) free(names);
	if (messages) free(messages);
}
