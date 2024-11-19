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

#ifndef _Maps_OpenCV_Conversion_H
#define _Maps_OpenCV_Conversion_H

#include <opencv2/opencv.hpp>
#include "maps.hpp"

namespace convTools
{
    // Copy the IplImage to create a cv::Mat object. Use copy only when needed.
    cv::Mat copyIplImage2Mat(const IplImage* image);

    // Don't copy the IplImage and create a cv::Mat object. Use constness to stop us from writing on an image that we shouldn't (e.g: Input image)
    const cv::Mat noCopyIplImage2Mat(const IplImage* image);

    // Don't copy the IplImage and create a cv::Mat object. Overload of previous one, use it when we have an IplImage that we can write on (e.g Output image)
    cv::Mat noCopyIplImage2Mat(IplImage* image);

    // Don't copy the IplImage and create a cv::Mat object. Use constness to stop us from writing on an image that we shouldn't (e.g: Input image)
    const cv::Mat noCopyIplImage2MatRoi(const IplImage* image, MAPSInt32 x, MAPSInt32 y, MAPSInt32 width, MAPSInt32 height);

    // Don't copy the IplImage and create a cv::Mat object. Overload of previous one, use it when we have an IplImage that we can write on (e.g Output image)
    cv::Mat noCopyIplImage2MatRoi(IplImage* image, MAPSInt32 x, MAPSInt32 y, MAPSInt32 width, MAPSInt32 height);
}

#endif