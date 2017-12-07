/*!
 * @file AsciiProtocol.cpp 定义文件, 定义GWAC/GFT系统中字符串型通信协议相关操作
 * @version 0.1
 * @date 2017-11-17
 **/

#include <boost/make_shared.hpp>
#include <stdlib.h>
#include "AsciiProtocol.h"

using namespace boost;

//////////////////////////////////////////////////////////////////////////////
/* 为各协议生成共享型指针 */
/* 注册设备及注册结果 */
apreg makeap_reg() {
	return boost::make_shared<ascii_proto_reg>();
}

/* 转台 */
apfindhome makeap_findhome() {
	return boost::make_shared<ascii_proto_find_home>();
}

aphomesync makeap_homesync() {
	return boost::make_shared<ascii_proto_home_sync>();
}

apslewto makeap_slewto() {
	return boost::make_shared<ascii_proto_slewto>();
}

appark makeap_park() {
	return boost::make_shared<ascii_proto_park>();
}

apguide makeap_guide() {
	return boost::make_shared<ascii_proto_guide>();
}

apabortslew makeap_abortslew() {
	return boost::make_shared<ascii_proto_abort_slew>();
}

apnftele makeap_telescopeinfo() {
	return boost::make_shared<ascii_proto_telescope_info>();
}

apfwhm makeap_fwhm() {
	return boost::make_shared<ascii_proto_fwhm>();
}

apfocus makeap_focus() {
	return boost::make_shared<ascii_proto_focus>();
}

apfocusync makeap_focusync() {
	return boost::make_shared<ascii_proto_focus_sync>();
}

apmcover makeap_mcover() {
	return boost::make_shared<ascii_proto_mcover>();
}

/* 相机 -- 上层 */
aptakeimg makeap_takeimg() {
	return boost::make_shared<ascii_proto_takeimg>();
}

apabortimg makeap_abortimg() {
	return boost::make_shared<ascii_proto_abortimg>();
}

/* 相机 -- 底层 */
apobject makeap_object() {
	return boost::make_shared<ascii_proto_object>();
}

apexpose makeap_expose() {
	return boost::make_shared<ascii_proto_expose>();
}

apnfcam makeap_camerainfo() {
	return boost::make_shared<ascii_proto_camera_info>();
}

/* GWAC相机辅助程序通信协议: 温度和真空度 */
apcooler makeap_cooler() {
	return boost::make_shared<ascii_proto_cooler>();
}

apvacuum makeap_vacuum() {
	return boost::make_shared<ascii_proto_vacuum>();
}

/* FITS文件传输 */
apfileinfo makeap_fileinfo() {
	return boost::make_shared<ascii_proto_fileinfo>();
}

apfilestat makeap_filestat() {
	return boost::make_shared<ascii_proto_filestat>();
}

AscProtoPtr make_ascproto() {
	return boost::make_shared<AsciiProtocol>();
}
//////////////////////////////////////////////////////////////////////////////

AsciiProtocol::AsciiProtocol() {
	ibuf_ = 0;
	buff_.reset(new char[1024 * 10]); //< 存储区
}

AsciiProtocol::~AsciiProtocol() {
}

const char* AsciiProtocol::output_compacted(string& output, int& n) {
	mutex_lock lck(mtx_);
	char* buff = buff_.get() + ibuf_ * 1024;
	if (++ibuf_ == 10) ibuf_ = 0;
	trim_right_if(output, is_punct() || is_space());
	n = sprintf(buff, "%s\n", output.c_str());
	return buff;
}

bool AsciiProtocol::resolve_kv(string& kv, string& keyword, string& value) {
	char seps[] = "=";	// 分隔符: 等号
	listring tokens;

	keyword = "";
	value   = "";
	algorithm::split(tokens, kv, is_any_of(seps), token_compress_on);
	if (!tokens.empty()) { keyword = tokens.front(); trim(keyword); tokens.pop_front(); }
	if (!tokens.empty()) { value   = tokens.front(); trim(value); }
	return (!(keyword.empty() || value.empty()));
}

