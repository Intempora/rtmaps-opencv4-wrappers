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
class MAPSOpenCV_Morphology : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_HEADER_CODE_WITHOUT_CONSTRUCTOR(MAPSOpenCV_Morphology)
    MAPS_COMPONENT_DYNAMIC_HEADER_CODE(MAPSOpenCV_Morphology)
    MAPSOpenCV_Morphology(const char* name, MAPSComponentDefinition& cd);
    void Set(MAPSProperty& p, MAPSInt64 value) override;
    void Set(MAPSProperty& p, const MAPSEnumStruct& enumStruct) override;
    void Set(MAPSProperty& p, const MAPSString& value) override;

private:
    void AllocateOutputBufferSize(const MAPSTimestamp /*ts*/, const MAPS::InputElt<IplImage> imageInElt);
    void ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt);

private :
    // Place here your specific methods and attributes
    int m_operation;
    int m_shape;
    int m_cols;
    int m_rows;
    int m_anchorx;
    int m_anchory;
    int m_iterations;

    std::vector<int> m_customStructEltValues;
    MAPSString m_customStructElt;
    MAPSMutex m_convKernelMutex;

    std::unique_ptr<MAPS::InputReader> m_inputReader;

    cv::Mat m_convKernel;
    cv::Mat m_tempImageIn;
    cv::Mat m_tempImageOut;

    void UpdateConvKernel();
};
