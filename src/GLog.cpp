/*
 * @file GLog.cpp 类GLog的定义文件
 * @version      2.0
 * @date    2016年10月28日
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include "globaldef.h"
#include "GLog.h"

using namespace boost::posix_time;

GLog::GLog(FILE *out) {
	day_ = -1;
	fd_  = out;
}

GLog::~GLog() {
	if (fd_ && fd_ != stdout && fd_ != stderr) fclose(fd_);
}

bool GLog::valid_file(ptime &t) {
	if (fd_ == stdout || fd_ == stderr) return true;
	ptime::date_type date = t.date();
	if (day_ != date.day()) {// 日期变更
		day_ = date.day();
		if (fd_) {// 关闭已打开的日志文件
			fprintf(fd_, "%s continue\n", string(69, '>').c_str());
			fclose(fd_);
			fd_ = NULL;
		}
	}

	if (fd_ == NULL) {
		if (access(gLogDir, F_OK)) mkdir(gLogDir, 0755);	// 创建目录
		if (!access(gLogDir, W_OK | X_OK)) {
			boost::filesystem::path path = gLogDir;
			boost::format fmt("%s%s.log");
			fmt % gLogPrefix % to_iso_string(date).c_str();
			path /= fmt.str();
			fd_ = fopen(path.c_str(), "a+t");
			fprintf(fd_, "%s\n", string(79, '-').c_str());
		}
	}

	return (fd_ != NULL);
}

void GLog::Write(const char* format, ...) {
	if (format == NULL) return;

	mutex_lock lock(mtx_);
	ptime t = microsec_clock::local_time();
	va_list vl;

	if (valid_file(t)) {
		fprintf(fd_, "%s >> ", to_simple_string(t.time_of_day()).c_str());
		va_start(vl, format);
		vfprintf(fd_, format, vl);
		va_end(vl);
		fprintf(fd_, "\n");
		fflush(fd_);
	}
}

void GLog::Write(const LOG_TYPE type, const char* where, const char* format, ...) {
	if (format == NULL) return;

	mutex_lock lock(mtx_);
	ptime t = microsec_clock::local_time();
	va_list vl;

	if (valid_file(t)) {
		fprintf(fd_, "%s >> ", to_simple_string(t.time_of_day()).c_str());
		if      (type == LOG_WARN)  fprintf(fd_, "WARN: ");
		else if (type == LOG_FAULT) fprintf(fd_, "ERROR: ");
		if (where) fprintf(fd_, "%s, ", where);
		va_start(vl, format);
		vfprintf(fd_, format, vl);
		va_end(vl);
		fprintf(fd_, "\n");
		fflush(fd_);
	}
}
