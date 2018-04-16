/*
 * @file ConfigurationParameter.cpp 配置文件定义文件
 * @version 0.1
 * @author 卢晓猛
 * @email lxm@nao.cas.cn
 * @note
 * 管理配置参数
 */
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "globaldef.h"
#include "ConfigurationParameter.h"

using boost::property_tree::ptree;

ConfigurationParameter::ConfigurationParameter(const string &filepath) {
	filepath_ = filepath;
	camType_ = -1;
	readport_ = readrate_ = gain_ = emgain_ = 0;
	coolerset_ = tsaturate_ = 0.0;
	tLow_ = tHigh_ = tExpect_ = 0.0;
	etMin_ = etMax_ = 0.0;
	bFilter_ = false;
	bAutoFree_ = false;
	bShowImg_ = false;
}

ConfigurationParameter::~ConfigurationParameter() {
}

void ConfigurationParameter::Init() {
	ptree pt;	//< 以ptree存储配置参数
	pt.add("Server.<xmlattr>.Port",     port_ = 4021);
	pt.add("Repository.<xmlattr>.Path", repo_ = "/usr/local");

	boost::property_tree::xml_writer_settings<string> settings(' ', 4);
	write_xml(filepath_, pt, std::locale(), settings);
}

void ConfigurationParameter::Load() {
	ptree  pt;	//< 以ptree存储配置参数
	read_xml(filepath_, pt, boost::property_tree::xml_parser::trim_whitespace);
	port_ = pt.get("Server.<xmlattr>.Port", 4021);
	repo_ = pt.get("Repository.<xmlattr>.Path", "/usr/local");
}

uint16_t ConfigurationParameter::GetPort() {
	return port_;
}

string ConfigurationParameter::GetRepo() {
	return repo_;
}
