/*
 * @file cameracs.cpp 相机控制软件声明文件, 实现天文相机工作流程
 */
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <longnam.h>
#include <fitsio.h>
#include "globaldef.h"
#include "GLog.h"
#include "cameracs.h"
#include "CameraApogee.h"
#include "CameraAndorCCD.h"
#include "CameraGY.h"
#include "FilterCtrlFLI.h"

using namespace std;
using namespace boost;
using namespace boost::posix_time;

cameracs::cameracs(boost::asio::io_service* ios) {
	ostype_ = OBSST_UNKNOWN;
	ios_    = ios;
	cmdexp_ = EXPOSE_INIT;
	cds9_   = boost::make_shared<CDs9>();
}

cameracs::~cameracs() {
}

bool cameracs::StartService() {
	param_  = boost::make_shared<Parameter>();
	param_->Load(gConfigPath);
	nfcam_ = boost::make_shared<ascii_proto_camera>();
	nfcam_->gid   = param_->gid;
	nfcam_->uid   = param_->uid;
	nfcam_->cid   = param_->cid;
	ascproto_ = make_ascproto();
	bufrcv_.reset(new char[TCP_PACK_SIZE]);
	/* 注册消息并尝试启动消息队列 */
	string name = "msgque_";
	name += DAEMON_NAME;
	register_messages();
	if (!Start(name.c_str())) return false; // 启动消息队列
	if (!connect_camera())    return false; // 连接相机
	if (!connect_filter())    return false; // 连接滤光片
	if (param_->bFile) {// 启动文件上传服务
		ftcli_.reset(new FileTransferClient(param_->hostFile, param_->portFile));
		ftcli_->SetDeviceID(param_->gid, param_->uid, param_->cid);
		ftcli_->Start();
	}
	if (param_->bNTP) {// 连接NTP服务器
		ntp_ = make_ntp(param_->hostNTP.c_str(), 123, param_->maxClockDiff);
		ntp_->EnableAutoSynch(false);
		thrd_clocksync_.reset(new boost::thread(boost::bind(&cameracs::thread_clocksync, this)));
	}
	connect_gtoaes(); // 连接服务器
	thrd_noon_.reset(new boost::thread(boost::bind(&cameracs::thread_noon, this)));

	return true;
}

void cameracs::StopService() {
	Stop();
	// 中断线程
	interrupt_thread(thrd_noon_);
	interrupt_thread(thrd_clocksync_);
	interrupt_thread(thrd_upload_);
	interrupt_thread(thrd_filter_);
	interrupt_thread(thrd_camera_);
	interrupt_thread(thrd_gtoaes_);

	if (ftcli_.unique()) ftcli_->Stop();
	// 断开与设备的连接
	if (filterctl_.unique()) filterctl_->Disconnect();
	if (camctl_.unique()) camctl_->Disconnect();
}

void cameracs::MonitorVersion() {
	_gLog.Write("version update will terminate program");
	ios_->stop();
}

//////////////////////////////////////////////////////////////////////////////
bool cameracs::connect_camera() {
	switch(param_->camType) {
	case 1: // Andor CCD
	{
		boost::shared_ptr<CameraAndorCCD> ccd = boost::make_shared<CameraAndorCCD>();
		camctl_ = to_cambase(ccd);
	}
		break;
	case 2:	// FLI CCD
	{

	}
		break;
	case 3:	// ApogeeU CCD
	{
		boost::shared_ptr<CameraApogee> ccd = boost::make_shared<CameraApogee>();
		camctl_ = to_cambase(ccd);
	}
		break;
	case 4:	// PI CCD
	{

	}
		break;
	case 5:	// GWAC-GY CCD
	{
		boost::shared_ptr<CameraGY> ccd = boost::make_shared<CameraGY>(param_->camIP);
		camctl_ = to_cambase(ccd);
	}
		break;
	default:
		_gLog.Write(LOG_FAULT, NULL, "unknown camera type <%d>", param_->camType);
		return false;
	}

	const NFDevCamPtr nfdev = camctl_->GetCameraInfo();
	if (!camctl_->Connect()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect camera: %s",
				nfdev->errmsg.c_str());
	}
	else {
		// 设置相机工作参数
		camctl_->SetCooler    (param_->coolerset, true);
		camctl_->SetADChannel (param_->ADChannel);
		camctl_->SetReadPort  (param_->readport);
		camctl_->SetReadRate  (param_->readrate);
		camctl_->SetVSRate    (param_->VSRate);
		camctl_->SetGain      (param_->gain);
		const ExpProcCBSlot &slot = boost::bind(&cameracs::expose_process, this, _1, _2, _3);
		camctl_->RegisterExposeProc(slot);
		// 设置相机监测量
		nfcam_->state = CAMCTL_IDLE;
		nfcam_->errcode = 0;
	}

	return nfdev->connected;
}

