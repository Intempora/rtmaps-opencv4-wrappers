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

#include "maps/input_reader/maps_input_reader.hpp"
#include "maps_OpenCV_Conversion.h"

// Declares a new MAPSComponent child class
class MAPScvOverlay : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPScvOverlay)
    MAPS_COMPONENT_DYNAMIC_HEADER_CODE(MAPScvOverlay)

    void Set(MAPSProperty &p, MAPSInt64 value) override;
    void Set(MAPSProperty &p, const MAPSString& value) override;
    void Set(MAPSProperty &p, const MAPSEnumStruct& enumStruct) override;

private:
    void AllocateOutputBufferSize(const MAPSTimestamp /*ts*/, const MAPS::ArrayView<MAPS::InputElt<>> inElts);
    void ProcessData(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<>> inElts);
    void updateFontFace(MAPSInt64 index);
    void overlayShape(cv::Mat output, MAPSDrawingObject& todraw);
    cv::Scalar setColorOverlay(int r, int g, int b);

private :
    // Place here your specific methods and attributes
    int m_readersMode;

    int	m_fontFace;
    int	m_nbInputs;

    std::vector<MAPSInput*> m_inputs;
    const char* m_chanSeq;

    MAPSArray<MAPSArray<MAPSDrawingObject>> m_shapes;
    std::unique_ptr<MAPS::InputReader> m_inputReader;
};
