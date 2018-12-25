/*
 * @file FileTransferClient.cpp 上传文件至服务器的客户端接口
 */
#include <boost/make_shared.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "FileTransferClient.h"
#include "GLog.h"

using namespace boost::asio;
using namespace boost::interprocess;

FileTransferClient::FileTransferClient(const string &host, const uint port) {
	host_ = host;
	port_ = port;
	fileinfo_ = boost::make_shared<ascii_proto_fileinfo>();
	ascproto_ = make_ascproto();
	bufrcv_.reset(new char[100]);
}

FileTransferClient::~FileTransferClient() {
}

void FileTransferClient::Start() {
	thrd_alive_.reset(new boost::thread(boost::bind(&FileTransferClient::thread_alive, this)));
	thrd_upload_.reset(new boost::thread(boost::bind(&FileTransferClient::thread_upload, this)));
}

void FileTransferClient::Stop() {
	if (thrd_alive_.unique()) {
		thrd_alive_->interrupt();
		thrd_alive_->join();
		thrd_alive_.reset();
	}
	if (thrd_upload_.unique()) {
		thrd_upload_->interrupt();
		thrd_upload_->join();
		thrd_upload_.reset();
	}
	if (sock_.unique()) {
		boost::system::error_code ec;
		sock_->close(ec);
		sock_.reset();
	}
}

void FileTransferClient::SetDeviceID(const string &gid, const string &uid, const string &cid) {
	fileinfo_->gid = gid;
	fileinfo_->uid = uid;
	fileinfo_->cid = cid;
}

void FileTransferClient::SetObject(const string &grid, const string &field) {
	fileinfo_->grid  = grid;
	fileinfo_->field = field;
}

FileTransferClient::FileDescPtr FileTransferClient::NewFile() {
	return boost::make_shared<FileDesc>();
}

void FileTransferClient::UploadFile(FileDescPtr ptr) {
	mutex_lock lck(mtx_que_);
	fileque_.push_back(ptr);
	if (fileque_.size() == 1 && sock_.unique()) cv_upload_.notify_one();
}

bool FileTransferClient::connect_server() {
	if (!sock_.unique()) {
		try {
			tcp::resolver resolver(keep_.get_service());
			tcp::resolver::query query(host_, boost::lexical_cast<string>(port_));
			tcp::resolver::iterator itertor = resolver.resolve(query);
			sock_.reset(new tcp::socket(keep_.get_service()));
			boost::asio::connect(*sock_, itertor);
			sock_->set_option(socket_base::keep_alive(true));
		}
		catch(std::exception& ex) {
			_gLog.Write(LOG_FAULT, NULL, "failed to connect FileServer[%s:%d]: %s",
					host_.c_str(), port_, ex.what());
			sock_.reset();
		}
	}

	return (sock_.unique() && sock_->is_open());
}

bool FileTransferClient::upload_file(FileDescPtr fileptr) {
	_gLog.Write("upload: %s", fileptr->filename.c_str());

	file_mapping fitsfile(fileptr->filepath.c_str(), read_only);
	mapped_region region(fitsfile, read_only);
	char* data = (char*) region.get_address();

	fileinfo_->tmobs    = fileptr->tmobs;
	fileinfo_->subpath  = fileptr->subpath;
	fileinfo_->filename = fileptr->filename;
	fileinfo_->filesize = region.get_size();

	if (!upload_fileinfo()) return false;
	if (download_flag() != 1) return false;
	if (!upload_filedata(data, fileinfo_->filesize)) return false;
	if (download_flag() != 2) return false;
	return true;
}

bool FileTransferClient::upload_fileinfo() {
	try {
		int n;
		const char *s = ascproto_->CompactFileInfo(fileinfo_, n);
		sock_->write_some(buffer(s, n));
		return true;
	}
	catch(std::exception &ex) {
		_gLog.Write(LOG_FAULT, "FileTransferClient::upload_fileinfo", "%s", ex.what());
		return false;
	}
}

bool FileTransferClient::upload_filedata(char const *data, int n) {
	try {
		int had(0);
		while (had < n) {
			had += sock_->write_some(buffer(data + had, n - had));
		}
		return true;
	}
	catch(std::exception &ex) {
		_gLog.Write(LOG_FAULT, "FileTransferClient::upload_filedata", "%s", ex.what());
		return false;
	}
}

int FileTransferClient::download_flag() {
	int flag(0);

	try {
		int n = sock_->read_some(buffer(bufrcv_.get(), 100));
		if (bufrcv_[n - 1] == '\n') {
			apbase proto;
			bufrcv_[n - 1] = 0;
			proto = ascproto_->Resolve(bufrcv_.get());
			if (boost::iequals(proto->type, APTYPE_FILESTAT))
				return from_apbase<ascii_proto_filestat>(proto)->status;
		}
	}
	catch(std::exception &ex) {
		_gLog.Write(LOG_FAULT, "FileTransferClient::download_flag", "%s", ex.what());
	}
	return flag;
}

/*
 * 检查网络连接活跃性, 并尝试重新连接服务器
 * - 网络已断开
 * - 有文件等待上传
 */
void FileTransferClient::thread_alive() {
	boost::chrono::minutes period(1);

	while(1) {
		boost::this_thread::sleep_for(period);
		if (!sock_.unique() && fileque_.size() && connect_server()) {
			cv_upload_.notify_one();
		}
	}
}

void FileTransferClient::thread_upload() {
	boost::mutex mtx;
	mutex_lock lck(mtx);

	while(1) {
		cv_upload_.wait(lck);

		while (fileque_.size() && sock_.unique()) {
			FileDescPtr fileptr = fileque_.front();
			if (upload_file(fileptr)) {
				mutex_lock lck(mtx_que_);
				fileque_.pop_front();
			}
			else if (sock_.unique()){// 文件传输失败时, 断开网络连接
				boost::system::error_code ec;
				sock_->close(ec);
				sock_.reset();
			}
		}
	}
}