bool cameracs::connect_filter() {
	if (!param_->bFilter) return true;
	// 仅支持FLI滤光片控制器. Dec 10, 2018
	boost::shared_ptr<FilterCtrlFLI> fli = boost::make_shared<FilterCtrlFLI>();
	if (!fli->Connect()) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect filter controller");
	}
	else {
		_gLog.Write("successfully connect to filter controller <%s>",
				fli->GetDeviceName().c_str());
		int n(param_->nFilter);
		fli->SetLayerCount(1);
		fli->SetSlotCount(n);
		for (int i = 0; i < n; ++i) {
			fli->SetFilterName(0, i, param_->sFilter[i]);
		}
	}

	filterctl_ = boost::static_pointer_cast<FilterCtrl>(fli);
	return filterctl_->IsConnected();
}

void cameracs::connect_gtoaes() {
	const TCPClient::CBSlot &slot = boost::bind(&cameracs::handle_connect_gtoaes, this, _1, _2);

	tcptr_ = maketcp_client();
	tcptr_->RegisterConnect(slot);
	tcptr_->AsyncConnect(param_->hostGC, param_->portGC);
}

void cameracs::handle_connect_gtoaes(const long client, const long ec) {
	PostMessage(MSG_CONNECT_GTOAES, ec);
}

void cameracs::handle_receive_gtoaes(const long client, const long ec) {
	PostMessage(ec ? MSG_CLOSE_GTOAES : MSG_RECEIVE_GTOAES);
}

/*
 * 解析由通信协议发送的滤光片组合, 生成本地滤光片集合
 */
void cameracs::resolve_filter() {
	if (!nfobj_->filter.empty()) {
		string &filter = nfobj_->filter;
		char seps[] = "| ";
		listring tokens;

		filters_.clear();
		algorithm::split(tokens, filter, is_any_of(seps), token_compress_on);
		while(tokens.size()) {
			string name = tokens.front();
			tokens.pop_front();
			trim(name);
			filters_.push_back(name);
		}

		nfcam_->filter = filters_[nfobj_->ifilter];
	}
}

/*
 * 检查曝光序列是否结束
 * - 是否已到达曝光结束时间
 * - 当前曝光参数序列是否结束
 * - 是否需要切换滤光片
 * - 是否需要开始新的循环轮次
 */
bool cameracs::exposure_over() {
	bool over(false);

	try {// 若设置曝光结束时间
		ptime tmend = from_iso_extended_string(nfobj_->end_time);
		ptime now   = second_clock::universal_time();
		over = (now - tmend).total_seconds() > nfobj_->expdur;
	}
	catch(...) {}
	// 当前曝光参数序列已完成
	if (!over && nfcam_->ifrm == nfcam_->frmcnt) {
		if (!filters_.size()) over = true;
		else if (++nfcam_->ifilter == filters_.size()) {
			if (++nfcam_->iloop == nfcam_->loopcnt) over = true;
			else nfcam_->ifilter = 0;
		}

		if (!over) {
			nfcam_->ifrm = 0;
			nfcam_->filter = filters_[nfcam_->ifilter];
		}
	}
	if (over) nfobj_.reset();

	return over;
}

void cameracs::expose_process(const double left, const double percent, const int state) {
	switch((CAMERA_STATUS) state) {
	case CAMERA_ERROR: // 相机故障
		PostMessage(MSG_FAIL_EXPOSE);
		break;
	case CAMERA_IDLE: // 空闲
		PostMessage(MSG_ABORT_EXPOSE);
		break;
	case CAMERA_EXPOSE:	// 曝光中
		PostMessage(MSG_PROCESS_EXPOSE, long(left * 1E6), long(percent));
		break;
	case CAMERA_IMGRDY:	// 已下载图像数据, 可以执行存储等操作
		PostMessage(MSG_COMPLETE_EXPOSE);
		break;
	default:
		break;
	}
}

