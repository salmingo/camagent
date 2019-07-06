/*
 * @brief FitsHandler.cpp 基于cfitsio的FITS文件访问接口
 */
#include <stdio.h>
#include "FitsHandler.h"

namespace AstroUtil {
//////////////////////////////////////////////////////////////////////////////
FitsHandler::FitsHandler() {
	fileptr_ = NULL;
	rows_ = cols_ = 0;
}

FitsHandler::~FitsHandler() {
	close();
}

void FitsHandler::close() {
	if (fileptr_) {
		int status(0);
		fits_close_file(fileptr_, &status);
		fileptr_ = NULL;
	}
}

void FitsHandler::fill_errmsg(int code) {
	if (code)
		ffgerr(code, errmsg);
}

fitsfile *FitsHandler::operator()() {
	return fileptr_;
}

bool FitsHandler::Open(const char *filepath) {
	close();
	// 尝试打开文件
	int status(0);
	int naxis, naxes[2], i, j;
	char name[10];

	fits_open_file(&fileptr_, filepath, 0, &status);
	fits_read_key(fileptr_, TINT, "NAXIS", &naxis, NULL, &status);
	for (j = naxis, i = 1; i >= 0; --i, --j) {
		sprintf(name, "NAXIS%d", j);
		fits_read_key(fileptr_, TINT, name, naxes + i, NULL, &status);
	}
	cols_ = naxes[0];
	rows_ = naxes[1];

	fill_errmsg(status);
	return status == 0;
}

bool FitsHandler::CreateImage(const char *filepath, int bitpix, int width,
		int height) {
	close();
	// 尝试创建文件
	int status(0);
	int naxis(2);
	long naxes[] = { width, height };

	fits_create_file(&fileptr_, filepath, &status);
	fits_create_img(fileptr_, bitpix, naxis, naxes, &status);
	if (!status) {
		cols_ = width;
		rows_ = height;
	}
	fill_errmsg(status);
	return status == 0;
}

void FitsHandler::GetDimension(int &cols, int &rows) {
	cols = cols_;
	rows = rows_;
}

float FitsHandler::GetExptime() {
	if (!fileptr_)
		return -1.0;
	int status(0);
	float expt(-1.0);
	fits_read_key(fileptr_, TFLOAT, "EXPTIME", &expt, NULL, &status);
	fill_errmsg(status);
	return expt;
}

/*!
 * @brief 查询曝光起始时间/日期
 * @return
 * 查询结果
 */
bool FitsHandler::GetDateobs(string &dateobs) {
	if (!fileptr_)
		return false;
	int status(0);
	char str[40];
	fits_read_key(fileptr_, TSTRING, "DATE-OBS", str, NULL, &status);
	fill_errmsg(status);
	if (!status)
		dateobs = str;

	return status == 0;
}
/*!
 * @brief 查询曝光起始时间
 * @param timeobs
 * @return
 * 查询结果
 */
bool FitsHandler::GetTimeobs(string &timeobs) {
	if (!fileptr_)
		return false;
	int status(0);
	char str[40];
	fits_read_key(fileptr_, TSTRING, "TIME-OBS", str, NULL, &status);
	fill_errmsg(status);
	if (!status)
		timeobs = str;

	return status == 0;
}

bool FitsHandler::LoadPixels(float *data, int pixels, int row, int col) {
	if (!fileptr_)
		return false;
	int status(0);
	if (pixels <= 0)
		pixels = cols_;
	fits_read_img(fileptr_, TFLOAT, row * cols_ + col + 1, pixels, NULL, data,
			NULL, &status);
	fill_errmsg(status);

	return status == 0;
}

bool FitsHandler::LoadImage(float *data) {
	if (!fileptr_)
		return false;
	int status(0);
	fits_read_img(fileptr_, TFLOAT, 1, rows_ * cols_, NULL, data, NULL,
			&status);
	fill_errmsg(status);

	return status == 0;
}

bool FitsHandler::WriteImage(float *data, int datatype) {
	if (!fileptr_)
		return false;
	int status(0);
	long pixels = rows_ * cols_;
	fits_write_img(fileptr_, datatype, 1, pixels, data, &status);

	fill_errmsg(status);
	return status == 0;
}
//////////////////////////////////////////////////////////////////////////////
} /* namespace AstroUtil */
