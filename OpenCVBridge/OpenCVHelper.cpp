#include "pch.h"
#include "OpenCVHelper.h"
#include "MemoryBuffer.h"

#include <algorithm>

using namespace OpenCVBridge;
using namespace Platform;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Foundation;
using namespace Microsoft::WRL;
using namespace cv;

bool OpenCVHelper::GetPointerToPixelData(SoftwareBitmap^ bitmap, unsigned char** pPixelData, unsigned int* capacity)
{
	BitmapBuffer^ bmpBuffer = bitmap->LockBuffer(BitmapBufferAccessMode::ReadWrite);
	IMemoryBufferReference^ reference = bmpBuffer->CreateReference();

	ComPtr<IMemoryBufferByteAccess> pBufferByteAccess;
	if ((reinterpret_cast<IInspectable*>(reference)->QueryInterface(IID_PPV_ARGS(&pBufferByteAccess))) != S_OK)
	{
		return false;
	}

	if (pBufferByteAccess->GetBuffer(pPixelData, capacity) != S_OK)
	{
		return false;
	}
	return true;
}

bool OpenCVHelper::TryConvert(SoftwareBitmap^ from, Mat& convertedMat)
{
	unsigned char* pPixels = nullptr;
	unsigned int capacity = 0;
	if (!GetPointerToPixelData(from, &pPixels, &capacity))
	{
		return false;
	}

	Mat mat(from->PixelHeight,
		from->PixelWidth,
		CV_8UC4, // assume input SoftwareBitmap is BGRA8
		(void*)pPixels);

	// shallow copy because we want convertedMat.data = pPixels
	// don't use .copyTo or .clone
	convertedMat = mat;
	return true;
}

void OpenCVHelper::Blur(SoftwareBitmap^ input, SoftwareBitmap^ output)
{
	Mat inputMat, outputMat;	
	if (!(TryConvert(input, inputMat) && TryConvert(output, outputMat)))
	{
		return;
	}
	blur(inputMat, outputMat, cv::Size(15, 15));
}

void OpenCVHelper::Process(SoftwareBitmap^ input, SoftwareBitmap^ output)
{
	Mat inputMat, outputMat;
	if (!(TryConvert(input, inputMat) && TryConvert(output, outputMat)))
	{
		return;
	}

	Mat src_gray;
	Mat canny_output;
	int thresh = 50;
	int max_thresh = 255;
	std::vector<std::vector<cv::Point> > contours;
	std::vector<Vec4i> hierarchy;

	//Mat inputFlipped;
	//flip(inputMat, inputFlipped, 1);

	cvtColor(inputMat, src_gray, CV_BGRA2GRAY);
	blur(src_gray, src_gray, cv::Size(3, 3));
	Canny(src_gray, canny_output, thresh, thresh * 3);
	findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	for (int i = 0; i < contours.size(); i++)
	{
		drawContours(outputMat, contours, i, Scalar(255, 0, 0, 255), 2, 8, hierarchy, 0);
	}

	std::vector<RotatedRect> likelyRects;
	for (int i = 0; i < contours.size(); i++) {
		RotatedRect boundsR = minAreaRect(contours[i]);
		float ratio = boundsR.size.width / boundsR.size.height;
		if ((ratio > 20 || ratio < (1.f / 20.f)) && ratio != 0) {
			likelyRects.push_back(boundsR);
		}
	}
	if (likelyRects.size() > 0) {
		float maxSize = 0;
		int index = 0;
		for (int i = 0; i < likelyRects.size(); i++) {
			float rectArea = max(likelyRects[i].size.width, likelyRects[i].size.height);
			if (rectArea > maxSize) {
				maxSize = rectArea;
				index = i;
			}
		}
		std::vector<cv::Point> box;
		Point2f rect_points[4]; likelyRects[index].points(rect_points);
		for (int j = 0; j < 4; j++)
			line(inputMat, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0, 255), 20, 8);
	}
	
	/*
	std::vector<cv::Point> box;
	Point2f rect_points[4]; boundsR.points(rect_points);
	for (int j = 0; j < 4; j++)
		line(outputMat, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0, 255), 1, 8);

	auto tip = rect_points[0] + (rect_points[1] - rect_points[0]) * 0.5;
	auto base = rect_points[2] + (rect_points[3] - rect_points[2]) * 0.5;

	circle(outputMat, tip, 5, Scalar(0, 0, 255, 255));
	circle(outputMat, base, 5, Scalar(0, 0, 255, 255));
	*/
}

void OpenCVHelper::Flip(SoftwareBitmap^ input, SoftwareBitmap^ output)
{
	Mat inputMat, outputMat;
	if (!(TryConvert(input, inputMat) && TryConvert(output, outputMat)))
	{
		return;
	}
	flip(inputMat, outputMat, 1);
}