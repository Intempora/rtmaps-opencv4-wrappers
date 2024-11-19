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

////////////////////////////////
// Author: Intempora S.A. - NL
// Date: 2022
////////////////////////////////

#pragma once

// Includes maps sdk library header
#include "maps/input_reader/maps_input_reader.hpp"
#include "maps_OpenCV_Conversion.h"

#define NB_LABEL_COLORS 19
static const int s_label_colors[NB_LABEL_COLORS][3] =
{
{0,0,255},
{0,255,255},
{0,255,0},
{255,0,0},
{255,255,0},
{255,0,255},
{255,255,255},
{0,0,0},
{0,0,128},
{0,128,128},
{0,128,0},
{128,0,0},
{128,128,0},
{128,0,128},
{128,128,128},
{0,128,255},
{0,255,255},
{128,255,0},
{255,128,0}
};

// Declares a new MAPSComponent child class
class MAPSOpenCV_Yolo : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPSOpenCV_Yolo)

private:
    void AllocateOutputBufferSize(const MAPSTimestamp /*ts*/, const MAPS::InputElt<IplImage> imageInElt);
    void ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt);
    void DrawLabel(cv::Mat& input_image, std::string label, int left, int top);

private:
	std::unique_ptr<MAPS::InputReader> m_inputReader;
    cv::dnn::DetectionModel m_model;
    std::vector<std::string> m_classes;
};
