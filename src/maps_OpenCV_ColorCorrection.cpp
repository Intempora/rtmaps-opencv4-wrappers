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

////////////////////////////////
// Purpose of this module : This component applies a gain on each channel separately to correct colors.
////////////////////////////////

#include "maps_OpenCV_ColorCorrection.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSColorCorrection)
    MAPS_INPUT("input", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSColorCorrection)
    MAPS_OUTPUT("output", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSColorCorrection)
    MAPS_PROPERTY("red", 1.0, false, true)
    MAPS_PROPERTY("green", 1.0, false, true)
    MAPS_PROPERTY("blue", 1.0, false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSColorCorrection)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component behaviour
MAPS_COMPONENT_DEFINITION(MAPSColorCorrection,"OpenCV_ColorCorrection", "1.0.1", 128,
                            MAPS::Threaded|MAPS::Sequential, MAPS::Sequential,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                            -1, // Nb of properties
                            -1) // Nb of actions


void MAPSColorCorrection::Birth()
{
    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSColorCorrection::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSColorCorrection::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
    m_dRed = GetFloatProperty("red");
    m_dGreen = GetFloatProperty("green");
    m_dBlue = GetFloatProperty("blue");
}

void MAPSColorCorrection::Core()
{
    m_inputReader->Read();
}

void MAPSColorCorrection::Death()
{
    m_inputReader.reset();
}

void MAPSColorCorrection::Set(MAPSProperty &p, MAPSFloat64 value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "red")
        m_dRed = value;
    else if (p.ShortName() == "green")
        m_dGreen = value;
    else if (p.ShortName() == "blue")
        m_dBlue = value;
}

void MAPSColorCorrection::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();
    const MAPSInt32 chanSeq = *(MAPSInt32*)imageIn.channelSeq;

    if (chanSeq != MAPS_CHANNELSEQ_BGR && chanSeq != MAPS_CHANNELSEQ_BGRA && chanSeq != MAPS_CHANNELSEQ_RGB &&
        chanSeq != MAPS_CHANNELSEQ_RGBA)
        Error("This component only accepts RGB/BGR/RGBA/BGRA images on its input.");


    // Create a new IplImage to allocate the output buffer using the channel sequence determined above
    IplImage model = MAPS::IplImageModel(imageIn.width, imageIn.height, chanSeq, imageIn.dataOrder, imageIn.depth, imageIn.align);
    Output(0).AllocOutputBufferIplImage(model);
}

void MAPSColorCorrection::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();
    const MAPSInt32 chanSeq = *(MAPSInt32*)outGuard.Data().channelSeq;

    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying
    m_tempImageIn = convTools::noCopyIplImage2Mat(&inElt.Data());

    try
    {
        cv::split(m_tempImageIn, m_vTmpSplit);
        if (chanSeq == MAPS_CHANNELSEQ_BGR || chanSeq == MAPS_CHANNELSEQ_BGRA)
        {
            m_vTmpSplit[0] *= m_dBlue;
            m_vTmpSplit[1] *= m_dGreen;
            m_vTmpSplit[2] *= m_dRed;
        }
        else
        {
            m_vTmpSplit[0] *= m_dRed;
            m_vTmpSplit[1] *= m_dGreen;
            m_vTmpSplit[2] *= m_dBlue;
        }
        cv::merge(m_vTmpSplit, m_tempImageOut);
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }

    if (static_cast<void*>(m_tempImageOut.data) != static_cast<void*>(imageOut.imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
        Error("cv::Mat data ptr and imageOut data ptr are different.");

    outGuard.VectorSize() = 0;
    outGuard.Timestamp() = ts;
}
