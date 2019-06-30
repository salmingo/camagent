/*
 * @file ioservice_keep.cpp 封装boost::asio::io_service, 维持run()在生命周期内的有效性
 * @date 2017-01-27
 * @version 0.1
 * @author Xiaomeng Lu
  */

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include "IOServiceKeep.h"

IOServiceKeep::IOServiceKeep() {
	work_.reset(new work(ios_));
	thrdkeep_.reset(new boost::thread(boost::bind(&IOServiceKeep::thread_keep, this)));
}

IOServiceKeep::~IOServiceKeep() {
	work_.reset();
	ios_.stop();
	thrdkeep_->join();
	thrdkeep_.reset();
}

io_service& IOServiceKeep::GetService() {
	return ios_;
}

void IOServiceKeep::thread_keep() {
	ios_.run();
}
