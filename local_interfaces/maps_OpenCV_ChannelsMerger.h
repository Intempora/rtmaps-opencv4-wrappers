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
class MAPSOpenCV_ChannelsMerger : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPSOpenCV_ChannelsMerger)

private:
    void AllocateOutputBufferSize(const MAPSTimestamp /*ts*/, const MAPS::ArrayView<MAPS::InputElt<IplImage>> imageInElts);
    void ProcessData(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts);

private :
    // Place here your specific methods and attributes
    bool m_isOutputPlanar;
    std::string channelSeq;

    std::array<cv::Mat, 3> m_tempImageIn;
    cv::Mat m_tempImageOut;
    std::unique_ptr<MAPS::InputReader> m_inputReader;
};
