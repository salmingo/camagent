/*
 * @file FilterCtrlFLI.cpp 定义FLI滤光片控制接口
 */
#include <unistd.h>
#include "FilterCtrlFLI.h"

FilterCtrlFLI::FilterCtrlFLI() {
	devID_ = FLI_INVALID_DEVICE;
}

FilterCtrlFLI::~FilterCtrlFLI() {
}

bool FilterCtrlFLI::Connect() {
	if (FilterCtrl::Connect()) return true;
	char fileName[260], devName[100];
	flidomain_t domain;

	if (0 == FLICreateList(FLIDOMAIN_USB | FLIDEVICE_FILTERWHEEL)) {
		if (0 == FLIListFirst(&domain, fileName, 260, devName, 100)) {
			connected_ = 0 == FLIOpen(&devID_, fileName, domain);
		}
		FLIDeleteList();
	}
	if (connected_) devname_ = devName;
	return connected_;
}

bool FilterCtrlFLI::Disconnect() {
	if (FilterCtrl::Disconnect()) return true;
	connected_ = 0 != FLIClose(devID_);
	return !connected_;
}

bool FilterCtrlFLI::FindHome() {
	if (!FilterCtrl::FindHome()) return false;
	FLIHomeDevice(devID_);
	return true;
}

bool FilterCtrlFLI::Goto(int iSlot, int iLayer) {
	if (!FilterCtrl::Goto(iSlot, iLayer)) return false;
	return FLISetFilterPos(devID_, iSlot) == 0;
}

int FilterCtrlFLI::get_slot(int iLayer) {
	bool ret(true);
	long iSlot = FilterCtrl::get_slot(iLayer);
	if (iSlot < 0) ret = FLIGetFilterPos(devID_, &iSlot) == 0;
	return ret ? int(iSlot) : -1;
}
