/*
 * @file globaldef.h 声明全局唯一参数
 */

#ifndef GLOBALDEF_H_
#define GLOBALDEF_H_

// 软件名称、版本与版权
#define DAEMON_NAME			"camagent"
#define DAEMON_VERSION		"v1.0 @ June, 2019"
#define DAEMON_AUTHORITY	"© SVOM Group"

// 日志文件路径与文件名前缀
const char gLogDir[]    = "/var/log/camagent";
const char gLogPrefix[] = "camagent_";

// 软件配置文件
const char gConfigPath[] = "/usr/local/etc/camagent.xml";

// 文件锁位置
const char gPIDPath[] = "/var/run/camagent.pid";

#endif /* GLOBALDEF_H_ */
