/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Google Inc.
 *
 * aiq.cpp - Intel IA Imaging library C++ wrapper
 */

#include "libcamera/internal/file.h"
#include "libcamera/internal/log.h"

#include "parameter_encoder.h"

#include "aiq.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(AIQ)

/**
 * \brief Binary Data wrapper
 *
 * Loads data from a file, and returns it as an ia_binary_data type.
 * Data is freed automatically when the object goes out of scope.
 */
class AIQBinaryData
{
public:
	// Disallow copy / assign...
	AIQBinaryData();

	int load(const char *filename);
	ia_binary_data *data() { return &iaBinaryData_; }

private:
	ia_binary_data iaBinaryData_;
	std::vector<uint8_t> data_;
};

AIQBinaryData::AIQBinaryData()
{
	iaBinaryData_.data = nullptr;
	iaBinaryData_.size = 0;
}

int AIQBinaryData::load(const char *filename)
{
	File binary(filename);

	if (!binary.exists()) {
		LOG(AIQ, Error) << "Failed to find file: " << filename;
		return -ENOENT;
	}

	if (!binary.open(File::ReadOnly)) {
		LOG(AIQ, Error) << "Failed to open: " << filename;
		return -EINVAL;
	}

	ssize_t fileSize = binary.size();
	if (fileSize < 0) {
		LOG(AIQ, Error) << "Failed to determine fileSize: " << filename;
		return -ENODATA;
	}

	data_.resize(fileSize);

	int bytesRead = binary.read(data_);
	if (bytesRead != fileSize) {
		LOG(AIQ, Error) << "Failed to read file: " << filename;
		return -EINVAL;
	}

	iaBinaryData_.data = data_.data();
	iaBinaryData_.size = fileSize;

	LOG(AIQ, Info) << "Successfully loaded: " << filename;

	return 0;
}

AIQ::AIQ()
{
	LOG(AIQ, Info) << "Creating IA AIQ Wrapper";
}

AIQ::~AIQ()
{
	LOG(AIQ, Info) << "Destroying IA AIQ Wrapper";
	ia_aiq_deinit(aiq_);
}

int AIQ::init()
{
	AIQBinaryData aiqb;
	AIQBinaryData nvm;
	AIQBinaryData aiqd;

	unsigned int stats_max_width = 1920;
	unsigned int stats_max_height = 1080;
	unsigned int max_num_stats_in = 4;
	ia_cmc_t *ia_cmc = nullptr;
	ia_mkn *ia_mkn = nullptr;

	int ret = aiqb.load("/etc/camera/ipu3/00imx258.aiqb");
	if (ret) {
		LOG(AIQ, Error) << "Not quitting";
	}

	/* Width, Height, other parameters to be set as parameters? */
	aiq_ = ia_aiq_init(aiqb.data(), nvm.data(), aiqd.data(),
			   stats_max_width, stats_max_height, max_num_stats_in,
			   ia_cmc, ia_mkn);
	if (!aiq_) {
		LOG(AIQ, Error) << "Failed to initialise aiq library";
		return -ENODATA;
	}

	return 0;
}

int AIQ::configure()
{
	LOG(AIQ, Debug) << "Configure AIQ";

	return 0;
}

int AIQ::setStatistics(unsigned int frame, const ipu3_uapi_stats_3a *stats)
{
	LOG(AIQ, Debug) << "Set Statistitcs";

	(void)frame;
	(void)stats;
	ia_aiq_statistics_input_params stats_param = {};

	/* We should give the converted statistics into the AIQ library here. */

	ia_err err = ia_aiq_statistics_set(aiq_, &stats_param);
	if (err) {
		LOG(AIQ, Error) << "Failed to set statistics: "
				<< ia_err_decode(err);

		LOG(AIQ, Error) << "Not quitting";
	}

	return 0;
}

/*
 * Run algorithms, and store the configuration in the parameters buffers
 * This is likely to change drastically as we progress, and the algorithms
 * might run asycnronously, or after receipt of statistics, with the filling
 * of the parameter buffer being the only part handled when called for.
 */
int AIQ::run(unsigned int frame, ipu3_uapi_params *params)
{
	(void)frame;

	aic_config config;

	/* Run AWB algorithms, using the config structures. */

	/* IPU3 firmware specific encoding for ISP controls. */
	ParameterEncoder::encode(&config, params);

	return 0;
}

} /* namespace libcamera */