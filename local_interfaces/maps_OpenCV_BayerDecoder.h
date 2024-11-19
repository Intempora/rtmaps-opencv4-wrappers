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
#include "maps_OpenCV_Conversion.h"
#include "maps/input_reader/maps_input_reader.hpp"

// Declares a new MAPSComponent child class
class MAPSBayerDecoder : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPSBayerDecoder)

    void Set(MAPSProperty& p, const MAPSString& value) override;
    void Dynamic() override;

private:
    void AllocateOutputBufferIpl(const MAPSTimestamp /*ts*/, const MAPS::InputElt<IplImage> imageInElt);
    void AllocateOutputBufferMaps(const MAPSTimestamp /*ts*/, const MAPS::InputElt<MAPSImage> imageInElt);
    void ProcessDataIpl(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt);
    void ProcessDataMaps(const MAPSTimestamp ts, const MAPS::InputElt<MAPSImage> inElt);

private :
    // Place here your specific methods and attributes
    bool m_isBGR;
    int	 m_pattern;

    cv::Mat m_tempImageIn;
    cv::Mat m_tempImageOut;

    std::unique_ptr<MAPS::InputReader> m_inputReader;
};
