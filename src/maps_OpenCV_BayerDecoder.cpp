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
// Purpose of this module : Bayer pattern is widely used in CCD and CMOS cameras. It allows to get color picture
//                          out of a single plane where R, G and B pixels(sensors of a particular component) are interleaved.
////////////////////////////////

#include "maps_OpenCV_BayerDecoder.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSBayerDecoder)
MAPS_INPUT("input_ipl", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_INPUT("input_maps", MAPS::FilterMAPSImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSBayerDecoder)
MAPS_OUTPUT("output", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSBayerDecoder)
    MAPS_PROPERTY_ENUM("input_type", "IPLImage|MAPSImage", 0, false, true)
    MAPS_PROPERTY_ENUM("input_pattern", "BG|GB|RG|GR", 0, false, true)
    MAPS_PROPERTY_ENUM("outputFormat", "BGR|RGB", 0, false, false)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSBayerDecoder)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (ColorConvert_Bayer2RGB) behaviour
MAPS_COMPONENT_DEFINITION(MAPSBayerDecoder,"OpenCV_BayerDecoder", "2.1.2", 128,
                            MAPS::Threaded|MAPS::Sequential, MAPS::Sequential,
                            0, // Nb of inputs
                            -1, // Nb of outputs
                            -1, // Nb of properties
                            -1) // Nb of actions

enum MAPS_BAYER_PATTERN : uint8_t
{
    MAPS_BAYER_PATTERN_BG,
    MAPS_BAYER_PATTERN_GB,
    MAPS_BAYER_PATTERN_RG,
    MAPS_BAYER_PATTERN_GR
};

void MAPSBayerDecoder::Birth()
{
    m_isBGR = (GetIntegerProperty("outputFormat") == 0);
    m_pattern = static_cast<MAPS_BAYER_PATTERN>(GetEnumProperty("input_pattern").GetSelected());

    if (GetIntegerProperty("input_type") == 0)
    {
        m_inputReader = MAPS::MakeInputReader::Reactive(
            this,
            Input(0),
            &MAPSBayerDecoder::AllocateOutputBufferIpl,  // Called when data is received for the first time only
            &MAPSBayerDecoder::ProcessDataIpl      // Called when data is received for the first time AND all subsequent times
        );
    }
    else
    {
        m_inputReader = MAPS::MakeInputReader::Reactive(
            this,
            Input(0),
            &MAPSBayerDecoder::AllocateOutputBufferMaps,  // Called when data is received for the first time only
            &MAPSBayerDecoder::ProcessDataMaps     // Called when data is received for the first time AND all subsequent times
        );
    }
}

void MAPSBayerDecoder::Dynamic()
{
    if (GetIntegerProperty("input_type") == 0)
    {
        NewInput("input_ipl");
    }
    else
    {
        NewInput("input_maps");
    }
}

void MAPSBayerDecoder::Core()
{
    m_inputReader->Read();
}

void MAPSBayerDecoder::Death()
{
    m_inputReader.reset();
}

void MAPSBayerDecoder::Set(MAPSProperty& p, const MAPSString& value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "input_pattern")
    {
        m_pattern = static_cast<MAPS_BAYER_PATTERN>(GetEnumProperty("input_pattern").GetSelected());
    }
}

void MAPSBayerDecoder::AllocateOutputBufferIpl(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();

    if (*(MAPSInt32*)imageIn.channelSeq != MAPS_CHANNELSEQ_GRAY)
        Error("This component only accepts GRAY images on its input (8 bpp or 16bpp).");

    int outputChanSeq;
    if (m_isBGR)
    {
        outputChanSeq = MAPS_CHANNELSEQ_BGR;
    }
    else
    {
        outputChanSeq = MAPS_CHANNELSEQ_RGB;
    }
    // Create a new IplImage to allocate the output buffer using the channel sequence determined above
    IplImage model = MAPS::IplImageModel(imageIn.width, imageIn.height, outputChanSeq, imageIn.dataOrder, imageIn.depth, imageIn.align);
    Output(0).AllocOutputBufferIplImage(model);
}

