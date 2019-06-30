/*
 * @file CDs9.cpp 通过XPA在ds9中显示FITS图像
 */

#include <stdlib.h>
#include <fcntl.h>
#include <boost/filesystem.hpp>
#include <string>
#include "CDs9.h"
using std::string;

bool CDs9::showimg_ = false;

CDs9::CDs9() {
	firstimg_ = true;
	xpa_ = XPAOpen(0);
	strcpy(tmpl_, "DS9:ds9");
}

CDs9::~CDs9() {
	XPAClose(xpa_);
}

bool CDs9::StartDS9() {
	if (showimg_) return true;
	char **classes, **names, **methods, **infos;
	int n = XPANSLookup(NULL, "DS9:ds9", "s", &classes, &names, &methods, &infos);
	if ((showimg_ = n > 0)) {
		for (int i = 0; i < n; ++i) {
			free(classes[i]);
			free(names[i]);
			free(methods[i]);
			free(infos[i]);
		}
		free(classes);
		free(names);
		free(methods);
		free(infos);
	}
	else showimg_ = system("ds9&") != -1;

	return showimg_;
}

bool CDs9::DisplayImage(const char *filepath) {
	if (!showimg_) return false;
	boost::filesystem::path path(filepath);
	string filename = path.filename().string();

	if (XPASetFile(filepath, filename.c_str())) {
		if (firstimg_) {
			firstimg_ = false;
			XPASetScaleMode("zscale");
			XPASetZoom("to fit");
			XPAPreservePan();
			XPAPreserveRegion();
		}
		return true;
	}
	return false;
}

bool CDs9::XPASetFile(const char *filepath, const char *filename) {
	bool rslt(false);
	char *names, *messages, params[100], mode[50];
	int fd;

	strcpy(mode, "ack=false");
	sprintf(params, "fits %s", filename);
	fd = open(filepath, O_RDONLY);
	if (fd >= 0) {
		if (XPASetFd(xpa_, tmpl_, params, mode, fd, &names, &messages, 1)) {
			free(names);
			free(messages);
			rslt = true;
		}
		close(fd);
	}
	return rslt;
}

void CDs9::XPASetScaleMode(const char *mode) {
	char *names, *messages, params[40];

	sprintf(params, "scale mode %s", mode);
	if (XPASet(xpa_, tmpl_, params, NULL, NULL, 0, &names, &messages, 1)) {
		free(names);
		free(messages);
	}
}

void CDs9::XPASetZoom(const char *zoom) {
	char *names, *messages, params[40];

	sprintf(params, "zoom %s", zoom);
	if (XPASet(xpa_, tmpl_, params, NULL, NULL, 0, &names, &messages, 1)) {
		free(names);
		free(messages);
	}
}

void CDs9::XPAPreservePan(bool yes) {
	char *names, *messages, params[40];

	sprintf(params, "preserve pan %s", yes ? "yes" : "no");
	if (XPASet(xpa_, tmpl_, params, NULL, NULL, 0, &names, &messages, 1)) {
		free(names);
		free(messages);
	}
}

void CDs9::XPAPreserveRegion(bool yes) {
	char *names, *messages, params[40];

	sprintf(params, "preserve regions %s", yes ? "yes" : "no");
	if (XPASet(xpa_, tmpl_, params, NULL, NULL, 0, &names, &messages, 1)) {
		free(names);
		free(messages);
	}
}