void cameracs::process_protocol(apbase proto) {
	string type = proto->type;

	if (iequals(type, APTYPE_OBJECT)) {// 观测目标及曝光参数
		if (!nfobj_.unique()) {
			nfobj_ = from_apbase<ascii_proto_object>(proto);
			_gLog.Write("New sequence: [name=%s, imgtype=%s, expdur=%.3f, frmcnt=%d]",
					nfobj_->objname.c_str(), nfobj_->imgtype.c_str(), nfobj_->expdur, nfobj_->frmcnt);
			*nfcam_ = *nfobj_;
			resolve_filter();
			if (ftcli_.unique()) {// 设置天区划分模式
				ftcli_->SetObject(nfobj_->grid_id, nfobj_->field_id);
			}
			// 设置平场初始曝光时间
			if (iequals(nfobj_->imgtype, "flat")) nfcam_->expdur = param_->flatMinT;
		}
		else {
			_gLog.Write(LOG_WARN, NULL, "<object> is rejected for system is busy");
		}
	}
	else if (iequals(type, APTYPE_EXPOSE)) {// 曝光指令
		if (nfobj_.unique()) {
			command_expose(EXPOSE_COMMAND(from_apbase<ascii_proto_expose>(proto)->command));
		}
	}
	else if (iequals(type, APTYPE_COOLER)) {// GWAC-GY CCD相机变更制冷温度
		const NFDevCamPtr nfdev = camctl_->GetCameraInfo();
		nfdev->coolerGet = from_apbase<ascii_proto_cooler>(proto)->coolget;
	}
	else if (iequals(type, APTYPE_FOCUS)) {// 变更焦点位置
		nfcam_->focus = from_apbase<ascii_proto_focus>(proto)->value;
	}
	else if (iequals(type, APTYPE_TELE)) {// 望远镜指向位置. 平场变更位置
		if (nfobj_.unique()) {
			boost::shared_ptr<ascii_proto_telescope> tele = from_apbase<ascii_proto_telescope>(proto);
			nfobj_->ra  = tele->ra;
			nfobj_->dec = tele->dec;
		}
	}
	else if (iequals(type, APTYPE_MCOVER)) {// 变更镜盖状态
		nfcam_->mcstate = from_apbase<ascii_proto_mcover>(proto)->value;
	}
	else if (iequals(type, APTYPE_OBSITE)) {// 注册结果. 反馈测站参数
		obsite_ = from_apbase<ascii_proto_obsite>(proto);
	}
	else if (iequals(type, APTYPE_OBSSTYPE)) {// 注册结果, 观测系统类型
		ostype_ = OBSS_TYPE(from_apbase<ascii_proto_obsstype>(proto)->obsstype);
	}
}

void cameracs::command_expose(EXPOSE_COMMAND cmd) {
	if (cmdexp_ != cmd) {
		CAMCTL_STATUS state = CAMCTL_STATUS(nfcam_->state);
		if ((cmd == EXPOSE_RESUME && (state == CAMCTL_PAUSE || state >= CAMCTL_WAIT_SYNC))
				|| (cmd == EXPOSE_START && state == CAMCTL_IDLE)) {// 开始或恢复曝光
			PostMessage(MSG_PREPARE_EXPOSE);
		}
		else if (cmd == EXPOSE_PAUSE) {// 暂停曝光

		}
		else if (cmd == EXPOSE_STOP) {// 中止曝光

		}
		cmdexp_ = cmd;
	}
}

//////////////////////////////////////////////////////////////////////////////
/*
 * 尝试重新连接服务器
 * - 每次失败延时增加1分钟
 * - 最长延时等待5分钟
 */
void cameracs::thread_tryconnect_gtoaes() {
	int count(0);

	while(1) {
		boost::this_thread::sleep_for(boost::chrono::minutes(++count));
		if (count > 5) count = 5;
		connect_gtoaes();
	}
}

/*
 * 尝试重新连接相机
 * - 每次失败延时增加1分钟
 * - 最长延时等待5分钟
 */
void cameracs::thread_tryconnect_camera() {
	int count(0);
	bool success;

	do {
		boost::this_thread::sleep_for(boost::chrono::minutes(++count));
		if (count > 5) count = 5;
		success = connect_camera();
	} while(!success);
	if (success) PostMessage(MSG_CONNECT_CAMERA);
}

/*
 * 尝试重新连接滤光片控制器
 * - 每次失败延时增加1分钟
 * - 最长延时等待5分钟
 */
void cameracs::thread_tryconnect_filter() {
	int count(0);
	bool success;

	do {
		boost::this_thread::sleep_for(boost::chrono::minutes(++count));
		if (count > 5) count = 5;
		success = connect_filter();
	} while(!success);
	if (success) PostMessage(MSG_CONNECT_FILTER);
}

/*
 * 向服务器上传相机工作状态
 * - 定时周期: 10秒
 * - 当工作状态发生变化时
 */
void cameracs::thread_upload() {
	boost::chrono::seconds period(10);
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while(1) {
		cv_statchanged_.wait_for(lck, period);

		if (camctl_.unique()) {
			int n;
			const char *s;
			nfcam_->coolget = camctl_->GetCameraInfo()->coolerGet;
			s = ascproto_->CompactCamera(nfcam_, n);
			tcptr_->Write(s, n);
		}
	}
}

/*
 * 定时检查清理磁盘空间
 */
void cameracs::thread_noon() {
	while (1) {
		free_disk();
		boost::this_thread::sleep_for(boost::chrono::seconds(next_noon()));
		// 重新连接滤光片控制器
		filterctl_.reset();
		thrd_filter_.reset(new boost::thread(boost::bind(&cameracs::thread_tryconnect_filter, this)));
	}
}

