#pragma once
#include <QImage>
#undef NO // MacOS macro that can conflict with OpenCV
#include <opencv2/opencv.hpp>

#ifdef OCR_SUPPORT
#include <tesseract/baseapi.h>
#else
namespace tesseract {
enum PageSegMode {
	PSM_OSD_ONLY = 0,
	PSM_AUTO_OSD = 1,
	PSM_AUTO_ONLY = 2,
	PSM_AUTO = 3,
	PSM_SINGLE_COLUMN = 4,
	PSM_SINGLE_BLOCK_VERT_TEXT = 5,
	PSM_SINGLE_BLOCK = 6,
	PSM_SINGLE_LINE = 7,
	PSM_SINGLE_WORD = 8,
	PSM_CIRCLE_WORD = 9,
	PSM_SINGLE_CHAR = 10,
	PSM_SPARSE_TEXT = 11,
	PSM_SPARSE_TEXT_OSD = 12,
	PSM_RAW_LINE = 13,

	PSM_COUNT
};

class TessBaseAPI {
public:
	void SetPageSegMode(PageSegMode) {}
	int Init(const char *, const char *) { return 0; }
	void End() {}
};
}
#endif

namespace advss {

constexpr int minMinNeighbors = 3;
constexpr int maxMinNeighbors = 6;
constexpr double defaultScaleFactor = 1.1;

struct PatternImageData {
	cv::Mat4b rgbaPattern;
	cv::Mat3b rgbPattern;
	cv::Mat1b mask;
};

PatternImageData createPatternData(QImage &pattern);
void matchPattern(QImage &img, const PatternImageData &patternData,
		  double threshold, cv::Mat &result, bool useAlphaAsMask,
		  cv::TemplateMatchModes matchMode);
void matchPattern(QImage &img, QImage &pattern, double threshold,
		  cv::Mat &result, bool useAlphaAsMask,
		  cv::TemplateMatchModes matchMode);
std::vector<cv::Rect> matchObject(QImage &img, cv::CascadeClassifier &cascade,
				  double scaleFactor, int minNeighbors,
				  cv::Size minSize, cv::Size maxSize);
uchar getAvgBrightness(QImage &img);
cv::Mat preprocessForOCR(const QImage &image, const QColor &color);
std::string runOCR(tesseract::TessBaseAPI *, const QImage &, const QColor &);
cv::Mat QImageToMat(const QImage &img);
QImage MatToQImage(const cv::Mat &mat);

} // namespace advss
