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
// Date: 2023
////////////////////////////////

#pragma once

// Includes maps sdk library header
#include "maps_OpenCV_Conversion.h"
#include "maps/input_reader/maps_input_reader.hpp"

// Declares a new MAPSComponent child class
class MAPSColorCorrection : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPSColorCorrection)

    void Set(MAPSProperty& p, MAPSFloat64 value) override;
private:
    void AllocateOutputBufferSize(const MAPSTimestamp /*ts*/, const MAPS::InputElt<IplImage> imageInElt);
    void ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt);

private :
    // Place here your specific methods and attributes
    cv::Mat m_tempImageIn;
    cv::Mat m_tempImageOut;
    std::vector<cv::Mat> m_vTmpSplit;
    double m_dRed, m_dGreen, m_dBlue;

    std::unique_ptr<MAPS::InputReader> m_inputReader;
};