/*
 * 空闲时校正本地时钟
 */
void cameracs::thread_clocksync() {
	boost::chrono::minutes period(1);

	while(1) {
		boost::this_thread::sleep_for(period);
		if (nfcam_->state == CAMCTL_IDLE) ntp_->SynchClock();
	}
}

/*
 * 清理磁盘空间
 */
void cameracs::free_disk() {
	namespace fs = boost::filesystem;
	int days3 = 3 * 86400; // 删除3日前数据
	ptime now = second_clock::local_time(), tmlast;
	fs::path path = param_->pathLocal;
	fs::space_info space = fs::space(path);

	// 当磁盘容量不足时, 删除3日前数据
	if ((space.available >> 30) <= param_->minFreeDisk) {
		_gLog.Write("Cleaning local storage <%s>", path.c_str());
		fs::directory_iterator itend = fs::directory_iterator();
		for (fs::directory_iterator x = fs::directory_iterator(path); x != itend; ++x) {
			tmlast = from_time_t(fs::last_write_time(x->path()));
			if ((now - tmlast).total_seconds() > days3) fs::remove_all(x->path());
		}

		space = fs::space(path);
		_gLog.Write("free capacity is %d GB", space.available >> 30);
	}
}

long cameracs::next_noon() {
	ptime now(second_clock::local_time());
	ptime noon(now.date(), hours(12));
	long secs = (noon - now).total_seconds();
	return secs < 10 ? secs + 86400 : secs;
}

//////////////////////////////////////////////////////////////////////////////
void cameracs::create_filepath() {
	if (filepath_.newseq) {// 创建文件路径

	}
	// 创建文件名
}

