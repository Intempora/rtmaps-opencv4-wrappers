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
class MAPSOpenCV_Smooth : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPSOpenCV_Smooth)
    MAPS_COMPONENT_DYNAMIC_HEADER_CODE(MAPSOpenCV_Smooth)
    void Set(MAPSProperty& p, MAPSInt64 value) override;
    void Set(MAPSProperty& p, MAPSFloat64 value) override;

private:
    void AllocateOutputBufferSize(const MAPSTimestamp /*ts*/, const MAPS::ArrayView <MAPS::InputElt<>> inElts);
    void ProcessData(const MAPSTimestamp ts, const size_t inputThatAnswered, const MAPS::ArrayView<MAPS::InputElt<>> inElts);
    void ProcessDataSync(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<>> inElts);
    void ProcessIplImage(const IplImage& imageIn, IplImage& imageOut);
    void ProcessRoi(const MAPS::InputElt<>& Elt);

private :
    // Place here your specific methods and attributes
    int m_useRoiInput;
    std::vector<MAPSInput*> m_inputs;
    int m_type;
    int m_param1;
    int m_param2;
    MAPSFloat64 m_param3;
    MAPSFloat64 m_param4;
    std::vector<IplROI> m_vLastRois;

    int m_width;
    int m_height;
    cv::Mat m_tempImageIn;
    cv::Mat m_tempImageOut;
    int m_syncMode;

    std::unique_ptr<MAPS::InputReader> m_inputReader;
};
