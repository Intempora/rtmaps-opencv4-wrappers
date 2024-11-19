/////////////////////////////////////////////////////////////////////////////////
//
//   Copyright 2014-2024 Intempora S.A.S.
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
/////////////////////////////////////////////////////////////////////////////////

#include "maps_OpenCV_Conversion.h"
#include <vector>
#include <sstream>

// #define CLAMP(val, low, high) (((value) < (low))? (low): (((value) > (high))? high : value))

template <typename IPL, typename MAT>
MAT noCopy(IPL image)
{
	if ((image->dataOrder == IPL_DATA_ORDER_PLANE) && (image->nChannels != 1))
	{
		throw std::domain_error("Cannot convert planar color IplImage to cv::Mat without a copy. cv::Mat does not support planar images.");
	}
	if (image->height <= 0)
	{
		throw std::domain_error("Image height <= 0. Cannot convert IplImage to cv::Mat");
	}
	if (image->width <= 0)
	{
		throw std::domain_error("Image width <= 0. Cannot convert IplImage to cv::Mat");
	}

	if (!image->roi)
	{
		return cv::Mat(static_cast<int>(image->height), static_cast<int>(image->width),
                       CV_MAKETYPE(image->depth == 8 ? 0 : image->depth / 8, image->nChannels),
                       image->imageData, image->widthStep);
	}
	else
	{
		if (image->roi->yOffset < 0)
		{
			std::ostringstream s;
			s << "Image ROI yOffset = [" << image->roi->yOffset << "] < 0. Cannot convert IplImage to cv::Mat";
			throw std::domain_error(s.str());
		}
		if (image->roi->yOffset > image->height)
		{
			std::ostringstream s;
			s << "Image ROI yOffset = [" << image->roi->yOffset << "] > image height = [" << image->height << "]. Cannot convert IplImage to cv::Mat.";
			throw std::domain_error(s.str());
		}
		int lastRow = image->roi->yOffset + image->roi->height;
		if (lastRow > image->height)
		{
			std::ostringstream s;
			s << "Image ROI last row = [" << lastRow << "] > image height = [" << image->height << "]. Cannot convert IplImage to cv::Mat";
			throw std::domain_error(s.str());
		}

		if (image->roi->xOffset < 0)
		{
			std::ostringstream s;
			s << "Image ROI xOffset = [" << image->roi->xOffset << "] < 0. Cannot convert IplImage to cv::Mat";
			throw std::domain_error(s.str());
		}
		if (image->roi->xOffset > image->width)
		{
			std::ostringstream s;
			s << "Image ROI xOffset = [" << image->roi->xOffset << "] > image width = [" << image->width << "]. Cannot convert IplImage to cv::Mat.";
			throw std::domain_error(s.str());
		}
		int lastCol = image->roi->xOffset + image->roi->width;
		if (lastCol > image->width)
		{
			std::ostringstream s;
			s << "Image ROI last column = [" << lastCol << "] > image width = [" << image->width << "]. Cannot convert IplImage to cv::Mat";
			throw std::domain_error(s.str());
		}

		cv::Mat shallowCopy = cv::Mat(static_cast<int>(image->height), static_cast<int>(image->width),
                                      CV_MAKETYPE(image->depth == 8 ? 0 : image->depth / 8, image->nChannels),
			image->imageData, image->widthStep);
		return shallowCopy(cv::Range(image->roi->yOffset, lastRow),
						   cv::Range(image->roi->xOffset, lastCol));
	}
}

cv::Mat convTools::noCopyIplImage2Mat(IplImage* image)
{
	return noCopy<IplImage*, cv::Mat>(image);
}

const cv::Mat convTools::noCopyIplImage2Mat(const IplImage* image)
{
	return noCopy<const IplImage*, const cv::Mat>(image);
}

cv::Mat convTools::copyIplImage2Mat(const IplImage* image)
{
	if ((image->dataOrder != IPL_DATA_ORDER_PLANE) || (image->nChannels == 1))
	{
		return noCopy<const IplImage*, const cv::Mat>(image).clone();
	}
	else
	{
		throw std::domain_error("Cannot convert planar color IplImage to cv::Mat.");
	}
}

