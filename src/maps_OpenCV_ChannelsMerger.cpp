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

////////////////////////////////
// Purpose of this module : Composes multi-channel array from several single channel arrays or inserts a single channel into the array.
////////////////////////////////

#include "maps_OpenCV_ChannelsMerger.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_ChannelsMerger)
    MAPS_INPUT("channel1", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("channel2", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("channel3", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_ChannelsMerger)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_ChannelsMerger)
    MAPS_PROPERTY("outputChannelSeq", "BGR", false, false)
    MAPS_PROPERTY("outputPlanar", false, false, false)
    MAPS_PROPERTY("synchro_tolerance", 0, false, false)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_ChannelsMerger)
    //MAPS_ACTION("aName",MAPSOpenCV_ChannelsMerger::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Resize) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_ChannelsMerger, "OpenCV_ChannelsMerger", "2.0.3", 128,
                            MAPS::Threaded|MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                            -1, // Nb of properties
                            -1) // Nb of actions


void MAPSOpenCV_ChannelsMerger::Birth()
{
    m_isOutputPlanar = GetBoolProperty("outputPlanar");
    channelSeq = GetStringProperty("outputChannelSeq");

    if (channelSeq.size() > 4)
        Error("outputChannelSeq property : Channel sequence is too long. It must be made of 4 characters max. (ex : RGB, BGR, YUV, HSV, etc...)");

    m_inputReader = MAPS::MakeInputReader::Synchronized(
        this,
        GetIntegerProperty("synchro_tolerance"),
        MAPS::InputReaderOption::Synchronized::SyncBehavior::SyncAllInputs,
        MAPS::MakeArray(&Input(0), &Input(1), &Input(2)),
        &MAPSOpenCV_ChannelsMerger::AllocateOutputBufferSize,
        &MAPSOpenCV_ChannelsMerger::ProcessData
    );
}

void MAPSOpenCV_ChannelsMerger::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_ChannelsMerger::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_ChannelsMerger::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::ArrayView<MAPS::InputElt<IplImage>> imageInElts)
{
    const IplImage& imageIn1 = imageInElts[0].Data();

    m_tempImageIn[0] = convTools::noCopyIplImage2Mat(&imageIn1); // Convert IplImage to cv::Mat
    m_tempImageIn[1] = convTools::noCopyIplImage2Mat(&imageInElts[1].Data()); // Convert IplImage to cv::Mat
    m_tempImageIn[2] = convTools::noCopyIplImage2Mat(&imageInElts[2].Data()); // Convert IplImage to cv::Mat

    if (m_tempImageIn[0].channels() != 1)
        Error("Input 1 : This component only supports single channel images on its inputs.");

    if (m_tempImageIn[1].channels() != 1)
        Error("Input 2 : This component only supports single channel images on its inputs.");

    if (m_tempImageIn[2].channels() != 1)
        Error("Input 3 : This component only supports single channel images on its inputs.");

    if (m_tempImageIn[0].size() != m_tempImageIn[1].size() ||
        m_tempImageIn[0].size() != m_tempImageIn[2].size())
        Error("Input images must have the same dimensions.");

    IplImage model = MAPS::IplImageModel(imageIn1.width, imageIn1.height, channelSeq.c_str(), m_isOutputPlanar ? IPL_DATA_ORDER_PLANE : IPL_DATA_ORDER_PIXEL, imageIn1.depth, imageIn1.align);
    Output(0).AllocOutputBufferIplImage(model);
}

void MAPSOpenCV_ChannelsMerger::ProcessData(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();

    if (m_isOutputPlanar)
    {
        size_t imageSize = inElts[0].Data().imageSize;
        for (size_t i = 0; i < inElts.size(); ++i)
        {
            std::memcpy(imageOut.imageData + i * imageSize, inElts[i].Data().imageData, imageSize);
        }
    }
    else
    {
        m_tempImageIn[0] = convTools::noCopyIplImage2Mat(&inElts[0].Data()); // Convert IplImage to cv::Mat
        m_tempImageIn[1] = convTools::noCopyIplImage2Mat(&inElts[1].Data()); // Convert IplImage to cv::Mat
        m_tempImageIn[2] = convTools::noCopyIplImage2Mat(&inElts[2].Data()); // Convert IplImage to cv::Mat
        m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying

        try{
            cv::merge(m_tempImageIn, m_tempImageOut); // Merge the 3 element of m_tempImageIn into m_tempImageOut
        }
        catch (const std::exception& e)
        {
            Error(e.what());
        }

        if (static_cast<void*>(m_tempImageOut.data) != static_cast<void*>(imageOut.imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
            Error("cv::Mat data ptr and imageOut data ptr are different.");
    }

    outGuard.VectorSize() = 0;
    outGuard.Timestamp() = ts;
}
