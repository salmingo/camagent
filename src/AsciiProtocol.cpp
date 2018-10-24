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
// 检查赤经/赤纬有效性
bool valid_ra(double ra) {
	return 0.0 <= ra && ra < 360.0;
}

bool valid_dec(double dec) {
	return -90.0 <= dec && dec <= 90.0;
}


/*!
 * @brief 检查图像类型
 */
bool check_imgtype(string imgtype, int &itype, string &sabbr) {
	bool isValid(true);

	if (boost::iequals(imgtype, "bias")) {
		itype = IMGTYPE_BIAS;
		sabbr = "bias";
	}
	else if (boost::iequals(imgtype, "dark")) {
		itype = IMGTYPE_DARK;
		sabbr = "dark";
	}
	else if (boost::iequals(imgtype, "flat")) {
		itype = IMGTYPE_FLAT;
		sabbr = "flat";
	}
	else if (boost::iequals(imgtype, "object")) {
		itype = IMGTYPE_OBJECT;
		sabbr = "objt";
	}
	else if (boost::iequals(imgtype, "focus")) {
		itype = IMGTYPE_FOCUS;
		sabbr = "focs";
	}
	else {
		itype = IMGTYPE_ERROR;
		isValid = false;
	}
	return isValid;
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

void AsciiProtocol::compact_base(apbase base, string &output) {
	output = base->type + " ";
	if (!base->utc.empty()) join_kv(output, "utc",      base->utc);
	if (!base->gid.empty()) join_kv(output, "group_id", base->gid);
	if (!base->uid.empty()) join_kv(output, "unit_id",  base->uid);
	if (!base->cid.empty()) join_kv(output, "cam_id",   base->cid);
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

void AsciiProtocol::resolve_kv_array(listring &tokens, likv &kvs, ascii_proto_base &basis) {
	string keyword, value;

	for (listring::iterator it = tokens.begin(); it != tokens.end(); ++it) {// 遍历键值对
		if (!resolve_kv(*it, keyword, value)) continue;
		// 识别通用项
		if      (iequals(keyword, "utc"))      basis.utc = value;
		else if (iequals(keyword, "group_id")) basis.gid = value;
		else if (iequals(keyword, "unit_id"))  basis.uid = value;
		else if (iequals(keyword, "cam_id"))   basis.cid = value;
		else {// 存储非通用项
			pair_key_val kv;
			kv.keyword = keyword;
			kv.value   = value;
			kvs.push_back(kv);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
/*---------------- 封装通信协议 ----------------*/
/*
 * @note
 * register协议功能:
 * 1/ 设备在服务器上注册其ID编号
 * 2/ 服务器通知注册结果. ostype在相机设备上生效, 用于创建不同的目录结构和文件名及FITS头
 */
const char *AsciiProtocol::CompactRegister(apreg proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	if (proto->result != INT_MIN) join_kv(output, "result",   proto->result);
	if (proto->ostype != INT_MIN) join_kv(output, "ostype",   proto->ostype);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactUnregister(apunreg proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactStart(apstart proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactStop(apstop proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactEnable(apenable proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactDisable(apdisable proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactObss(apobss proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output, camid;
	int m(proto->camera.size());

	compact_base(to_apbase(proto), output);
	join_kv(output, "state",    proto->state);
	if (proto->op_sn >= 0) {
		join_kv(output, "op_sn",    proto->op_sn);
		join_kv(output, "op_time",  proto->op_time);
	}
	join_kv(output, "mount",    proto->mount);
	for (int i = 0; i < m; ++i) {
		camid = "cam#" + proto->camera[i].cid;
		join_kv(output, camid, proto->camera[i].state);
	}

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFindHome(apfindhome proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactHomeSync(aphomesync proto, int &n) {
	if (!proto.use_count()
			|| !(valid_ra(proto->ra) && valid_dec(proto->dc)))
		return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dc);
	join_kv(output, "epoch",    proto->epoch);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactSlewto(apslewto proto, int &n) {
	if (!proto.use_count()
			|| !(valid_ra(proto->ra) && valid_dec(proto->dc)))
		return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dc);
	join_kv(output, "epoch",    proto->epoch);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactPark(appark proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactGuide(apguide proto, int &n) {
	if (!proto.use_count()
			|| !(valid_ra(proto->ra) && valid_dec(proto->dc)))
		return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	// 真实指向位置或位置偏差
	join_kv(output, "ra",       proto->ra);
	join_kv(output, "dec",      proto->dc);
	// 当(ra,dec)和目标位置同时有效时, (ra,dec)指代真实位置而不是位置偏差
	if (valid_ra(proto->objra) && valid_dec(proto->objdc)) {
		join_kv(output, "objra",    proto->objra);
		join_kv(output, "objdec",   proto->objdc);
	}

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactAbortSlew(apabortslew proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactTelescope(aptele proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
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

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "value", proto->value);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFocus(apfocus proto, int &n) {
	if (!proto.use_count() || proto->value == INT_MIN) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "value", proto->value);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFocusSync(apfocusync proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactMirrorCover(apmcover proto, int &n) {
	if (!proto.use_count() || proto->value == INT_MIN) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "value", proto->value);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactTakeImage(aptakeimg proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "objname",  proto->objname);
	join_kv(output, "imgtype",  proto->imgtype);
	join_kv(output, "filter",   proto->filter);
	join_kv(output, "expdur",   proto->expdur);
	join_kv(output, "delay",    proto->delay);
	join_kv(output, "frmcnt",   proto->frmcnt);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactAbortImage(apabortimg proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactAppendGWAC(apappgwac proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "plan_sn",     proto->plan_sn);
	join_kv(output, "plan_time",   proto->plan_time);
	join_kv(output, "plan_type",   proto->plan_type);
	join_kv(output, "obstype",     proto->obstype);
	join_kv(output, "grid_id",     proto->grid_id);
	join_kv(output, "field_id",    proto->field_id);
	join_kv(output, "obj_id",      proto->obj_id);
	join_kv(output, "ra",          proto->ra);
	join_kv(output, "dec",         proto->dec);
	join_kv(output, "epoch",       proto->epoch);
	join_kv(output, "objra",       proto->objra);
	join_kv(output, "objdec",      proto->objdec);
	join_kv(output, "objepoch",    proto->objepoch);
	join_kv(output, "objerror",    proto->objerror);
	join_kv(output, "imgtype",     proto->imgtype);
	join_kv(output, "expdur",      proto->expdur);
	join_kv(output, "delay",       proto->delay);
	join_kv(output, "frmcnt",      proto->frmcnt);
	join_kv(output, "priority",    proto->priority);
	join_kv(output, "begin_time",  proto->begin_time);
	join_kv(output, "end_time",    proto->end_time);
	join_kv(output, "pair_id",     proto->pair_id);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactAppendPlan(apappplan proto, int &n) {
	if (!proto.use_count()
			|| !(proto->expdur.size() && proto->frmcnt.size()))
		return NULL;

	int size, i;
	string output, tmp;
	compact_base(to_apbase(proto), output);

	join_kv(output, "plan_sn",     proto->plan_sn);
	join_kv(output, "plan_time",   proto->plan_time);
	join_kv(output, "observer",    proto->observer);
	join_kv(output, "obstype",     proto->obstype);
	join_kv(output, "objname",     proto->objname);
	join_kv(output, "runname",     proto->runname);
	join_kv(output, "ra",          proto->ra);
	join_kv(output, "dec",         proto->dec);
	join_kv(output, "epoch",       proto->epoch);
	join_kv(output, "objerror",    proto->objerror);
	join_kv(output, "imgtype",     proto->imgtype);

	if ((size = proto->filter.size())) {
		tmp = proto->filter[0];
		for (i = 1; i < size; ++i) {
			tmp += "|" + proto->filter[i];
		}
		join_kv(output, "filter", tmp);
	}

	tmp = proto->expdur[0];
	for (i = 1, size = proto->expdur.size(); i < size; ++i) {
		tmp += "|" + std::to_string(proto->expdur[i]);
	}
	join_kv(output, "expdur", tmp);

	if ((size = proto->delay.size())) {
		tmp = proto->delay[0];
		for (i = 1, size = proto->delay.size(); i < size; ++i) {
			tmp += "|" + std::to_string(proto->delay[i]);
		}
		join_kv(output, "delay", tmp);
	}

	tmp = proto->frmcnt[0];
	for (i = 1, size = proto->frmcnt.size(); i < size; ++i) {
		tmp += "|" + std::to_string(proto->frmcnt[i]);
	}
	join_kv(output, "frmcnt", tmp);

	join_kv(output, "priority",    proto->priority);
	join_kv(output, "begin_time",  proto->begin_time);
	join_kv(output, "end_time",    proto->end_time);

	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactRemovePlan(aprmvplan proto, int &n) {
	if (!proto.use_count() || proto->plan_sn < 0) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "plan_sn", proto->plan_sn);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactCheckPlan(apchkplan proto, int &n) {
	if (!proto.use_count() || proto->plan_sn < 0) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "plan_sn", proto->plan_sn);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactPlan(applan proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);
	join_kv(output, "plan_sn", proto->plan_sn);
	join_kv(output, "state",   proto->state);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactObject(apobject proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	if (proto->plan_sn != INT_MIN) join_kv(output, "plan_sn",    proto->plan_sn);
	if (!proto->plan_time.empty()) join_kv(output, "plan_time",  proto->plan_time);
	if (!proto->plan_type.empty()) join_kv(output, "plan_type",  proto->plan_type);
	if (!proto->observer.empty())  join_kv(output, "observer", proto->observer);
	if (!proto->obstype.empty())   join_kv(output, "obstype",  proto->obstype);
	if (!proto->grid_id.empty())   join_kv(output, "grid_id",  proto->grid_id);
	if (!proto->field_id.empty())  join_kv(output, "field_id", proto->field_id);
	if (valid_ra(proto->ra) && valid_dec(proto->dc)) {
		join_kv(output, "ra",       proto->ra);
		join_kv(output, "dec",      proto->dc);
		join_kv(output, "epoch",    proto->epoch);
	}
	if (valid_ra(proto->objra) && valid_dec(proto->objdc)) {
		join_kv(output, "objra",    proto->objra);
		join_kv(output, "objdec",   proto->objdc);
		join_kv(output, "objepoch", proto->objepoch);
	}
	if (!proto->objerror.empty())  join_kv(output, "objerror", proto->objerror);
	join_kv(output, "priority", proto->priority);
	join_kv(output, "begin_time",  proto->begin_time);
	join_kv(output, "end_time",    proto->end_time);
	join_kv(output, "pair_id",  proto->pair_id);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactExpose(apexpose proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "command", proto->command);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactCamera(apcam proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "state",     proto->state);
	join_kv(output, "errcode",   proto->errcode);
	join_kv(output, "imgtype",   proto->imgtype);
	join_kv(output, "objname",   proto->objname);
	join_kv(output, "frmno",     proto->frmno);
	join_kv(output, "filename",  proto->filename);
	join_kv(output, "mcstate",   proto->mcstate);
	join_kv(output, "focus",     proto->focus);
	join_kv(output, "coolget",   proto->coolget);
	join_kv(output, "filter",    proto->filter);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactCooler(apcooler proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "voltage",  proto->voltage);
	join_kv(output, "current",  proto->current);
	join_kv(output, "hotend",   proto->hotend);
	join_kv(output, "coolget",  proto->coolget);
	join_kv(output, "coolset",  proto->coolset);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactVacuum(apvacuum proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

	join_kv(output, "voltage",  proto->voltage);
	join_kv(output, "current",  proto->current);
	join_kv(output, "pressure", proto->pressure);
	return output_compacted(output, n);
}

const char *AsciiProtocol::CompactFileInfo(apfileinfo proto, int &n) {
	if (!proto.use_count()) return NULL;

	string output;
	compact_base(to_apbase(proto), output);

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

	string output;
	compact_base(to_apbase(proto), output);

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
	likv kvs;
	ascii_proto_base basis;
	bool is_valid(true);

	// 提取协议类型
	for (ptr = rcvd; *ptr && *ptr != ' '; ++ptr) type += *ptr;
	while (*ptr && *ptr == ' ') ++ptr;
	// 分解键值对
	if (*ptr) algorithm::split(tokens, ptr, is_any_of(seps), token_compress_on);
	resolve_kv_array(tokens, kvs, basis);
	// 按照协议类型解析键值对
	if ((ch = type[0]) == 'a') {
		if      (iequals(type, "abort_slew"))  proto = resolve_abortslew(kvs);
		else if (iequals(type, "abort_image")) proto = resolve_abortimg(kvs);
		else if (iequals(type, "append_gwac")) proto = resolve_append_gwac(kvs);
		else if (iequals(type, "append_plan")) proto = resolve_append_plan(kvs);
	}
	else if (ch == 'c') {
		if      (iequals(type, "cooler"))      proto = resolve_cooler(kvs);
		else if (iequals(type, "camera"))      proto = resolve_camera(kvs);
		else if (iequals(type, "check_plan"))  proto = resolve_check_plan(kvs);
	}
	else if (iequals(type, "disable"))         proto = resolve_disable(kvs);
	else if (ch == 'e') {
		if      (iequals(type, "expose"))      proto = resolve_expose(kvs);
		else if (iequals(type, "enable"))      proto = resolve_enable(kvs);
	}
	else if (ch == 'f') {
		if      (iequals(type, "fileinfo"))    proto = resolve_fileinfo(kvs);
		else if (iequals(type, "filestat"))    proto = resolve_filestat(kvs);
		else if (iequals(type, "find_home"))   proto = resolve_findhome(kvs);
		else if (iequals(type, "fwhm"))        proto = resolve_fwhm(kvs);
		else if (iequals(type, "focus"))       proto = resolve_focus(kvs);
		else if (iequals(type, "focus_sync"))  proto = resolve_focusync(kvs);
	}
	else if (iequals(type, "guide"))           proto = resolve_guide(kvs);
	else if (iequals(type, "home_sync"))       proto = resolve_homesync(kvs);
	else if (iequals(type, "mcover"))          proto = resolve_mcover(kvs);
	else if (ch == 'o') {
		if      (iequals(type, "object"))      proto = resolve_object(kvs);
		else if (iequals(type, "obss"))        proto = resolve_obss(kvs);
	}
	else if (ch == 'p') {
		if      (iequals(type, "park"))        proto = resolve_park(kvs);
		else if (iequals(type, "plan"))        proto = resolve_plan(kvs);
	}
	else if (ch == 'r') {
		if      (iequals(type, "register"))    proto = resolve_register(kvs);
		else if (iequals(type, "remove_plan")) proto = resolve_remove_plan(kvs);
	}
	else if (ch == 's') {
		if      (iequals(type, "slewto"))      proto = resolve_slewto(kvs);
		else if (iequals(type, "start"))       proto = resolve_start(kvs);
		else if (iequals(type, "stop"))        proto = resolve_stop(kvs);
	}
	else if (ch == 't'){
		if      (iequals(type, "telescope"))   proto = resolve_telescope(kvs);
		else if (iequals(type, "take_image"))  proto = resolve_takeimg(kvs);
	}
	else if (iequals(type, "unregister"))      proto = resolve_unregister(kvs);
	else if (iequals(type, "vacuum"))          proto = resolve_vacuum(kvs);
	else is_valid = false;

	if (is_valid) {
		proto->type = type;
		proto->utc  = basis.utc;
		proto->gid  = basis.gid;
		proto->uid  = basis.uid;
		proto->cid  = basis.cid;
	}

	return proto;
}

apbase AsciiProtocol::resolve_register(likv &kvs) {
	apreg proto = boost::make_shared<ascii_proto_reg>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "ostype"))  proto->ostype = std::stoi((*it).value);
		else if (iequals(keyword, "result"))  proto->result = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_unregister(likv &kvs) {
	apunreg proto = boost::make_shared<ascii_proto_unreg>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_start(likv &kvs) {
	apstart proto = boost::make_shared<ascii_proto_start>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_stop(likv &kvs) {
	apstop proto = boost::make_shared<ascii_proto_stop>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_enable(likv &kvs) {
	apenable proto = boost::make_shared<ascii_proto_enable>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_disable(likv &kvs) {
	apdisable proto = boost::make_shared<ascii_proto_disable>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_obss(likv &kvs) {
	apobss proto = boost::make_shared<ascii_proto_obss>();
	string keyword, precid = "cam#";
	int nprecid = precid.size();

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "state"))   proto->state   = std::stoi((*it).value);
		else if (iequals(keyword, "op_sn"))   proto->op_sn   = std::stoi((*it).value);
		else if (iequals(keyword, "op_time")) proto->op_time = (*it).value;
		else if (iequals(keyword, "mount"))   proto->mount   = std::stoi((*it).value);
		else if (keyword.find(precid) == 0) {// 相机工作状态. 关键字 cam#xxx
			camera_state cs;
			cs.cid   = keyword.substr(nprecid);
			cs.state = std::stoi((*it).value);

			proto->camera.push_back(cs);
		}
	}
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_findhome(likv &kvs) {
	apfindhome proto = boost::make_shared<ascii_proto_find_home>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_homesync(likv &kvs) {
	aphomesync proto = boost::make_shared<ascii_proto_home_sync>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "ra"))     proto->ra    = std::stod((*it).value);
		else if (iequals(keyword, "dec"))    proto->dc    = std::stod((*it).value);
		else if (iequals(keyword, "epoch"))  proto->epoch = std::stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_slewto(likv &kvs) {
	apslewto proto = boost::make_shared<ascii_proto_slewto>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "ra"))     proto->ra    = std::stod((*it).value);
		else if (iequals(keyword, "dec"))    proto->dc    = std::stod((*it).value);
		else if (iequals(keyword, "epoch"))  proto->epoch = std::stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_park(likv &kvs) {
	appark proto = boost::make_shared<ascii_proto_park>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_guide(likv &kvs) {
	apguide proto = boost::make_shared<ascii_proto_guide>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "ra"))       proto->ra    = std::stod((*it).value);
		else if (iequals(keyword, "dec"))      proto->dc    = std::stod((*it).value);
		else if (iequals(keyword, "objra"))    proto->objra = std::stod((*it).value);
		else if (iequals(keyword, "objdec"))   proto->objdc = std::stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_abortslew(likv &kvs) {
	apabortslew proto = boost::make_shared<ascii_proto_abort_slew>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_telescope(likv &kvs) {
	aptele proto = boost::make_shared<ascii_proto_telescope>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "state"))    proto->state = std::stoi((*it).value);
		else if (iequals(keyword, "ec"))       proto->ec    = std::stoi((*it).value);
		else if (iequals(keyword, "ra"))       proto->ra    = std::stod((*it).value);
		else if (iequals(keyword, "dec"))      proto->dc    = std::stod((*it).value);
		else if (iequals(keyword, "objra"))    proto->objra = std::stod((*it).value);
		else if (iequals(keyword, "objdec"))   proto->objdc = std::stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_fwhm(likv &kvs) {
	apfwhm proto = boost::make_shared<ascii_proto_fwhm>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "value"))  proto->value = std::stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_focus(likv &kvs) {
	apfocus proto = boost::make_shared<ascii_proto_focus>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "value"))  proto->value = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_focusync(likv &kvs) {
	apfocusync proto = boost::make_shared<ascii_proto_focus_sync>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_mcover(likv &kvs) {
	apmcover proto = boost::make_shared<ascii_proto_mcover>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "value"))  proto->value = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_takeimg(likv &kvs) {
	aptakeimg proto = boost::make_shared<ascii_proto_takeimg>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "obj_id") || iequals(keyword, "objname"))
			proto->objname = (*it).value;
		else if (iequals(keyword, "imgtype"))  proto->imgtype = (*it).value;
		else if (iequals(keyword, "filter"))   proto->filter  = (*it).value;
		else if (iequals(keyword, "expdur"))   proto->expdur  = std::stod((*it).value);
		else if (iequals(keyword, "frmcnt"))   proto->frmcnt  = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_abortimg(likv &kvs) {
	apabortimg proto = boost::make_shared<ascii_proto_abortimg>();
	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_object(likv &kvs) {
	apobject proto = boost::make_shared<ascii_proto_object>();
	string keyword;
	char ch;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if ((ch = keyword[0]) == 'o') {
			if      (iequals(keyword, "observer"))  proto->observer  = (*it).value;
			else if (iequals(keyword, "obstype"))   proto->obstype   = (*it).value;
			else if (iequals(keyword, "objra"))     proto->objra     = std::stod((*it).value);
			else if (iequals(keyword, "objdec"))    proto->objdc     = std::stod((*it).value);
			else if (iequals(keyword, "objepoch"))  proto->objepoch  = std::stod((*it).value);
			else if (iequals(keyword, "objerror"))  proto->objerror  = (*it).value;
		}
		else if (ch == 'p') {
			if      (iequals(keyword, "plan_sn"))   proto->plan_sn   = std::stoi((*it).value);
			else if (iequals(keyword, "plan_time")) proto->plan_time = (*it).value;
			else if (iequals(keyword, "plan_type")) proto->plan_type = (*it).value;
			else if (iequals(keyword, "priority"))  proto->priority  = std::stoi((*it).value);
			else if (iequals(keyword, "pair_id"))   proto->pair_id   = std::stoi((*it).value);
		}
		else if (ch == 't') {
			if      (iequals(keyword, "begin_time"))  proto->begin_time  = (*it).value;
			else if (iequals(keyword, "end_time"))    proto->end_time    = (*it).value;
		}
		else if (iequals(keyword, "ra"))        proto->ra       = std::stod((*it).value);
		else if (iequals(keyword, "dec"))       proto->dc       = std::stod((*it).value);
		else if (iequals(keyword, "epoch"))     proto->epoch    = std::stod((*it).value);
		else if (iequals(keyword, "field_id"))  proto->field_id = (*it).value;
		else if (iequals(keyword, "grid_id"))   proto->grid_id  = (*it).value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_expose(likv &kvs) {
	apexpose proto = boost::make_shared<ascii_proto_expose>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "command")) proto->command = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_camera(likv &kvs) {
	apcam proto = boost::make_shared<ascii_proto_camera>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "state"))     proto->state     = std::stoi((*it).value);
		else if (iequals(keyword, "ec"))        proto->errcode   = std::stoi((*it).value);
		else if (iequals(keyword, "imgtype"))   proto->imgtype   = (*it).value;
		else if (iequals(keyword, "objname"))   proto->objname   = (*it).value;
		else if (iequals(keyword, "frmno"))     proto->frmno     = std::stoi((*it).value);
		else if (iequals(keyword, "filename"))  proto->filename  = (*it).value;
		else if (iequals(keyword, "mcstate"))   proto->mcstate   = std::stoi((*it).value);
		else if (iequals(keyword, "focus"))     proto->focus     = std::stoi((*it).value);
		else if (iequals(keyword, "coolget"))   proto->coolget   = std::stod((*it).value);
		else if (iequals(keyword, "filter"))    proto->filter    = (*it).value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_cooler(likv &kvs) {
	apcooler proto = boost::make_shared<ascii_proto_cooler>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "voltage"))  proto->voltage = std::stod((*it).value);
		else if (iequals(keyword, "current"))  proto->current = std::stod((*it).value);
		else if (iequals(keyword, "hotend"))   proto->hotend  = std::stod((*it).value);
		else if (iequals(keyword, "coolget"))  proto->coolget = std::stod((*it).value);
		else if (iequals(keyword, "coolset"))  proto->coolset = std::stod((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_vacuum(likv &kvs) {
	apvacuum proto = boost::make_shared<ascii_proto_vacuum>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "voltage"))  proto->voltage  = std::stod((*it).value);
		else if (iequals(keyword, "current"))  proto->current  = std::stod((*it).value);
		else if (iequals(keyword, "pressure")) proto->pressure = (*it).value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_fileinfo(likv &kvs) {
	apfileinfo proto = boost::make_shared<ascii_proto_fileinfo>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "grid_id"))  proto->grid     = (*it).value;
		else if (iequals(keyword, "field_id")) proto->field    = (*it).value;
		else if (iequals(keyword, "tmobs"))    proto->tmobs    = (*it).value;
		else if (iequals(keyword, "subpath"))  proto->subpath  = (*it).value;
		else if (iequals(keyword, "filename")) proto->filename = (*it).value;
		else if (iequals(keyword, "filesize")) proto->filesize = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_filestat(likv &kvs) {
	apfilestat proto = boost::make_shared<ascii_proto_filestat>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "status")) proto->status = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_plan(likv &kvs) {
	applan proto = boost::make_shared<ascii_proto_plan>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if      (iequals(keyword, "plan_sn")) proto->plan_sn = std::stoi((*it).value);
		else if (iequals(keyword, "state"))   proto->state   = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_check_plan(likv &kvs) {
	apchkplan proto = boost::make_shared<ascii_proto_check_plan>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "plan_sn")) proto->plan_sn = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_remove_plan(likv &kvs) {
	aprmvplan proto = boost::make_shared<ascii_proto_remove_plan>();
	string keyword;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		// 识别关键字
		if (iequals(keyword, "plan_sn")) proto->plan_sn = std::stoi((*it).value);
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_append_gwac(likv &kvs) {
	apappgwac proto = boost::make_shared<ascii_proto_append_gwac>();
	string keyword;
	char ch;

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		ch = keyword[0];
		// 识别关键字
		if (ch == 'p') {
			if      (iequals(keyword, "plan_sn"))   proto->plan_sn   = std::stoi((*it).value);
			else if (iequals(keyword, "plan_time")) proto->plan_time = (*it).value;
			else if (iequals(keyword, "plan_type")) proto->plan_type = (*it).value;
			else if (iequals(keyword, "priority"))  proto->priority  = std::stoi((*it).value);
			else if (iequals(keyword, "pair_id"))   proto->pair_id   = std::stoi((*it).value);
		}
		else if (ch == 'o') {
			if      (iequals(keyword, "obstype"))   proto->obstype   = (*it).value;
			else if (iequals(keyword, "obj_id"))    proto->obj_id    = (*it).value;
			else if (iequals(keyword, "objra"))     proto->objra     = std::stod((*it).value);
			else if (iequals(keyword, "objdec"))    proto->objdec    = std::stod((*it).value);
			else if (iequals(keyword, "objepoch"))  proto->objepoch  = std::stod((*it).value);
			else if (iequals(keyword, "objerror"))  proto->objerror  = (*it).value;
		}
		else if (ch == 'e') {
			if      (iequals(keyword, "epoch"))    proto->epoch    = std::stod((*it).value);
			else if (iequals(keyword, "expdur"))   proto->expdur   = std::stod((*it).value);
			else if (iequals(keyword, "end_time")) proto->end_time = (*it).value;
		}
		else if (ch == 'd') {
			if      (iequals(keyword, "dec"))   proto->dec   = std::stod((*it).value);
			else if (iequals(keyword, "delay")) proto->delay = std::stod((*it).value);
		}
		else if (ch == 'f') {
			if      (iequals(keyword, "field_id")) proto->field_id = (*it).value;
			else if (iequals(keyword, "frmcnt"))   proto->frmcnt   = std::stoi((*it).value);
		}
		else if (iequals(keyword, "ra"))         proto->ra         = std::stod((*it).value);
		else if (iequals(keyword, "grid_id"))    proto->grid_id    = (*it).value;
		else if (iequals(keyword, "imgtype"))    proto->imgtype    = (*it).value;
		else if (iequals(keyword, "begin_time")) proto->begin_time = (*it).value;
	}

	return to_apbase(proto);
}

apbase AsciiProtocol::resolve_append_plan(likv &kvs) {
	apappplan proto = boost::make_shared<ascii_proto_append_plan>();
	string keyword;
	char ch;
	const char seps[] = "| ";

	for (likv::iterator it = kvs.begin(); it != kvs.end(); ++it) {// 遍历键值对
		keyword = (*it).keyword;
		ch = keyword[0];
		// 识别关键字
		if (ch == 'p') {
			if      (iequals(keyword, "plan_sn"))   proto->plan_sn   = std::stoi((*it).value);
			else if (iequals(keyword, "plan_time")) proto->plan_time = (*it).value;
			else if (iequals(keyword, "priority"))  proto->priority  = std::stoi((*it).value);
		}
		else if (ch == 'o') {
			if      (iequals(keyword, "observer"))   proto->observer  = (*it).value;
			else if (iequals(keyword, "obstype"))    proto->obstype   = (*it).value;
			else if (iequals(keyword, "objname"))    proto->objname   = (*it).value;
			else if (iequals(keyword, "objerror"))   proto->objerror  = (*it).value;
		}
		else if (ch == 'e') {
			if      (iequals(keyword, "epoch"))    proto->epoch    = std::stod((*it).value);
			else if (iequals(keyword, "end_time")) proto->end_time = (*it).value;
			else if (iequals(keyword, "expdur")) {// 曝光时间
				listring tokens;
				algorithm::split(tokens, (*it).value, is_any_of(seps), token_compress_on);
				for (listring::iterator it1 = tokens.begin(); it1 != tokens.end(); ++it1) {
					double t = std::stod(*it1);
					proto->expdur.push_back(t);
				}
			}
		}
		else if (ch == 'd') {
			if      (iequals(keyword, "dec"))   proto->dec = std::stod((*it).value);
			else if (iequals(keyword, "delay")) {// 延时时间
				listring tokens;
				algorithm::split(tokens, (*it).value, is_any_of(seps), token_compress_on);
				for (listring::iterator it1 = tokens.begin(); it1 != tokens.end(); ++it1) {
					double t = std::stod(*it1);
					proto->delay.push_back(t);
				}
			}
		}
		else if (ch == 'f') {
			if      (iequals(keyword, "filter")) {// 滤光片
				listring tokens;
				algorithm::split(tokens, (*it).value, is_any_of(seps), token_compress_on);
				for (listring::iterator it1 = tokens.begin(); it1 != tokens.end(); ++it1) {
					string t = *it1;
					proto->filter.push_back(t);
				}
			}
			else if (iequals(keyword, "frmcnt")) {// 曝光总帧数
				listring tokens;
				algorithm::split(tokens, (*it).value, is_any_of(seps), token_compress_on);
				for (listring::iterator it1 = tokens.begin(); it1 != tokens.end(); ++it1) {
					int t = std::stoi(*it1);
					proto->frmcnt.push_back(t);
				}
			}
		}
		else if (ch == 'r') {
			if      (iequals(keyword, "ra"))      proto->ra      = std::stod((*it).value);
			else if (iequals(keyword, "runname")) proto->runname = (*it).value;
		}
		else if (iequals(keyword, "imgtype"))    proto->imgtype    = (*it).value;
		else if (iequals(keyword, "begin_time")) proto->begin_time = (*it).value;
	}

	return to_apbase(proto);
}
