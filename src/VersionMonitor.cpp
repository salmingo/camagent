/*
 * @file VersionMonitor.cpp 版本监视声明文件
 */

#include "VersionMonitor.h"

VersionMonitor::VersionMonitor() {
}

VersionMonitor::~VersionMonitor() {
}

bool VersionMonitor::Start() {// 启动服务
	const CBSlot &slot = boost::bind(&VersionMonitor::on_terminate, this, _1, _2);
	RegisterMessage(MSG_TERMINATE, slot);

	return MessageQueue::Start("msgque_mhssvau", false);
}

// 注册回调函数
void VersionMonitor::RegisterTerminate(const CBSlot0& slot) {
	if (!cb_term_.empty()) cb_term_.disconnect_all_slots();
	cb_term_.connect(slot);
}

void VersionMonitor::on_terminate(long p1, long p2) {
	cb_term_();	// 调用回调函数
}
