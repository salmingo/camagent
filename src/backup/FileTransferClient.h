/*
 * @file FileTransferClient.h 上传文件至服务器的客户端接口
 * @version 0.2
 * @date 2018年12月22日
 * @author 卢晓猛
 * - 维护与服务器之间的网络连接
 * - 上传文件至服务器
 */

#ifndef SRC_FILETRANSFERCLIENT_H_
#define SRC_FILETRANSFERCLIENT_H_

#include <string>
#include <boost/container/deque.hpp>
#include "AsciiProtocol.h"
#include "IOServiceKeep.h"

using std::string;
using boost::asio::ip::tcp;

class FileTransferClient {
public:
	FileTransferClient(const string &host, const uint port);
	virtual ~FileTransferClient();

public:
	/*!
	 * @struct 待上传文件描述信息
	 */
	struct FileDesc {
		string tmobs;		//< 观测时间
		string subpath;		//< 子目录名称
		string filename;	//< 文件名称
		string filepath;	//< 待上传文件的本地存储路径
	};

	typedef boost::shared_ptr<FileDesc> FileDescPtr;
	typedef boost::container::deque<FileDescPtr> FileDescQue;

protected:
	typedef boost::unique_lock<boost::mutex> mutex_lock;	//< 互斥锁
	typedef boost::shared_ptr<boost::thread> threadptr;		//< 线程

protected:
	IOServiceKeep keep_;	//< boost::asio服务
	string host_;	//< 文件服务器地址
	uint port_;		//< 文件服务器端口
	AscProtoPtr ascproto_;	//< 通信协议接口
	boost::shared_array<char> bufrcv_;		//< 网络信息接收缓冲区
	boost::shared_ptr<tcp::socket> sock_;	//< 与文件服务器之间的网络连接
	apfileinfo fileinfo_;	//< 文件描述信息
	FileDescQue fileque_;	//< 待上传文件队列
	boost::mutex mtx_que_;	//< 互斥锁: 文件队列
	boost::condition_variable cv_upload_;	//< 事件: 触发上传文件
	threadptr thrd_alive_;	//< 线程: 处理网络连接活跃性
	threadptr thrd_upload_;	//< 线程: 上传文件

public:
	/*!
	 * @brief 启动服务
	 */
	void Start();
	/*!
	 * @brief 停止服务
	 */
	void Stop();
	/*!
	 * @brief 设置设备编号
	 * @param gid 组标志
	 * @param uid 单元标志
	 * @param cid 相机标志
	 */
	void SetDeviceID(const string &gid, const string &uid, const string &cid);
	/*!
	 * @brief 设置观测目标天区编号
	 * @param grid   天区划分模式
	 * @param field  天区编号
	 */
	void SetObject(const string &grid, const string &field);
	/*!
	 * @brief 为新文件生成描述信息存储地址
	 * @return
	 * 存储地址
	 */
	FileDescPtr NewFile();
	/*!
	 * @brief 上传文件
	 * @param ptr 文件描述信息存储地址
	 */
	void UploadFile(FileDescPtr ptr);

protected:
	/*!
	 * @brief 连接文件服务器
	 * @return
	 * 网络连接结果
	 */
	bool connect_server();
	/*!
	 * @brief 上传文件
	 * @param fileptr 文件描述信息
	 * @return
	 * 文件上传结果
	 */
	bool upload_file(FileDescPtr fileptr);
	/*!
	 * @brief 上传文件描述信息
	 */
	bool upload_fileinfo();
	/*!
	 * @brief 上传文件数据
	 * @param data 文件数据存储地址
	 * @param n    文件数据长度, 量纲: 字节
	 */
	bool upload_filedata(char const *data, int n);
	/*!
	 * @brief 读取文件上传结果
	 * @return
	 * - 1: 服务器完成准备, 通知客户端可以发送文件数据
	 * - 2: 服务器完成接收, 通知客户端可以发送其它文件
	 * - 3: 文件接收错误
	 */
	int download_flag();

protected:
	/*!
	 * @brief 线程: 检查并处理网络连接的活跃性
	 */
	void thread_alive();
	/*!
	 * @brief 线程: 上传文件
	 */
	void thread_upload();
};
typedef boost::shared_ptr<FileTransferClient> FTClientPtr;

#endif /* SRC_FILETRANSFERCLIENT_H_ */
