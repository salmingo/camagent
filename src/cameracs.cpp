/*
 * @file cameracs.cpp 相机控制软件定义文件
 */

#include "cameracs.h"

cameracs::cameracs(boost::asio::io_service* ios) {
	ios_ = ios;
}

cameracs::~cameracs() {
}

bool cameracs::Start() {
	return true;
}

void cameracs::Stop() {

}