//////////////////////////////////////////////////////////////////////////////
/*---------------- 封装通信协议 ----------------*/
const char *AsciiProtocol::CompactRegister(apreg proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "cam_id",   proto->cid);
	join_kv(output, "result",   proto->result);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFindHome(apfindhome proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactHomeSync(aphomesync proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dc);
	join_kv(output, "epoch",    proto->epoch);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactSlewto(apslewto proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dc);
	join_kv(output, "epoch",    proto->epoch);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactPark(appark proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactGuide(apguide proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dc);
	join_kv(output, "objra",    proto->objra);
	join_kv(output, "objdec",   proto->objdc);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactAbortSlew(apabortslew proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactTelescopeInfo(apnftele proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "state",    proto->state);
	join_kv(output, "ec",       proto->ec);
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dc);
	join_kv(output, "objra",    proto->objra);
	join_kv(output, "objdec",   proto->objdc);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFWHM(apfwhm proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "cam_id",   proto->uid);
	join_kv(output, "value",    proto->value);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFocus(apfocus proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "cam_id",   proto->uid);
	join_kv(output, "value",    proto->value);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFocusSync(apfocusync proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "cam_id",   proto->uid);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactMirrorCover(apmcover proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "cam_id",   proto->uid);
	join_kv(output, "value",    proto->value);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactObject(apobject proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	join_kv(output, "op_sn",    proto->op_sn);
	join_kv(output, "op_time",  proto->op_time);
	join_kv(output, "op_type",  proto->op_type);
	join_kv(output, "observer", proto->observer);
	join_kv(output, "obstype",  proto->obstype);
	join_kv(output, "grid_id",  proto->grid_id);
	join_kv(output, "field_id", proto->field_id);
	join_kv(output, "obj_id",   proto->obj_id);
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dc);
	join_kv(output, "epoch",    proto->epoch);
	join_kv(output, "objra",    proto->objra);
	join_kv(output, "objdec",   proto->objdc);
	join_kv(output, "objepoch", proto->objepoch);
	join_kv(output, "objerror", proto->objerror);
	join_kv(output, "imgtype",  proto->imgtype);
	join_kv(output, "expdur",   proto->expdur);
	join_kv(output, "delay",    proto->delay);
	join_kv(output, "frmcnt",   proto->frmcnt);
	join_kv(output, "priority", proto->priority);
	join_kv(output, "tmbegin",  proto->tmbegin);
	join_kv(output, "tmend",    proto->tmend);
	join_kv(output, "pair_id",  proto->pair_id);
	join_kv(output, "filter",   proto->filter);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactExpose(apexpose proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";
	join_kv(output, "command", proto->command);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactCameraInfo(apnfcam proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";
	join_kv(output, "bitdepth",  proto->bitdepth);
	join_kv(output, "readport",  proto->readport);
	join_kv(output, "readrate",  proto->readrate);
	join_kv(output, "vrate",     proto->vrate);
	join_kv(output, "gain",      proto->gain);
	join_kv(output, "connected", proto->connected);
	join_kv(output, "state",     proto->state);
	join_kv(output, "errcode",   proto->errcode);
	join_kv(output, "coolget",   proto->coolget);
	join_kv(output, "coolset",   proto->coolset);
	join_kv(output, "filter",    proto->filter);
	join_kv(output, "frmno",     proto->frmno);
	join_kv(output, "freedisk",  proto->freedisk);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactCooler(apcooler proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "cam_id",   proto->cid);
	join_kv(output, "voltage",  proto->voltage);
	join_kv(output, "current",  proto->current);
	join_kv(output, "hotend",   proto->hotend);
	join_kv(output, "coolget",  proto->coolget);
	join_kv(output, "coolset",  proto->coolset);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactVacuum(apvacuum proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";

	if (!proto->utc.empty()) join_kv(output, "time", proto->utc);
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "cam_id",   proto->cid);
	join_kv(output, "voltage",  proto->voltage);
	join_kv(output, "current",  proto->current);
	join_kv(output, "pressure", proto->pressure);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFileInfo(apfileinfo proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";
	join_kv(output, "group_id", proto->gid);
	join_kv(output, "unit_id",  proto->uid);
	join_kv(output, "cam_id",   proto->cid);
	join_kv(output, "grid_id",  proto->grid);
	join_kv(output, "field_id", proto->field);
	join_kv(output, "tmobs",    proto->tmobs);
	join_kv(output, "subpath",  proto->subpath);
	join_kv(output, "filename", proto->filename);
	join_kv(output, "filesize", proto->filesize);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFileStat(apfilestat proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output = proto->type + " ";
	join_kv(output, "status", proto->status);
	return output_compacted(output, n);
}

//////////////////////////////////////////////////////////////////////////////
apbase AsciiProtocol::Resolve(const char *rcvd) {
	const char seps[] = ",", *ptr;
	char ch;
	listring tokens;
	apbase proto;
	string type;

	for (ptr = rcvd; *ptr && *ptr != ' '; ++ptr) type += *ptr;
	while (*ptr && *ptr == ' ') ++ptr;

	if (*ptr) algorithm::split(tokens, ptr, is_any_of(seps), token_compress_on);
	if ((ch = type[0]) == 'a') {
		if      (iequals(type, "abort_slew")) proto = resolve_abortslew(tokens);
		else if (iequals(type, "abort_image")) proto = resolve_abortimg(tokens);
	}
	else if (ch == 'c') {
		if (iequals(type, "cooler"))            proto = resolve_cooler(tokens);
		else if (iequals(type, "camera_info"))  proto = resolve_camerainfo(tokens);
	}
	else if (ch == 'e') {
		if (iequals(type, "expose")) proto = resolve_expose(tokens);
	}
	else if (ch == 'f') {
		if      (iequals(type, "fileinfo"))    proto = resolve_fileinfo(tokens);
		else if (iequals(type, "filestat"))    proto = resolve_filestat(tokens);
		else if (iequals(type, "find_home"))   proto = resolve_findhome(tokens);
		else if (iequals(type, "fwhm"))        proto = resolve_fwhm(tokens);
		else if (iequals(type, "focus"))       proto = resolve_focus(tokens);
		else if (iequals(type, "focus_sync"))  proto = resolve_focusync(tokens);
	}
	else if (iequals(type, "guide"))          proto = resolve_guide(tokens);
	else if (iequals(type, "home_sync"))      proto = resolve_homesync(tokens);
	else if (iequals(type, "mcover"))         proto = resolve_mcover(tokens);
	else if (iequals(type, "object"))         proto = resolve_object(tokens);
	else if (iequals(type, "park"))           proto = resolve_park(tokens);
	else if (iequals(type, "register"))       proto = resolve_register(tokens);
	else if (iequals(type, "slewto"))         proto = resolve_slewto(tokens);
	else if (ch == 't'){
		if      (iequals(type, "telescope_info")) proto = resolve_telescopeinfo(tokens);
		else if (iequals(type, "take_image"))     proto = resolve_takeimg(tokens);
	}
	else if (iequals(type, "vacuum"))         proto = resolve_vacuum(tokens);

	return proto;
}

apbase AsciiProtocol::resolve_register(listring &tokens) {
	apreg proto = makeap_reg();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
		else if (iequals(keyword, "result"))   proto->result = atoi(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_findhome(listring &tokens) {
	apfindhome proto = makeap_findhome();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_homesync(listring &tokens) {
	aphomesync proto = makeap_homesync();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "ra"))       proto->ra = atof(value.c_str());
		else if (iequals(keyword, "dec"))      proto->dc = atof(value.c_str());
		else if (iequals(keyword, "epoch"))    proto->epoch = atof(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_slewto(listring &tokens) {
	apslewto proto = makeap_slewto();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "ra"))       proto->ra = atof(value.c_str());
		else if (iequals(keyword, "dec"))      proto->dc = atof(value.c_str());
		else if (iequals(keyword, "epoch"))    proto->epoch = atof(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_park(listring &tokens) {
	appark proto = makeap_park();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_guide(listring &tokens) {
	apguide proto = makeap_guide();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "ra"))       proto->ra = atof(value.c_str());
		else if (iequals(keyword, "dec"))      proto->dc = atof(value.c_str());
		else if (iequals(keyword, "objra"))    proto->objra = atof(value.c_str());
		else if (iequals(keyword, "objdec"))   proto->objdc = atof(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_abortslew(listring &tokens) {
	apabortslew proto = makeap_abortslew();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_telescopeinfo(listring &tokens) {
	apnftele proto = makeap_telescopeinfo();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if (iequals(keyword, "state"))         proto->state = atoi(value.c_str());
		else if (iequals(keyword, "ec"))       proto->ec = atoi(value.c_str());
		else if (iequals(keyword, "ra"))       proto->ra = atof(value.c_str());
		else if (iequals(keyword, "dec"))      proto->dc = atof(value.c_str());
		else if (iequals(keyword, "objra"))    proto->objra = atof(value.c_str());
		else if (iequals(keyword, "objdec"))   proto->objdc = atof(value.c_str());
		else if (iequals(keyword, "time"))     proto->utc = value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_fwhm(listring &tokens) {
	apfwhm proto = makeap_fwhm();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
		else if (iequals(keyword, "value"))    proto->value = atof(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_focus(listring &tokens) {
	apfocus proto = makeap_focus();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
		else if (iequals(keyword, "value"))    proto->value = atoi(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_focusync(listring &tokens) {
	apfocusync proto = makeap_focusync();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_mcover(listring &tokens) {
	apmcover proto = makeap_mcover();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
		else if (iequals(keyword, "value"))    proto->value = atoi(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_takeimg(listring &tokens) {
	aptakeimg proto = makeap_takeimg();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
		else if (iequals(keyword, "obj_id") || iequals(keyword, "objname"))
			proto->obj_id  = value;
		else if (iequals(keyword, "imgtype"))  proto->imgtype = value;
		else if (iequals(keyword, "expdur"))   proto->expdur  = atof(value.c_str());
		else if (iequals(keyword, "delay"))    proto->delay   = atof(value.c_str());
		else if (iequals(keyword, "frmcnt"))   proto->frmcnt  = atoi(value.c_str());
		else if (iequals(keyword, "filter"))   proto->filter  = value;

		proto->check_imgtype();
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_abortimg(listring &tokens) {
	apabortimg proto = makeap_abortimg();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_object(listring &tokens) {
	apobject proto = makeap_object();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_expose(listring &tokens) {
	apexpose proto = makeap_expose();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if (iequals(keyword, "command")) proto->command = atoi(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_camerainfo(listring &tokens) {
	apnfcam proto = makeap_camerainfo();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "bitdepth"))  proto->bitdepth  = atoi(value.c_str());
		else if (iequals(keyword, "readport"))  proto->readport  = atoi(value.c_str());
		else if (iequals(keyword, "readrate"))  proto->readrate  = atoi(value.c_str());
		else if (iequals(keyword, "vrate"))     proto->vrate     = atoi(value.c_str());
		else if (iequals(keyword, "gain"))      proto->gain      = atoi(value.c_str());
		else if (iequals(keyword, "connected")) proto->connected = atoi(value.c_str());
		else if (iequals(keyword, "state"))     proto->state     = atoi(value.c_str());
		else if (iequals(keyword, "ec"))        proto->errcode   = atoi(value.c_str());
		else if (iequals(keyword, "coolget"))   proto->coolget   = atof(value.c_str());
		else if (iequals(keyword, "coolset"))   proto->coolset   = atof(value.c_str());
		else if (iequals(keyword, "filter"))    proto->filter    = value;
		else if (iequals(keyword, "frmno"))     proto->frmno     = atoi(value.c_str());
		else if (iequals(keyword, "freedisk"))  proto->freedisk  = atof(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_cooler(listring& tokens) {
	apcooler proto = makeap_cooler();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
		else if (iequals(keyword, "voltage"))  proto->voltage = atof(value.c_str());
		else if (iequals(keyword, "current"))  proto->current = atof(value.c_str());
		else if (iequals(keyword, "hotend"))   proto->hotend  = atof(value.c_str());
		else if (iequals(keyword, "coolget"))  proto->coolget = atof(value.c_str());
		else if (iequals(keyword, "coolset"))  proto->coolset = atof(value.c_str());
		else if (iequals(keyword, "time"))     proto->utc = value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_vacuum(listring& tokens) {
	apvacuum proto = makeap_vacuum();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid = value;
		else if (iequals(keyword, "unit_id"))  proto->uid = value;
		else if (iequals(keyword, "cam_id"))   proto->cid = value;
		else if (iequals(keyword, "voltage"))  proto->voltage = atof(value.c_str());
		else if (iequals(keyword, "current"))  proto->current = atof(value.c_str());
		else if (iequals(keyword, "pressure")) proto->pressure = value;
		else if (iequals(keyword, "time"))     proto->utc = value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_fileinfo(listring& tokens) {
	apfileinfo proto = makeap_fileinfo();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if      (iequals(keyword, "group_id")) proto->gid      = value;
		else if (iequals(keyword, "unit_id"))  proto->uid      = value;
		else if (iequals(keyword, "cam_id"))   proto->cid      = value;
		else if (iequals(keyword, "grid_id"))  proto->grid     = value;
		else if (iequals(keyword, "field_id")) proto->field    = value;
		else if (iequals(keyword, "tmobs"))    proto->tmobs    = value;
		else if (iequals(keyword, "subpath"))  proto->subpath  = value;
		else if (iequals(keyword, "filename")) proto->filename = value;
		else if (iequals(keyword, "filesize")) proto->filesize = atoi(value.c_str());
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_filestat(listring& tokens) {
	apfilestat proto = makeap_filestat();
	listring::iterator itend = tokens.end();
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != itend; ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别关键字
		if (iequals(keyword, "status")) proto->status = atoi(value.c_str());
	}

	return to_apbase(proto);
}
