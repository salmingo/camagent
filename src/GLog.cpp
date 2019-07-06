/*!
 * @file    GLog.cpp 类GLog的定义文件
 * @version 2.0
 * @date    2016年10月28日
 */

#include <stdarg.h>
#include <string.h>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "GLog.h"

using namespace boost::posix_time;
using namespace boost::filesystem;

GLog::GLog() {
	fd_  = stdout;
}

GLog::~GLog() {
	Stop();
}

bool GLog::valid_file(const string &ymd) {
	if (fd_ == stdout)
		return true;

	if (ymd != ymd_) {// 关闭文件
		if (fd_) {
			fprintf(fd_, "%s continue\n", string(69, '>').c_str());
			fclose(fd_);
			fd_ = NULL;
		}
		ymd_ = ymd;
	}

	if (fd_ == NULL) { // 创建文件
		boost::format fmt("%1%%2%.log");
		path filepath = pathroot_;

		fmt % prefix_ % ymd;
		if (!exists(filepath))
			boost::filesystem::create_directory(filepath);
		filepath /= fmt.str();
		fd_ = fopen(filepath.c_str(), "a+");
		fprintf(fd_, "%s\n", string(79, '-').c_str());
	}

	return (fd_ != NULL);
}

void GLog::thread_write() {
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while (1) {
		cvnew_.wait(lck);
		flush_logs();
	}
}

void GLog::new_log(const char *msg, const char *where,
		const LOG_LEVEL level) {
	// 构建日志
	ptime t = second_clock::local_time();
	ologptr olog;
	olog = boost::make_shared<one_log>(to_iso_string(t.date()), level);
	olog->tmc = to_simple_string(t.time_of_day());
	if (where) olog->pos = where;
	olog->msg = msg;
	// 日志加入列表
	mutex_lock lock(mtxlist_);
	logs_.push_back(olog);
	cvnew_.notify_one();
}

void GLog::flush_logs() {
	ologptr olog;
	while (logs_.size()) {
		mutex_lock lock(mtxlist_);
		olog = logs_.front();
		logs_.pop_front();
		// 检查是否需要创建日志文件
		if (!valid_file(olog->dtc)) // 文件创建失败
			break;
		// 写入文件
		fprintf(fd_, "%s ", olog->tmc.c_str());
		if (olog->level == LOG_WARN)
			fprintf(fd_, "<%s> ", "WARN");
		else if (olog->level == LOG_FAULT)
			fprintf(fd_, "<%s> ", "FAULT");
		if (olog->pos.size())
			fprintf(fd_, "[%s] ", olog->pos.c_str());
		fprintf(fd_, ">> %s\n", olog->msg.c_str());
	}
	fflush(fd_);
}

void GLog::Start(const char *pathroot, const char *prefix) {
	if (pathroot && prefix) {
		pathroot_ = pathroot;
		prefix_ = prefix;
		fd_ = NULL;
	} else if (fd_ != stdout) {
		fd_ = stdout;
	}
	if (!thrdwr_.unique()) { // 检查启动写日志线程
		thrdwr_.reset(
				new boost::thread(boost::bind(&GLog::thread_write, this)));
	}
}

void GLog::Stop() {
	if (thrdwr_.unique()) { // 结束写入线程
		thrdwr_->interrupt();
		thrdwr_->join();
		thrdwr_.reset();
	}
	if (fd_ && logs_.size()) // 处理未写入的日志
		flush_logs();
	if (fd_ && fd_ != stdout) { // 关闭文件
		fclose(fd_);
		fd_ = NULL;
	}
}

void GLog::Write(const char* format, ...) {
	if (format) {// 构建一条日志
		va_list vl;
		char line[400];

		va_start(vl, format);
		vsprintf(line, format, vl);
		va_end(vl);
		new_log(line);
	}
}

void GLog::Write(const LOG_LEVEL level, const char* where, const char* format, ...) {
	if (format == NULL) {
		va_list vl;
		char line[400];

		va_start(vl, format);
		vsprintf(line, format, vl);
		va_end(vl);
		new_log(line, where, level);
	}
}
