/*
 * IoConfig.cpp 类IOConfig定义文件
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "IoConfig.h"

using namespace std;

IoConfig::IoConfig(const char * filepath) {
	m_pHead = new onesection;
	strcpy (m_strFilepath, filepath);
	m_modified = false;
	LoadFile();
}

IoConfig::~IoConfig() {
	if (m_modified)
		SaveFile();

	onesection * p1 = m_pHead->next;
	onesection * p2;
	while (p1 != NULL) {
		p2 = p1->next;
		delete p1;
		p1 = p2;
	}
	delete m_pHead;
}

void IoConfig::LoadFile() {
	FILE * fp = fopen(m_strFilepath, "r");
	char line[300];
	onesection * sec0 = NULL;
	onesection * sec1;

	if (fp != NULL) {
		while (!feof(fp)) {
			if (NULL == fgets (line, 300, fp)) continue;
			if (line[0] == '[') {// new section
				if (sec0 == NULL) {
					sec0 = new onesection;
					m_pHead->next = sec0;
				}
				else {
					sec1 = new onesection;
					sec0->next = sec1;
					sec0 = sec1;
				}
				sec0->resolve_name(line);
			}
			else {
				assert(sec0 != NULL);
				kvpair * pair = new kvpair;
				assert(pair != NULL);
				if (pair->resolve(line)) sec0->insert(pair);
			}
		}

		fclose(fp);
	}
	m_pTail = sec0 == NULL ? m_pHead : sec0;
}

void IoConfig::SaveFile() {
	FILE * fp = fopen(m_strFilepath, "w");
	assert (fp != NULL);
	onesection * s = m_pHead->next;
	while (s != NULL) {
		if (s != m_pHead->next)
			fprintf (fp, "\n");
		fprintf (fp, "[%s]\n", s->section_name.c_str());
		kvpair * p = s->head;
		while (p != NULL) {
			fprintf (fp, "%s = %s", p->keyword.c_str(), p->value.c_str());
			if (p->comment != "")
				fprintf (fp, " # %s", p->comment.c_str());
			fprintf (fp, "\n");
			p = p->next;
		}
		s = s->next;
	}
	fprintf (fp, "\n");
	fclose(fp);
}

bool IoConfig::GetConfigBool(const char* section_name, const char* keyword, const bool bDefault) {
	char strdef[20];
	const char* ret;

	if (bDefault) strcpy(strdef, "True");
	else          strcpy(strdef, "False");

	ret = GetConfigString(section_name, keyword, strdef);

	if (strcasecmp(ret, "True") == 0)       return true;
	else if (strcasecmp(ret, "False") == 0) return false;
	else                                    return 0 != atoi(ret);
}

int IoConfig::GetConfigInt(const char * section_name, const char * keyword, const int nDefault) {
	char buff[20];
	sprintf (buff, "%d", nDefault);

	return atoi(GetConfigString(section_name, keyword, buff));
}

const char * IoConfig::GetConfigString(const char * section_name, const char * keyword, const char * strDefault) {
	onesection * s = m_pHead->next;
	while (s != NULL) {
		if (0 == strcasecmp(s->section_name.c_str(), section_name)) break;
		s = s->next;
	}
	if (s == NULL) return strDefault;
	kvpair * p = s->find(keyword);
	if (p == NULL) return strDefault;
	return p->value.c_str();
}

void IoConfig::SetConfigBool(const char * section_name, const char * keyword, const bool bValue) {
	if (bValue) SetConfigString(section_name, keyword, "True");
	else        SetConfigString(section_name, keyword, "False");
}

void IoConfig::SetConfigInt(const char * section_name, const char * keyword, const int nValue) {
	char buff[20];
	sprintf (buff, "%d", nValue);
	SetConfigString(section_name, keyword, buff);
}

void IoConfig::SetConfigString(const char * section_name, const char * keyword, const char * strValue) {
	onesection * s = m_pHead->next;
	while (s != NULL) {
		if (0 == strcasecmp(s->section_name.c_str(), section_name)) break;
		s = s->next;
	}
	if (s == NULL) {// 创建区块
		s = new onesection;
		assert(s != NULL);
		s->section_name = section_name;
		m_pTail->next = s;
		m_pTail = s;
	}
	kvpair * p = s->find(keyword);
	if (p == NULL) {// 创建键-值对
		p = new kvpair;
		assert(p != NULL);
		p->keyword = keyword;
		s->insert(p);
	}
	p->value = strValue;

	m_modified = true;
}
