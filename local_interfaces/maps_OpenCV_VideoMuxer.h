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

typedef struct SortingElt
{
	int index;
	int zorder;
} SortingElt;

// Declares a new MAPSComponent child class
class MAPSOpenCV_VideoMuxer : public MAPSComponent
{
    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPSOpenCV_VideoMuxer)
    MAPS_COMPONENT_DYNAMIC_HEADER_CODE(MAPSOpenCV_VideoMuxer)
    void Set(MAPSProperty& p, MAPSInt64 value) override;

private:
    void Initialization(const MAPSTimestamp /*ts*/, const MAPS::ArrayView <MAPS::InputElt<IplImage>> inElts);
    void Initialization_Reactive(const MAPSTimestamp /*ts*/, const size_t inputThatAnswered, const MAPS::ArrayView <MAPS::InputElt<IplImage>> inElts);
    void Initialization_Trigerred(const MAPSTimestamp /*ts*/, const MAPS::ArrayView <MAPS::InputElt<IplImage>> inElts);
    void ProcessData(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts);
    void ProcessData_Reactive(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts);

    void ComputeSizesAndAllocOutBuffer();
    void OutputResultImage(MAPSTimestamp t, const MAPS::ArrayView<MAPS::InputElt<IplImage>>& inElts);
    void SortIOEltsByZOrder();

    void BackgroundInitialization(IplImage* imageOut);

private :
    // Place here your specific methods and attributes
    int m_firstPositionPropRuntime;
    bool m_firstTimeAllInit;
    int m_nbInputs;
    int m_readersMode;
    bool m_allSizesInitialized;
    int m_trigger;
    int m_totalmWidth; // output image width (before eventual resize)
    int m_totalmHeight;
    int m_outmWidth; // final output image width (after eventual resize)
    int m_outmHeight;
    bool m_outNeedResize;
    bool m_outputInitialized;
    unsigned int m_dataOrder;
    unsigned int m_depth;
    unsigned int m_align;

    std::vector<bool> m_sizeInitialized;
    std::vector<int> m_width;
    std::vector<int> m_height;
    std::vector<int> m_posX;
    std::vector<int> m_posY;
    std::vector<int> m_zOrder;
    std::vector<int> m_ioeltsOrder;

    std::vector<MAPSInput*> m_inputs;
    std::vector<void*> m_bgInitialized;

    std::mutex m_zorderMutex;
    MAPSUInt32 m_chanSeq;
    IplImage m_tempImage;
    IplROI m_tempROI;
    std::vector<char> m_tempImageData;
    MAPSArray<MAPSIOElt*> m_ioEltImage;
    cv::Mat m_black;

    std::unique_ptr<MAPS::InputReader> m_inputReader;
};
