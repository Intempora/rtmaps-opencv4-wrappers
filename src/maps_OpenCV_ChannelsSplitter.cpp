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
// Purpose of this module : Divides 3-channel image into several single channel GRAY images.
////////////////////////////////

#include "maps_OpenCV_ChannelsSplitter.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_SplitChannels)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_SplitChannels)
    MAPS_OUTPUT("channel1", MAPS::IplImage, nullptr, nullptr, 0)
    MAPS_OUTPUT("channel2", MAPS::IplImage, nullptr, nullptr, 0)
    MAPS_OUTPUT("channel3", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_SplitChannels)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_SplitChannels)
    //MAPS_ACTION("aName",MAPSOpenCV_SplitChannels::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Resize) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_SplitChannels, "OpenCV_ChannelsSplitter", "2.0.3", 128,
                            MAPS::Threaded|MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                            -1, // Nb of properties
                            -1) // Nb of actions

void MAPSOpenCV_SplitChannels::Birth()
{
    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSOpenCV_SplitChannels::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSOpenCV_SplitChannels::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSOpenCV_SplitChannels::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_SplitChannels::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_SplitChannels::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();

    if (imageIn.nChannels != 3)
    {
        Error("This component only supports 3 channels images.");
    }

    m_isInputPlanar = (imageIn.dataOrder == IPL_DATA_ORDER_PLANE);
    IplImage model = MAPS::IplImageModel(imageIn.width, imageIn.height, MAPS_CHANNELSEQ_GRAY, imageIn.dataOrder, imageIn.depth, imageIn.align);

    Output("channel1").AllocOutputBufferIplImage(model);
    Output("channel2").AllocOutputBufferIplImage(model);
    Output("channel3").AllocOutputBufferIplImage(model);
}

void MAPSOpenCV_SplitChannels::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    const IplImage& imageIn = inElt.Data();
    MAPS::OutputGuard<IplImage> outGuard1{ this, Output("channel1")};
    MAPS::OutputGuard<IplImage> outGuard2{ this, Output("channel2")};
    MAPS::OutputGuard<IplImage> outGuard3{ this, Output("channel3")};

    IplImage& imageOut1 = outGuard1.Data();
    IplImage& imageOut2 = outGuard2.Data();
    IplImage& imageOut3 = outGuard3.Data();
    if (m_isInputPlanar)
    {
        std::memcpy(imageOut1.imageData, imageIn.imageData, imageOut1.imageSize);
        std::memcpy(imageOut2.imageData, imageIn.imageData + imageOut2.imageSize, imageOut2.imageSize);
        std::memcpy(imageOut3.imageData, imageIn.imageData + 2 * imageOut3.imageSize, imageOut3.imageSize);
    }
    else
    {
        m_tempImageOut[0] = convTools::noCopyIplImage2Mat(&imageOut1);
        m_tempImageOut[1] = convTools::noCopyIplImage2Mat(&imageOut2);
        m_tempImageOut[2] = convTools::noCopyIplImage2Mat(&imageOut3);

        try
        {
            cv::Mat tempImageIn = convTools::noCopyIplImage2Mat(&imageIn); // Take a cv::Mat reference to input image
            cv::split(tempImageIn, m_tempImageOut);
        }
        catch (const std::exception& e)
        {
            Error(e.what());
        }

        if (static_cast<void*>(m_tempImageOut[0].data) != static_cast<void*>(imageOut1.imageData) ||
            static_cast<void*>(m_tempImageOut[1].data) != static_cast<void*>(imageOut2.imageData) ||
            static_cast<void*>(m_tempImageOut[2].data) != static_cast<void*>(imageOut3.imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
            Error("cv::Mat data ptr and imageOut data ptr are different.");
    }
    outGuard1.VectorSize() = 0;
    outGuard1.Timestamp() = ts;
    outGuard2.VectorSize() = 0;
    outGuard2.Timestamp() = ts;
    outGuard3.VectorSize() = 0;
    outGuard3.Timestamp() = ts;
}