bool cameracs::save_fitsfile() {
	fitsfile *fitsptr;
	int status(0);
	int naxis(2);
	const NFDevCamPtr nfdev = camctl_->GetCameraInfo();
	long naxes[] = {nfdev->roi.Width(), nfdev->roi.Height()};
	long pixels = nfdev->roi.Width() * nfdev->roi.Height();
	int tmpint;
	string tmpstr;

	/* 1: 创建文件并保存图像数据 */
	fits_create_file(&fitsptr, filepath_.filepath.c_str(), &status);
	if (nfdev->ADBitPixel <= 8) {
		fits_create_img(fitsptr, BYTE_IMG, naxis, naxes, &status);
		fits_write_img(fitsptr, TBYTE, 1, pixels, nfdev->data.get(), &status);
	}
	else if (nfdev->ADBitPixel <= 16) {
		fits_create_img(fitsptr, USHORT_IMG, naxis, naxes, &status);
		fits_write_img(fitsptr, TUSHORT, 1, pixels, nfdev->data.get(), &status);
	}
	else {
		fits_create_img(fitsptr, ULONG_IMG, naxis, naxes, &status);
		fits_write_img(fitsptr, TUINT, 1, pixels, nfdev->data.get(), &status);
	}
	/* 2: 曝光参数 */
	fits_write_key(fitsptr, TSTRING, "CCDTYPE",  (void*)nfobj_->imgtype.c_str(), "type of image",                   &status);
	fits_write_key(fitsptr, TSTRING, "DATE-OBS", (void*)nfdev->dateobs.c_str(),  "UTC date of begin observation",   &status);
	fits_write_key(fitsptr, TSTRING, "TIME-OBS", (void*)nfdev->timeobs.c_str(),  "UTC time of begin observation",   &status);
	fits_write_key(fitsptr, TSTRING, "TIME-END", (void*)nfdev->timeend.c_str(),  "UTC time of end observation",     &status);
	fits_write_key(fitsptr, TDOUBLE, "JD",       &nfdev->jd,                     "Julian day of begin observation", &status);
	fits_write_key(fitsptr, TFLOAT,  "EXPTIME",  &nfdev->expdur,                 "exposure duration",               &status);
	if (nfcam_->filter.size()) {
		fits_write_key(fitsptr, TSTRING, "FILTER", (void*)nfcam_->filter.c_str(), "name of selected filter", &status);
	}
	/* 3: 观测目标 */
	if (nfobj_->plan_sn != INT_MAX)
		fits_write_key(fitsptr, TINT,    "PLAN_SN",  &nfobj_->plan_sn, "plan number", &status);
	if (nfobj_->plan_time.size())
		fits_write_key(fitsptr, TSTRING, "PLANTIME", (void*)nfobj_->plan_time.c_str(), "plan time", &status);
	if (nfobj_->plan_type.size())
		fits_write_key(fitsptr, TSTRING, "PLANTYPE", (void*)nfobj_->plan_type.c_str(), "plan type", &status);
	if (nfobj_->observer.size())
		fits_write_key(fitsptr, TSTRING, "OBSERVER", (void*)nfobj_->observer.c_str(), "observer name", &status);
	if (nfobj_->obstype.size())
		fits_write_key(fitsptr, TSTRING, "OBSTYPE",  (void*)nfobj_->obstype.c_str(), "observation type", &status);
	if (nfobj_->grid_id.size())
		fits_write_key(fitsptr, TSTRING, "GRID_ID",  (void*)nfobj_->grid_id.c_str(), "grid id", &status);
	if (nfobj_->field_id.size())
		fits_write_key(fitsptr, TSTRING, "FIELD_ID", (void*)nfobj_->field_id.c_str(), "sky field id", &status);
	if (nfobj_->objname.size())
		fits_write_key(fitsptr, TSTRING, "OBJNAME",  (void*)nfobj_->objname, "object name", &status);
	if (nfobj_->runname.size())
		fits_write_key(fitsptr, TSTRING, "RUN_NAME", (void*)nfobj_->runname, "object name in this run", &status);
	if (valid_ra(nfobj_->ra) && valid_dec(nfobj_->dec)) {
		fits_write_key(fitsptr, TDOUBLE, "RA",    &nfobj_->ra,    "RA of field center", &status);
		fits_write_key(fitsptr, TDOUBLE, "DEC",   &nfobj_->dec,   "DEC of field center", &status);
		fits_write_key(fitsptr, TDOUBLE, "EPOCH", &nfobj_->epoch, "epoch of center position", &status);
	}
	if (valid_ra(nfobj_->objra) && valid_dec(nfobj_->objdec)) {
		fits_write_key(fitsptr, TDOUBLE, "OBJRA",    &nfobj_->objra,    "object RA", &status);
		fits_write_key(fitsptr, TDOUBLE, "OBJDEC",   &nfobj_->objdec,   "object DEC", &status);
		fits_write_key(fitsptr, TDOUBLE, "OBJEPOCH", &nfobj_->objepoch, "epoch of object position", &status);
	}
	if (nfobj_->objerror.size())
		fits_write_key(fitsptr, TSTRING, "OBJERR",   (void*)nfobj_->objerror.c_str(), "error bar of position", &status);
	if (nfobj_->priority > 0)
		fits_write_key(fitsptr, TINT,    "PRIORITY", &nfobj_->priority, "plan priority", &status);
	if (nfobj_->begin_time.size())
		fits_write_key(fitsptr, TSTRING, "BEGIN_T", (void*)nfobj_->begin_time.c_str(), "expected begin time", &status);
	if (nfobj_->end_time.size())
		fits_write_key(fitsptr, TSTRING, "END_T",   (void*)nfobj_->end_time.c_str(), "expected end time", &status);
	if (nfcam_->focus > INT_MIN)
		fits_write_key(fitsptr, TINT,    "TELFOCUS", &nfcam_->focus,    "focus position in micron", &status);
	if (nfobj_->pair_id >= 0)
		fits_write_key(fitsptr, TINT,    "PAIR_ID", &nfobj_->pair_id, "pair id", &status);

	tmpint = nfcam_->iloop * nfcam_->frmcnt + nfcam_->ifrm;
	fits_write_key(fitsptr, TINT, "FRAMENO", &tmpint, "frame number in sequence", &status);
	/* 4: FITS头: 观测设备参数 */
	fits_write_key(fitsptr, TSTRING, "GROUP_ID", (void*)param_->gid.c_str(),     "group id",                        &status);
	fits_write_key(fitsptr, TSTRING, "UNIT_ID",  (void*)param_->uid.c_str(),     "unit id",                         &status);
	fits_write_key(fitsptr, TSTRING, "CAM_ID",   (void*)param_->cid.c_str(),     "camera id",                       &status);
	fits_write_key(fitsptr, TSTRING, "READPORT", (void*)nfdev->ReadPort.c_str(), "Readout Port",                    &status);
	fits_write_key(fitsptr, TSTRING, "READRATE", (void*)nfdev->ReadRate.c_str(), "Readout Rate",                    &status);
	fits_write_key(fitsptr, TFLOAT,  "GAIN",     &nfdev->gain,                   "PreAmp Gain",                     &status);
	fits_write_key(fitsptr, TFLOAT,  "VSSPD",    &nfdev->VSRate,                 "line transfer rate in micro-sec", &status);
	fits_write_key(fitsptr, TFLOAT,  "TEMPSET",  &nfdev->coolerSet,              "temperature setpoint",            &status);
	fits_write_key(fitsptr, TFLOAT,  "TEMPDET",  &nfdev->coolerGet,              "actual sensor temperature",       &status);
	if (nfdev->sensorW != nfdev->roi.Width() || nfdev->sensorH != nfdev->roi.Height()) {
		fits_write_key(fitsptr, TINT, "XORGSUBF", &nfdev->roi.startX, "subregion origin on X axis", &status);
		fits_write_key(fitsptr, TINT, "YORGSUBF", &nfdev->roi.startY, "subregion origin on Y axis", &status);
		fits_write_key(fitsptr, TINT, "XBINNING", &nfdev->roi.binX,   "binning factor on X axis",   &status);
		fits_write_key(fitsptr, TINT, "YBINNING", &nfdev->roi.binY,   "binning factor on Y axis",   &status);
	}
	if (nfdev->EMOn) {
		fits_write_key(fitsptr, TINT, "EGAIN", &nfdev->EMGain, "electronic gain in e- per ADU", &status);
	}
	fits_write_key(fitsptr, TFLOAT,  "XPIXSZ",   &nfdev->pixelX, "pixel X in microns", &status);
	fits_write_key(fitsptr, TFLOAT,  "YPIXSZ",   &nfdev->pixelY, "pixel Y in microns", &status);
	tmpstr = nfdev->model;
	if (!nfdev->serialno.empty()) tmpstr = tmpstr + " : " + nfdev->serialno;
	fits_write_key(fitsptr, TSTRING, "CCDMODEL", (void*)tmpstr.c_str(), "camera model", &status);
	fits_write_key(fitsptr, TSTRING, "SITENAME", (void*)obsite_->sitename.c_str(), "geo longitude in degrees", &status);
	fits_write_key(fitsptr, TDOUBLE, "SITELONG", &obsite_->lgt, "geo longitude in degrees", &status);
	fits_write_key(fitsptr, TDOUBLE, "SITELAT",  &obsite_->lat,  "geo latitude in degrees", &status);
	fits_write_key(fitsptr, TDOUBLE, "SITEELEV", &obsite_->alt,  "geo altitude in meters", &status);
	fits_write_key(fitsptr, TINT,    "APTDIA",   &param_->aptdia, "diameter of the telescope in centimeters", &status);
	fits_write_key(fitsptr, TINT,    "FOCLEN",   &param_->focalen, "focus length in millimeters", &status);
	fits_write_key(fitsptr, TSTRING, "FOCUS",    (void*)param_->focus.c_str(), "focus type", &status);

	/* 5: 关闭文件 */
	fits_close_file(fitsptr, &status);
	if (status) {
		char txt[200];
		fits_get_errstatus(status, txt);
		_gLog.Write(LOG_FAULT, NULL, "failed to save FITS file[%s]: %s", filepath_.filepath.c_str(), txt);
	}
	return (status == 0);
}

