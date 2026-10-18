#include <QImage>

#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/thread_parallel_runner.h>
#include <jxl/thread_parallel_runner_cxx.h>

#include "QtTypes.h"
#include "jxl.h"
#include "log.h"

namespace HomeCompa::JXL {

#define JXL_GO2(F, P1, P2) if (JXL_ENC_SUCCESS != F((P1), (P2))) { PLOGE << #F" failed"; return {}; }
#define JXL_GO3(F, P1, P2, P3) if (JXL_ENC_SUCCESS != F((P1), (P2), (P3))) { PLOGE << #F" failed"; return {}; }
#define JXL_GO4(F, P1, P2, P3, P4) if (JXL_ENC_SUCCESS != F((P1), (P2), (P3), (P4))) { PLOGE << #F" failed"; return {}; }

QByteArray Encode(const QImage& image, const int quality)
{
	const JxlEncoderPtr              enc    = JxlEncoderMake(/*memory_manager=*/nullptr);
	const JxlThreadParallelRunnerPtr runner = JxlThreadParallelRunnerMake(
		/*memory_manager=*/nullptr,
		JxlThreadParallelRunnerDefaultNumWorkerThreads()
	);
	if (JXL_ENC_SUCCESS != JxlEncoderSetParallelRunner(enc.get(), JxlThreadParallelRunner, runner.get()))
	{
		PLOGE << "JxlEncoderSetParallelRunner failed";
		return {};
	}

	JxlEncoderFrameSettings* frameSettings = JxlEncoderFrameSettingsCreate(enc.get(), nullptr);
	if (!frameSettings)
	{
		PLOGE << "JxlEncoderFrameSettingsCreate failed";
		return {};
	}

	const auto distance = JxlEncoderDistanceFromQuality(static_cast<float>(quality > 0 ? quality : 70));
	JXL_GO2(JxlEncoderSetFrameLossless, frameSettings, JXL_FALSE)
	JXL_GO2(JxlEncoderSetFrameDistance, frameSettings, distance)
	JXL_GO2(JxlEncoderUseContainer, enc.get(), 0)
	JXL_GO2(JxlEncoderSetCodestreamLevel, enc.get(), -1)
	JXL_GO3(JxlEncoderFrameSettingsSetOption, frameSettings, JXL_ENC_FRAME_SETTING_GABORISH, 1)
	JXL_GO3(JxlEncoderFrameSettingsSetOption, frameSettings, JXL_ENC_FRAME_SETTING_EPF, 1)
	JXL_GO3(JxlEncoderFrameSettingsSetOption, frameSettings, JXL_ENC_FRAME_SETTING_GROUP_ORDER, 1)
#ifdef NDEBUG
	JXL_GO3(JxlEncoderFrameSettingsSetOption, frameSettings, JXL_ENC_FRAME_SETTING_EFFORT, 10)
#endif

	const auto           imagePixelFormat = image.pixelFormat();
	const JxlPixelFormat pixelFormat { imagePixelFormat.channelCount(), JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, static_cast<size_t>(image.bytesPerLine()) };

	JxlBasicInfo basicInfo;
	JxlEncoderInitBasicInfo(&basicInfo);
	basicInfo.xsize                    = image.width();
	basicInfo.ysize                    = image.height();
	basicInfo.bits_per_sample          = 8;
	basicInfo.exponent_bits_per_sample = 0;
	basicInfo.uses_original_profile    = JXL_FALSE;
	basicInfo.num_color_channels       = imagePixelFormat.colorModel() == QPixelFormat::Grayscale ? 1 : 3;
	basicInfo.num_extra_channels       = imagePixelFormat.alphaUsage() == QPixelFormat::AlphaUsage::UsesAlpha ? 1 : 0;
	if (basicInfo.num_extra_channels != 0)
		basicInfo.alpha_bits = 8;

	JxlBitDepth bitDepth { JXL_BIT_DEPTH_FROM_PIXEL_FORMAT, 0, 0 };

	JXL_GO2(JxlEncoderSetBasicInfo, enc.get(), &basicInfo)
	JXL_GO3(JxlEncoderSetUpsamplingMode, enc.get(), 1, 1)
	JXL_GO2(JxlEncoderSetFrameBitDepth, frameSettings, &bitDepth)

	if (basicInfo.num_extra_channels != 0)
		JXL_GO3(JxlEncoderSetExtraChannelDistance, frameSettings, 0, 0.0f)

	JxlColorEncoding colorEncoding = {};
	JXL_BOOL         isGray        = TO_JXL_BOOL(pixelFormat.num_channels < 3);
	JxlColorEncodingSetToSRGB(&colorEncoding, isGray);

	JXL_GO2(JxlEncoderSetColorEncoding, enc.get(), &colorEncoding)
	JXL_GO4(JxlEncoderAddImageFrame, frameSettings, &pixelFormat, image.constBits(), image.height() * image.bytesPerLine())

	JxlEncoderCloseInput(enc.get());

	std::vector<uint8_t> compressed;
	compressed.resize(16LL * 1024);
	uint8_t* nextOut  = compressed.data();
	size_t   availOut = compressed.size() - (nextOut - compressed.data());

	JxlEncoderStatus processResult = JXL_ENC_NEED_MORE_OUTPUT;
	while (processResult == JXL_ENC_NEED_MORE_OUTPUT)
	{
		processResult = JxlEncoderProcessOutput(enc.get(), &nextOut, &availOut);
		if (processResult == JXL_ENC_NEED_MORE_OUTPUT)
		{
			const auto offset = nextOut - compressed.data();
			compressed.resize(compressed.size() * 2);
			nextOut  = compressed.data() + offset;
			availOut = compressed.size() - offset;
		}
	}
	compressed.resize(nextOut - compressed.data());
	if (JXL_ENC_SUCCESS != processResult)
	{
		PLOGE << "JxlEncoderProcessOutput failed";
		return {};
	}

	return QByteArray { reinterpret_cast<char*>(compressed.data()), static_cast<qsizetype_t>(compressed.size()) };
}

} // namespace HomeCompa::JXL
