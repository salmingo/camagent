/*
 * @file VersionMonitor.cpp 版本监视定义文件
 */

#include "VersionMonitor.h"

VersionMonitor::VersionMonitor() {

}

VersionMonitor::~VersionMonitor() {
}

bool VersionMonitor::Start() {// 启动服务
	return true;
}

void VersionMonitor::Stop() {// 终止服务

}

// 注册回调函数
void VersionMonitor::RegisterTerminate(const CBSlot& slot) {
	if (!cb_term_.empty()) cb_term_.disconnect_all_slots();
	cb_term_.connect(slot);
}

void VersionMonitor::on_terminate(const long p1, const long p2) {
	cb_term_();	// 调用回调函数
}