/*
 * 评估平场有效性, 返回值定义:
 * - & 0x01 != 0时, 表示数据有效
 * - & 0x10 != 0时, 表示曝光时间在有效范围内
 * 安装上述定义, 返回值选项为:
 * 0x00: 无效平场, 调节后曝光时间超出有效范围
 * 0x01: 有效平场, 调节后曝光时间超出有效范围
 * 0x10: 无效平场, 调节后曝光时间仍然在有效范围内
 * 0x11: 有效平场, 调节后曝光时间仍然在有效范围内
 */
int cameracs::assess_flat() {
	int rslt(0);
	/* 1: 统计图像中心区域平均值, 作为平场有效性评估依据 */
	int mean;
	const NFDevCamPtr nfdev = camctl_->GetCameraInfo();
	int w = nfdev->roi.Width();
	int h = nfdev->roi.Height();
	if (nfdev->ADBitPixel <= 8) {
		mean = asses_flat((uint8_t*) (nfdev->data.get()), w, h);
	}
	else if (nfdev->ADBitPixel <= 16) {
		mean = asses_flat((uint16_t*) (nfdev->data.get()), w, h);
	}
	else if (nfdev->ADBitPixel <= 32) {
		mean = asses_flat((uint32_t*) (nfdev->data.get()), w, h);
	}
	// 饱和反转
	if (mean <= param_->tsaturate) mean += (0x1 << nfdev->ADBitPixel);

	/* 2: 评估平场有效性 */
	if (mean >= param_->flatMin && mean <= param_->flatMax) rslt |= 0x01;

	/* 3: 修正曝光时间 */
	double tmin(param_->flatMinT), tmax(param_->flatMaxT);
	double t1 = nfcam_->expdur * param_->flatMin / mean;
	double t2 = nfcam_->expdur * param_->flatMax / mean;
	double t;
	bool c1, c2;

	if (   (c1 = nfdev->ampm && t1 > tmax)   // 早上: 还没到达合适的平场时间
		|| (c2 = !nfdev->ampm && t2 < tmin)  // 下午: 还没到达合适的平场时间
		|| (t1 <= tmax && t2 >= tmin)) {     // 平场时间
		if      (c1) t = tmax;
		else if (c2) t = tmin;
		else {
			t = nfdev->ampm ? t2 * 0.95 : t2 * 1.05;
			if      (t < tmin) t = tmin;
			else if (t > tmax) t = tmax;
		}
		nfobj_->expdur = t;
		rslt |= 0x10;
	}

	return rslt;
}

