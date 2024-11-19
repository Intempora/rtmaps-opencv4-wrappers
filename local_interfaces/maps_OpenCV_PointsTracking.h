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
// Date: 2019
////////////////////////////////

#pragma once

// Includes maps sdk library header
#include "maps/input_reader/maps_input_reader.hpp"
#include "maps_OpenCV_Conversion.h"

// Declares a new MAPSComponent child class
class MAPSOpenCV_PointsTracking : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPSOpenCV_PointsTracking)
    MAPS_COMPONENT_DYNAMIC_HEADER_CODE(MAPSOpenCV_PointsTracking)
    void Set(MAPSProperty& p, MAPSInt64 value) override;

private:
    void OutputCurrentPoints(MAPSTimestamp t);
    void OutputCurrentObjects(MAPSTimestamp t);
    void OutputPreviousPoints(MAPSTimestamp t);

    static void AutoInitAction(MAPSModule* module, int actionNb);
    static void RemoveAllPointsAction(MAPSModule* module, int actionNb);

    void ProcessData(const MAPSTimestamp ts, const size_t inputThatAnswered, const MAPS::ArrayView<MAPS::InputElt<>> inElts);
    void ProcessIplImage(const IplImage& image, const MAPSTimestamp& t);
    void ProcessPoints(const size_t vectorSize, const MAPSInt32* dataPtr, const MAPSTimestamp& ts);

private :
    // Place here your specific methods and attributes
    bool m_useAutoInitInput;
    int m_maxNbPoints;
    int m_lineWidth;
    int m_color;

    int m_nbInputs;
    std::vector<MAPSInput*> m_inputs;
    std::unique_ptr<MAPS::InputReader> m_inputReader;

    int m_nbPoints2Track;
    bool m_needAutoInit;
    std::array<std::vector<cv::Point2f>, 2> m_points;
    int m_flags;

    cv::Mat m_status;
    cv::Mat m_gray;
    cv::Mat m_prevGray;
    std::vector<cv::Point2f> m_swapPoints;
    cv::Mat m_image;
};
