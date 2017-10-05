/**
 * IoConfig.h 类IOConfig声明文件
 * 仿照Windows读写配置文件功能, 构建Linux/Unix平台访问文本格式配置的接口类
 * 配置文件由多个区块，和区块内的键-值对组成. 示例:
 * [Section1]
 * Keyword11 = Value11   # comment11
 * Keyword12 = Value12   # comment12
 *
 * [Section2]
 * Keyword21 = Value21   # comment21
 * Keyword22 = Value22   # comment22
 *
 * @date Jun 5, 2014
 * @author Xiaomeng Lu
 * @email lxm@nao.cas.cn
 * @ver 0.1
 *
 * @date Oct 29, 2016
 * @author Xiaomeng Lu
 * @version 0.2
 * @note
 * 扩展两个函数, 支持布尔型:
 * (1) SetConfigBool
 * (2) GetConfigBool
 **/

#ifndef IOCONFIG_H_
#define IOCONFIG_H_

#include <string>
#include <string.h>

class IoConfig
{
/**
 * 使用结构体构建链表, 作为配置项数据结构
 **/
public:
	struct kvpair {				//< 键-值对
		std::string keyword;	//< 键名
		std::string value;		//< 键值
		std::string comment;	//< 注释
		kvpair * next;

	public:
		kvpair() {
			keyword = "";
			value   = "";
			comment = "";
			next = NULL;
		}

		/**
		 * \brief 解析行信息, 将解析结果分别赋予键-值-注释
		 * \param[in] line 文件行信息
		 * \note
		 * 键-值之间用'='分隔, 值-注释之间用'#'分隔
		 */
		bool resolve(char *line) {
			int n = (int) strlen(line);
			if (line[n - 1] == '\r' || line[n - 1] == '\n') --n;
			if (n == 0) return false;
			int i, j;
			char * buff = new char[n];
			// 解析键名
			for (i = 0; i < n && line[i] == ' '; ++i);	// 消除行首空格
			for (j = 0; i < n && line[i] != '='; ++i, ++j) {
				buff[j] = line[i];
			}
			--j;
			while (j >= 0 && buff[j] == ' ') --j;	// 消除键名附加空格
			buff[j + 1] = 0;
			keyword = buff;
			// 解析键值
			for (++i; i < n && line[i] == ' '; ++i);	// 消除多余空格
			for (j = 0; i < n && line[i] != '#'; ++i, ++j) {
				buff[j] = line[i];
			}
			--j;
			while (j >= 0 && buff[j] == ' ') --j;
			buff[j + 1] = 0;
			value = buff;
			// 解析注释
			for (++i; i < n && line[i] == ' '; ++i);	// 消除多余空格
			for (j = 0; i < n; ++i, ++j) {
				buff[j] = line[i];
			}
			buff[j] = 0;
			comment = buff;
			// 释放临时资源
			delete []buff;
			return true;
		}
	};

	struct onesection {				//< 单个区块数据结构
		std::string section_name;	//< 区块名
		kvpair * head;				//< 区块内首个键-值对的地址, 主要用于从头遍历查询
		kvpair * tail;				//< 区块内最后一个键-值对的址, 主要用于从尾端插入新键-值对
		onesection * next;

	public:
		/** 构造函数 **/
		onesection() {
			head = NULL;
			tail = NULL;
			next = NULL;
		}

		/** 析构函数 **/
		~onesection() {
			kvpair *p1 = head;
			kvpair *p2;
			while (p1 != NULL) {// 逐一释放区块内为键-值对分配的内存
				p2 = p1->next;
				delete p1;
				p1 = p2;
			}
		}

		/**
		 * \brief 解析文件行信息, 提取区块名称
		 * \param[in] line 行信息
		 */
		void resolve_name(char * line) {
			int n = (int) strlen(line);
			if (line[n - 1] == '\r' || line[n - 1] == '\n') --n;
			--n;
			int i;
			for (i = 1; i < n; ++i) section_name += line[i];
		}