/*
 * 将新产生的FITS文件上传到服务器
 */
void cameracs::upload_file() {
	if (ftcli_.unique()) {
		FileTransferClient::FileDescPtr fileptr = ftcli_->NewFile();
		fileptr->tmobs    = to_iso_extended_string(camctl_->GetCameraInfo()->tmobs);
		fileptr->filename = filepath_.filename;
		fileptr->subpath  = filepath_.subpath;
		fileptr->filepath = filepath_.filepath;
		ftcli_->UploadFile(fileptr);
	}
}

/*
 * 显示新产生的FITS图像
 */
void cameracs::display_image() {
	if (param_->bShowImg) cds9_->DisplayImage(filepath_.filepath.c_str());
}

//////////////////////////////////////////////////////////////////////////////
void cameracs::register_messages() {
	const CBSlot &slot01 = boost::bind(&cameracs::on_connect_gtoaes,  this, _1, _2);
	const CBSlot &slot02 = boost::bind(&cameracs::on_receive_gtoaes,  this, _1, _2);
	const CBSlot &slot03 = boost::bind(&cameracs::on_close_gtoaes,    this, _1, _2);
	const CBSlot &slot04 = boost::bind(&cameracs::on_connect_camera,  this, _1, _2);
	const CBSlot &slot05 = boost::bind(&cameracs::on_connect_filter,  this, _1, _2);
	const CBSlot &slot06 = boost::bind(&cameracs::on_prepare_expose,  this, _1, _2);
	const CBSlot &slot07 = boost::bind(&cameracs::on_process_expose,  this, _1, _2);
	const CBSlot &slot08 = boost::bind(&cameracs::on_complete_expose, this, _1, _2);
	const CBSlot &slot09 = boost::bind(&cameracs::on_abort_expose,    this, _1, _2);
	const CBSlot &slot10 = boost::bind(&cameracs::on_fail_expose,     this, _1, _2);

	RegisterMessage(MSG_CONNECT_GTOAES,  slot01);
	RegisterMessage(MSG_RECEIVE_GTOAES,  slot02);
	RegisterMessage(MSG_CLOSE_GTOAES,    slot03);
	RegisterMessage(MSG_CONNECT_CAMERA,  slot04);
	RegisterMessage(MSG_CONNECT_FILTER,  slot05);
	RegisterMessage(MSG_PREPARE_EXPOSE,  slot06);
	RegisterMessage(MSG_PROCESS_EXPOSE,  slot07);
	RegisterMessage(MSG_COMPLETE_EXPOSE, slot08);
	RegisterMessage(MSG_ABORT_EXPOSE,    slot09);
	RegisterMessage(MSG_FAIL_EXPOSE,     slot10);
}

void cameracs::on_connect_gtoaes(const long ec, const long) {
	if (!ec) {// 注册相机并销毁线程
		interrupt_thread(thrd_gtoaes_);
		// 注册回调函数
		const TCPClient::CBSlot &slot = boost::bind(&cameracs::handle_receive_gtoaes, this, _1, _2);
		tcptr_->RegisterRead(slot);
		// 注册相机
		_gLog.Write("register camera as [%s:%s:%s]",
				param_->gid.c_str(), param_->uid.c_str(), param_->cid.c_str());

		int n;
		const char *s;
		apreg proto = boost::make_shared<ascii_proto_reg>();
		proto->gid = param_->gid;
		proto->uid = param_->uid;
		proto->cid = param_->cid;
		s = ascproto_->CompactRegister(proto, n);
		tcptr_->Write(s, n);
		// 启动线程
		thrd_upload_.reset(new boost::thread(boost::bind(&cameracs::thread_upload, this)));
	}
	else if (!thrd_gtoaes_.unique()) {
		thrd_gtoaes_.reset(new boost::thread(boost::bind(&cameracs::thread_tryconnect_gtoaes, this)));
	}
}

void cameracs::on_receive_gtoaes(const long, const long) {
	char term[] = "\n";	   // 换行符作为信息结束标记
	int len = strlen(term);// 结束符长度
	int pos;      // 标志符位置
	int toread;   // 信息长度
	apbase proto;

	while (tcptr_->IsOpen() && (pos = tcptr_->Lookup(term, len)) >= 0) {
		if ((toread = pos + len) > TCP_PACK_SIZE) {
			_gLog.Write(LOG_FAULT, "cameracs::on_receive_gtoaes",
					"too long message from server");
			tcptr_->Close();
		}
		else {// 读取协议内容并解析执行
			tcptr_->Read(bufrcv_.get(), toread);
			bufrcv_[pos] = 0;

			proto = ascproto_->Resolve(bufrcv_.get());
			// 检查: 协议有效性及设备标志基本有效性
			if (proto.use_count()) process_protocol(proto);
			else {
				_gLog.Write(LOG_FAULT, "cameracs::on_receive_gtoaes",
						"received illegal protocol: %s", bufrcv_.get());
				tcptr_->Close();
			}
		}
	}
}

