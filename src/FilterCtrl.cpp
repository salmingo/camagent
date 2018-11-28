/*
 * @file FilterCtrl.cpp 定义通用滤光片控制接口
 */
#include <boost/make_shared.hpp>
#include "FilterCtrl.h"

FilterCtrl::FilterCtrl() {
	connected_ = false;
	nLayer_ = 1;
	nSlot_  = 0;
	default_.reset(new int[nLayer_]);
	position_.reset(new int[nLayer_]);
}

FilterCtrl::~FilterCtrl() {
	Disconnect();
}

bool FilterCtrl::IsConnected() {
	return connected_;
}

bool FilterCtrl::Connect() {
	return connected_;
}

bool FilterCtrl::Disconnect() {
	return !connected_;
}

const string & FilterCtrl::GetDeviceName() {
	return devname_;
}

bool FilterCtrl::SetLayerCount(int n) {
	if (n > 0 && n != nLayer_) {
		nLayer_ = n;
		n *= nSlot_;
		if (n) filters_.reset(new string[n]);
		default_.reset(new int[nLayer_]);
		position_.reset(new int[nLayer_]);
		for (int i = 0; i < nLayer_; ++i) {
			default_[i] = -1;
			position_[i]= -1;
		}
		return true;
	}
	return false;
}

bool FilterCtrl::SetSlotCount(int n) {
	if (n > 0 && n != nSlot_) {
		nSlot_ = n;
		n*= nLayer_;
		if (n) filters_.reset(new string[n]);
		return true;
	}
	return false;
}

bool FilterCtrl::SetFilterName(int iLayer, int iSlot, const string &name) {
	if (iLayer >= 0 && iLayer < nLayer_ && iSlot >= 0 && iSlot < nSlot_ && !name.empty()) {
		filters_[iLayer * nSlot_ + iSlot] = name;
		return true;
	}
	return false;
}

bool FilterCtrl::SetDefaultFilter(const string &name, int iLayer) {
	if (iLayer >= 0 && iLayer < nLayer_ && nSlot_ > 0) {
		int imin(iLayer * nSlot_), imax(imin + nSlot_), i;
		for(i = imin; i < imax && filters_[i] != name; ++i);
		if (i != imax) {
			default_[iLayer] = i % nSlot_;
			return true;
		}
	}
	return false;
}

bool FilterCtrl::SetDefaultFilter(int iSlot, int iLayer) {
	if (iLayer >= 0 && iLayer < nLayer_ && iSlot >= 0 && iSlot < nSlot_) {
		default_[iLayer] = iSlot;
		return true;
	}
	return false;
}

bool FilterCtrl::FindHome() {
	return connected_;
}

bool FilterCtrl::GetFilterName(string &name) {
	name = "";
	if (connected_) {
		int iLayer, iSlot;
		for (iLayer = 0; iLayer < nLayer_; ++iLayer) {
			if (position_[iLayer] == -1 && (iSlot = get_slot(iLayer)) < 0) return false;
			position_[iLayer] = iSlot;
			if (iSlot != default_[iLayer]) {
				if (name.empty()) name = filters_[iLayer * nSlot_ + iSlot];
				else name = name + "+" + filters_[iLayer * nSlot_ + iSlot];
			}
		}
	}
	return true;
}

bool FilterCtrl::Goto(int iSlot, int iLayer) {
	return (connected_ && iSlot >= 0 && iSlot < nSlot_ && iLayer >= 0 && iLayer < nLayer_);
}

bool FilterCtrl::Goto(const string &name) {
	if (!(connected_ && filters_.unique())) return false;
	int iLayer, iSlot, i, n = nLayer_ * nSlot_;
	// 查找滤光片位置
	for (i = 0; i < n && name != filters_[i]; ++i);
	if (i == n) return false;
	iLayer = i / nSlot_;
	iSlot  = i % nSlot_;
	// 定位
	for (i = 0; i < iLayer; ++i) {
		if (position_[i] != default_[i] && !Goto(default_[i], i))
			return false;
	}
	if (!Goto(iSlot, iLayer)) return false;
	for (i = iLayer + 1; i < nLayer_; ++i) {
		if (position_[i] != default_[i] && !Goto(default_[i], i))
			return false;
	}

	position_[iLayer] = iSlot;
	return true;
}

int FilterCtrl::get_slot(int iLayer) {
	return position_[iLayer];
}