		/**
		 * \brief 插入一个新的键-值对
		 * \param[in] one 键值对地址
		 */
		void insert(kvpair * one) {
			if (head == NULL) head = one;
			if (tail == NULL) tail = one;
			else {
				tail->next = one;
				tail = one;
			}
		}

		/**
		 * \brief 查找区块内关键字匹配的键-值对
		 * \param[in] keyword 键名称
		 * \return
		 * 在区块内若能找到匹配的键-值对, 则返回其访问指针. 若不能找到则返回NULL
		 */
		kvpair * find(const char * keyword) {
			kvpair * p = head;

			while (p != NULL) {
				if (0 == strcasecmp(p->keyword.c_str(), keyword)) return p;
				p = p->next;
			}
			return NULL;
		}
	};

public:
	/**
	 * \brief 构造函数, 打开配置文件, 并解析文件内容至内存
	 * \param[in] filepath 文件路径名
	 */
	IoConfig(const char * filepath);
	virtual ~IoConfig();

protected:
	/** 加载文件至内存 **/
	void LoadFile();
	/** 保存内存数据至文件 **/
	void SaveFile();

public:
	/**
	 * \brief 从配置文件中读取对应项的布尔值
	 * \param[in] section_name 区块名
	 * \param[in] keyword      键名
	 * \param[in] bDefault     缺省值. 即文件中找不到对应项时返回该值
	 * \return
	 * 配置文件中的对应键值或者缺省值
	 */
	bool GetConfigBool(const char* section_name, const char* keyword, const bool bDefault);
	/**
	 * \brief 从配置文件中读取对应项的整数值
	 * \param[in] section_name 区块名
	 * \param[in] keyword      键名
	 * \param[in] nDefault     缺省值. 即文件中找不到对应项时返回该值
	 * \return
	 * 配置文件中的对应键值或者缺省值
	 */
	int GetConfigInt(const char * section_name, const char * keyword, const int nDefault);
	/**
	 * \brief 从配置文件中读取对应项的字符串
	 * \param[in] section_name 区块名
	 * \param[in] keyword      键名
	 * \param[in] strDefault   缺省值. 即文件中找不到对应项时返回该值
	 * \return
	 * 配置文件中的对应键值或者缺省值
	 */
	const char * GetConfigString(const char * section_name, const char * keyword, const char * strDefault);

	/**
	 * \brief 改变配置文件中对应项的布尔值
	 * \param[in] section_name 区块名
	 * \param[in] keyword      键名
	 * \param[in] bValue       数值
	 * \note
	 * 改变配置项中的对应键值. 若文件中没有对应区块则创建, 若文件中没有键-值对则创建
	 */
	void SetConfigBool(const char * section_name, const char * keyword, const bool bValue);
	/**
	 * \brief 改变配置文件中对应项的数值
	 * \param[in] section_name 区块名
	 * \param[in] keyword      键名
	 * \param[in] nValue       数值
	 * \note
	 * 改变配置项中的对应键值. 若文件中没有对应区块则创建, 若文件中没有键-值对则创建
	 */
	void SetConfigInt(const char * section_name, const char * keyword, const int nValue);
	/**
	 * \brief 改变配置文件中对应项的字符串
	 * \param[in] section_name 区块名
	 * \param[in] keyword      键名
	 * \param[in] strValue     字符串类型键值
	 * \note
	 * 改变配置项中的对应键值. 若文件中没有对应区块则创建, 若文件中没有键-值对则创建
	 */
	void SetConfigString(const char * section_name, const char * keyword, const char * strValue);

private:
	char m_strFilepath[300];	//< 文件路径名
	bool m_modified;			//< 修改标记
	onesection * m_pHead;		//< 配置项首地址
	onesection * m_pTail;		//< 配置项尾地址
};

#endif /* IOCONFIG_H_ */