void MAPSBayerDecoder::AllocateOutputBufferMaps(const MAPSTimestamp, const MAPS::InputElt<MAPSImage> imageInElt)
{
    const MAPSImage& imageIn = imageInElt.Data();

    int outputChanSeq;
    if (m_isBGR)
    {
        outputChanSeq = MAPS_CHANNELSEQ_BGR;
    }
    else
    {
        outputChanSeq = MAPS_CHANNELSEQ_RGB;
    }

    MAPSUInt32 fourcc = 0;
    MAPS::Memcpy((char*)&fourcc, (const char*)imageIn.imageCoding, 4);
    MAPSInt32 depth = IPL_DEPTH_8U;
    switch (fourcc)
    {
    case MAPS_IMAGECODING_RGGB:
    case MAPS_IMAGECODING_GRBG:
    case MAPS_IMAGECODING_GBRG:
    case MAPS_IMAGECODING_BA81:
        depth = IPL_DEPTH_8U;
        break;
    case MAPS_IMAGECODING_RG10:
    case MAPS_IMAGECODING_BA10:
    case MAPS_IMAGECODING_GB10:
    case MAPS_IMAGECODING_BG10:
    case MAPS_IMAGECODING_RG12:
    case MAPS_IMAGECODING_BA12:
    case MAPS_IMAGECODING_GB12:
    case MAPS_IMAGECODING_BG12:
    case MAPS_IMAGECODING_RG16:
    case MAPS_IMAGECODING_GR16:
    case MAPS_IMAGECODING_GB16:
    case MAPS_IMAGECODING_BYR2:
    {
        depth = IPL_DEPTH_16U;
    }
    break;
    default:
        Error("Image coding not supported");
    }

    // Create a new IplImage to allocate the output buffer using the channel sequence determined above
    IplImage model = MAPS::IplImageModel(imageIn.width, imageIn.height, outputChanSeq, IPL_DATA_ORDER_PIXEL, depth, IPL_ALIGN_QWORD);
    Output(0).AllocOutputBufferIplImage(model);
}

void MAPSBayerDecoder::ProcessDataIpl(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();

    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying
    m_tempImageIn = convTools::noCopyIplImage2Mat(&inElt.Data());

    try {
        // Convert an image from one color space to another depending on the pattern use
        switch (m_pattern)
        {
        case MAPS_BAYER_PATTERN_BG:
            cv::cvtColor(m_tempImageIn, m_tempImageOut, m_isBGR ? cv::COLOR_BayerBG2BGR : cv::COLOR_BayerBG2RGB);
            break;
        case MAPS_BAYER_PATTERN_GB:
            cv::cvtColor(m_tempImageIn, m_tempImageOut, m_isBGR ? cv::COLOR_BayerGB2BGR : cv::COLOR_BayerGB2RGB);
            break;
        case MAPS_BAYER_PATTERN_RG:
            cv::cvtColor(m_tempImageIn, m_tempImageOut, m_isBGR ? cv::COLOR_BayerRG2BGR : cv::COLOR_BayerRG2RGB);
            break;
        case MAPS_BAYER_PATTERN_GR:
            cv::cvtColor(m_tempImageIn, m_tempImageOut, m_isBGR ? cv::COLOR_BayerGR2BGR : cv::COLOR_BayerGR2RGB);
            break;
        }
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

void MAPSBayerDecoder::ProcessDataMaps(const MAPSTimestamp ts, const MAPS::InputElt<MAPSImage> inElt)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();

    const MAPSImage& imageIn = inElt.Data();

    MAPSUInt32 fourcc = 0;
    MAPS::Memcpy((char*)&fourcc, (const char*)imageIn.imageCoding, 4);
    switch (fourcc)
    {
    case MAPS_IMAGECODING_RGGB:
    case MAPS_IMAGECODING_GRBG:
    case MAPS_IMAGECODING_GBRG:
    case MAPS_IMAGECODING_BA81:
    {
        m_tempImageIn = cv::Mat(imageIn.height, imageIn.width, CV_8UC1, imageIn.imageData);
    }
    break;
    case MAPS_IMAGECODING_RG10:
    case MAPS_IMAGECODING_BA10:
    case MAPS_IMAGECODING_GB10:
    case MAPS_IMAGECODING_BG10:
    case MAPS_IMAGECODING_RG12:
    case MAPS_IMAGECODING_BA12:
    case MAPS_IMAGECODING_GB12:
    case MAPS_IMAGECODING_BG12:
    case MAPS_IMAGECODING_RG16:
    case MAPS_IMAGECODING_GR16:
    case MAPS_IMAGECODING_GB16:
    case MAPS_IMAGECODING_BYR2:
    {
        m_tempImageIn = cv::Mat(imageIn.height, imageIn.width, CV_16UC1, imageIn.imageData);
    }
    break;
    default:
        Error("Image coding not supported");
    }

    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying

    try {
        // Convert an image from one color space to another depending on the pattern use
        switch (m_pattern)
        {
        case MAPS_BAYER_PATTERN_BG:
            cv::cvtColor(m_tempImageIn, m_tempImageOut, m_isBGR ? cv::COLOR_BayerBG2BGR : cv::COLOR_BayerBG2RGB);
            break;
        case MAPS_BAYER_PATTERN_GB:
            cv::cvtColor(m_tempImageIn, m_tempImageOut, m_isBGR ? cv::COLOR_BayerGB2BGR : cv::COLOR_BayerGB2RGB);
            break;
        case MAPS_BAYER_PATTERN_RG:
            cv::cvtColor(m_tempImageIn, m_tempImageOut, m_isBGR ? cv::COLOR_BayerRG2BGR : cv::COLOR_BayerRG2RGB);
            break;
        case MAPS_BAYER_PATTERN_GR:
            cv::cvtColor(m_tempImageIn, m_tempImageOut, m_isBGR ? cv::COLOR_BayerGR2BGR : cv::COLOR_BayerGR2RGB);
            break;
        }
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