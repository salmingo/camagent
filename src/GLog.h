/*!
 * @file GLog.h  类GLog声明文件
 * @author       卢晓猛
 * @note         日志文件访问接口
 * @version      2.0
 * @date         2016年10月28日
 * @note
 * - 在日志目录下创建日志文件
 * - 每天一个日志文件, 文件命名规则是: <prefix>_yyyymmdd.log
 * - 日志分为3级, 依次为: Normal, Warn, Fault
 * - 每一行对应一条日志, 由四部分组成:
 *   1. 本地时间, hh:mm:ss
 *   2. 日志等级. 当为Normal时， 不记录
 *   3. 发生位置, 当为NULL时， 不记录
 *   4. 用户提交的日志信息
 * - 各线程共享使用日志文件
 * - 创建并维护消息队列
 * - 用户是消息队列的生产者
 * - 写文件线程是消息队列的消费者
 */

#ifndef GLOG_H_
#define GLOG_H_

#include <stdio.h>
#include <string>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/container/list.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using std::string;

enum LOG_LEVEL {// 日志等级
	LOG_NORMAL,	// 普通
	LOG_WARN,	// 警告, 可以继续操作
	LOG_FAULT	// 错误, 需清除错误再继续操作
};

struct one_log {// 单条日志
	LOG_LEVEL level;	//< 日志等级
	string dtc;			//< 产生日志的日期
	string tmc;			//< 产生日志的时间
	string pos;			//< 产生日志的位置
	string msg;			//< 日志信息

public:
	one_log(const string &ymd, LOG_LEVEL lvl = LOG_NORMAL) {
		dtc   = ymd;
		level = lvl;
	}
};
typedef boost::shared_ptr<one_log> ologptr;
typedef boost::container::list<ologptr> loglist;

class GLog {
public:
	GLog();
	virtual ~GLog();

protected:
	/* 数据类型 */
	typedef boost::unique_lock<boost::mutex> mutex_lock; //< 基于boost::mutex的互斥锁
	typedef boost::shared_ptr<boost::thread> threadptr;

protected:
	/* 成员变量 */
	boost::mutex mtxlist_;	//< 消息列表互斥区
	boost::condition_variable cvnew_;	//< 事件: 产生日志
	threadptr thrdwr_;	//< 写入线程
	loglist logs_;		//< 日志列表
	string ymd_;		//< 当前日志日期
	FILE *fd_;			//< 日志文件描述符
	string pathroot_;	//< 目录名
	string prefix_;		//< 文件名前缀

protected:
	/*!
	 * @brief 线程: 消费日志列表, 将日志写入文件
	 */
	void thread_write();
	/*!
	 * @brief 检查日志文件有效性
	 * @param ymd 日志创建日期
	 * @return
	 * 文件有效性. true: 可继续操作文件; false: 文件访问错误
	 * @note
	 * 当日期变更时, 需重新创建日志文件
	 */
	bool valid_file(const string &ymd);
	/*!
	 * @brief 创建新的日志并加入列表
	 * @param msg    日志信息
	 * @param where  日志发生位置
	 * @param level  日志级别
	 */
	void new_log(const char *msg, const char *where = NULL,
			const LOG_LEVEL level = LOG_NORMAL);
	/*!
	 * @brief 将日志列表中所有信息写入文件
	 */
	void flush_logs();

public:
	/*!
	 * @brief 显式启动日志写入操作
	 * @param pathroot 目录名
	 * @param prefix   文件名钱拽
	 */
	void Start(const char *pathroot = NULL, const char *prefix = NULL);
	/*!
	 * @brief 显式结束日志写入操作
	 */
	void Stop();
	/*!
	 * @brief 记录一条日志
	 * @param format  日志描述的格式和内容
	 */
	void Write(const char* format, ...);
	/*!
	 * @brief 记录一条日志
	 * @param level   日志等级
	 * @param where   事件位置
	 * @param format  日志描述的格式和内容
	 */
	void Write(const LOG_LEVEL level, const char* where, const char* format, ...);
};

extern GLog _gLog;		//< 日志全局访问接口

#endif /* GLOG_H_ */