void cameracs::on_close_gtoaes(const long, const long) {
	_gLog.Write(LOG_WARN, NULL, "connection with server was broken");
	tcptr_->Close();
	interrupt_thread(thrd_upload_);
	thrd_gtoaes_.reset(new boost::thread(boost::bind(&cameracs::thread_tryconnect_gtoaes, this)));
}

void cameracs::on_connect_camera(const long, const long) {
	_gLog.Write("successfully re-connection to camera");
	interrupt_thread(thrd_camera_);
}

void cameracs::on_connect_filter(const long, const long) {
	_gLog.Write("successfully re-connection to filter controller");
	interrupt_thread(thrd_filter_);
}

void cameracs::on_prepare_expose(const long, const long) {
	if ((filepath_.newseq = !nfcam_->ifrm || nfobj_->ifrm == nfcam_->ifrm)) {// 新的观测序列
		// 检查并重定位滤光片
		if (filterctl_.unique() && filterctl_->Goto(nfcam_->filter)) {
			_gLog.Write("filter switches to [%s]", nfcam_->filter.c_str());
		}
	}
	nfcam_->expdur = nfobj_->expdur; // 平场可能调整曝光时间
	// 计算曝光前延时或直接开始曝光
}

/*
 * 进入条件: 曝光过程中
 */
void cameracs::on_process_expose(const long, const long) {
	//...暂不处理
}

/*
 * 进入条件: 图像数据已读入内存
 */
void cameracs::on_complete_expose(const long, const long) {
	int state = nfcam_->state;
	/* 1: 处理已完成曝光数据 */
	bool is_flat = iequals(nfobj_->imgtype, "flat");
	bool tosave(true), tocont(true);

	if (is_flat) {
		int rslt = assess_flat();
		tosave = rslt & 0x01;
		tocont = rslt & 0x10;
	}
	if (tosave) {
		++nfcam_->ifrm;
		create_filepath();
		if (save_fitsfile()) { // 保存文件
			nfcam_->filename = filepath_.filename;
			upload_file();   // 上传文件
			display_image(); // 显示图像
		}
		// 更新工作状态
		nfcam_->state = CAMCTL_COMPLETE;
	}

	/* 2: 启动后续曝光 */
	tocont = tocont && (cmdexp_ == EXPOSE_START || cmdexp_ == EXPOSE_RESUME) && !exposure_over();
	if (tocont) {
		if (is_flat) {// 平场: 等待新的曝光指令或重新定位
			nfcam_->state = tosave ? CAMCTL_WAIT_FLAT : CAMCTL_WAIT_SYNC;
		}
		else PostMessage(MSG_PREPARE_EXPOSE);
	}
	else {
		bool paused = cmdexp_ == EXPOSE_PAUSE;
		nfcam_->state = paused ? CAMCTL_PAUSE : CAMCTL_IDLE;
		_gLog.Write("exposure sequence is %s", paused ? "suspend" : "over");
	}
	/* 3: 通知工作状态发生变化 */
	if (state != nfcam_->state) cv_statchanged_.notify_one();
}

/*
 * 进入条件: 相机空闲
 * - 启动曝光流程后, 响应指令进入空闲状态
 */
void cameracs::on_abort_expose(const long, const long) {
	if (cmdexp_ == EXPOSE_STOP) {
		_gLog.Write("exposure sequence is aborted");
		nfcam_->state = CAMCTL_IDLE;
	}
	else if (cmdexp_ == EXPOSE_PAUSE) {
		_gLog.Write("exposure sequence is suspend");
		nfcam_->state = CAMCTL_PAUSE;
	}
	cv_statchanged_.notify_one();
}

/*
 * 进入条件: 相机故障
 */
void cameracs::on_fail_expose(const long, const long) {
	// 设置相机状态
	_gLog.Write(LOG_FAULT, NULL, "exposure failed: %s",
			camctl_->GetCameraInfo()->errmsg.c_str());
	nfcam_->state = CAMCTL_ERROR;
	cv_statchanged_.notify_one();
	// 重新连接相机
	_gLog.Write("camera fall in trouble. try to re-connect camera");
	camctl_->Disconnect();
	thrd_camera_.reset(new boost::thread(boost::bind(&cameracs::thread_tryconnect_camera, this)));
}
