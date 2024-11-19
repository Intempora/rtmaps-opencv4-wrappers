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
// Purpose of this module : Converts image from one color space to another.
////////////////////////////////

#include "maps_OpenCV_ColorSpaceConverter.h"    // Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSColorSpaceConverter)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSColorSpaceConverter)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSColorSpaceConverter)
    MAPS_PROPERTY_ENUM("input_colorspace", "RGB 24|BGR 24|YUV 24|HSV|GRAY|RGBA 32|BGRA 32|AUTO", 6, false, false)
    MAPS_PROPERTY_ENUM("output_colorspace", "RGB 24|BGR 24|YUV 24|HSV|GRAY|RGBA 32|BGRA 32", 1, false, false)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSColorSpaceConverter)
    //MAPS_ACTION("aName",MAPSColorSpaceConverter::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (ColorDemux_YUV) behaviour
MAPS_COMPONENT_DEFINITION(MAPSColorSpaceConverter,"OpenCV_ColorSpaceConverter", "2.0.3", 128,
                            MAPS::Threaded|MAPS::Sequential, MAPS::Sequential,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                            -1, // Nb of properties
                            -1) // Nb of actions


void MAPSColorSpaceConverter::Dynamic()
{
    m_inputCS = static_cast<int>(GetIntegerProperty("input_colorspace"));
    m_outputCS = static_cast<int>(GetIntegerProperty("output_colorspace"));
}

void MAPSColorSpaceConverter::Birth()
{
    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSColorSpaceConverter::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSColorSpaceConverter::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSColorSpaceConverter::Core()
{
    m_inputReader->Read();
}

void MAPSColorSpaceConverter::Death()
{
    m_inputReader.reset();
}

void MAPSColorSpaceConverter::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();

    if (imageIn.dataOrder != IPL_DATA_ORDER_PIXEL)
        Error("This component only supports pixel oriented images on its input.");

    int inputChanSeq = *(MAPSUInt32*)imageIn.channelSeq;
    CheckInputColorSpace(inputChanSeq);
    IplImage model;
    switch (m_outputCS) // Depending on the output colorspace that is wanted, m_openCVConvertCode will take different value corresponding to the opencv flag
    {
    case CS_GRAY: // GRAY
        if (m_inputCS == CS_RGB24)
            m_openCVConvertCode = cv::COLOR_RGB2GRAY;
        else if (m_inputCS == CS_BGR24)
            m_openCVConvertCode = cv::COLOR_BGR2GRAY;
        else
            Error("Cannot convert the input image format to GRAY. Only RGB to GRAY and BGR to GRAY are supported.");

        model = MAPS::IplImageModel(imageIn.width, imageIn.height, "GRAY", imageIn.dataOrder, imageIn.depth, imageIn.align);
        break;

    case CS_RGB24: //RGB
        if (m_inputCS == CS_GRAY)
            m_openCVConvertCode = cv::COLOR_GRAY2RGB;
        else if (m_inputCS == CS_YUV24)
            m_openCVConvertCode = cv::COLOR_YCrCb2RGB;
        else if (m_inputCS == CS_HSV)
            m_openCVConvertCode = cv::COLOR_HSV2RGB;
        else if (m_inputCS == CS_RGBA)
            m_openCVConvertCode = cv::COLOR_RGBA2RGB;
        else if (m_inputCS == CS_BGR24)
            m_openCVConvertCode = cv::COLOR_BGR2RGB;
        else
            Error("Cannot convert the input image format to RGB24. Only GRAY to RGB, YUV 24 to RGB and HSV to RGB transformations are supported.");

        model = MAPS::IplImageModel(imageIn.width, imageIn.height, MAPS_CHANNELSEQ_RGB, imageIn.dataOrder, imageIn.depth, imageIn.align);
        break;

    case CS_BGR24: // BGR
        if (m_inputCS == CS_GRAY)
            m_openCVConvertCode = cv::COLOR_GRAY2BGR;
        else if (m_inputCS == CS_YUV24)
            m_openCVConvertCode = cv::COLOR_YCrCb2BGR;
        else if (m_inputCS == CS_HSV)
            m_openCVConvertCode = cv::COLOR_HSV2BGR;
        else if (m_inputCS == CS_BGRA)
            m_openCVConvertCode = cv::COLOR_BGRA2BGR;
        else if (m_inputCS == CS_RGBA)
            m_openCVConvertCode = cv::COLOR_RGBA2BGR;
        else if (m_inputCS == CS_RGB24)
            m_openCVConvertCode = cv::COLOR_RGB2BGR;
        else
            Error("Cannot convert the input image to BRG24. Only GRAY to BGR, YUV 24 to BGR and HSV to BGR transformations are supported.");

        model = MAPS::IplImageModel(imageIn.width, imageIn.height, MAPS_CHANNELSEQ_BGR, imageIn.dataOrder, imageIn.depth, imageIn.align);
        break;

    case CS_YUV24: // YUV
        if (m_inputCS == CS_RGB24)
            m_openCVConvertCode = cv::COLOR_RGB2YCrCb;
        else if (m_inputCS == CS_BGR24)
            m_openCVConvertCode = cv::COLOR_BGR2YCrCb;
        else
            Error("Cannot convert the input image format to YUV24. Only RGB to YUV and BGR to YUV transformations are supported.");

        model = MAPS::IplImageModel(imageIn.width, imageIn.height, MAPS_CHANNELSEQ_YUV, imageIn.dataOrder, imageIn.depth, imageIn.align);
        break;

    case CS_HSV: // HSV
        if (m_inputCS == CS_RGB24)
            m_openCVConvertCode = cv::COLOR_RGB2HSV;
        else if (m_inputCS == CS_BGR24)
            m_openCVConvertCode = cv::COLOR_BGR2HSV;
        else
            Error("Cannot convert the input image format to HSV. Only RGB to HSV and BGR to HSV transformations are supported.");

        model = MAPS::IplImageModel(imageIn.width, imageIn.height, MAPS_FC('H', 'S', 'V', 000), imageIn.dataOrder, imageIn.depth, imageIn.align);
        break;

    case CS_RGBA: // RGBA
        if (m_inputCS == CS_RGB24)
            m_openCVConvertCode = cv::COLOR_RGB2RGBA;
        else if (m_inputCS == CS_BGR24)
            m_openCVConvertCode = cv::COLOR_BGR2RGBA;
        else if (m_inputCS == CS_GRAY)
            m_openCVConvertCode = cv::COLOR_GRAY2RGBA;
        else
            Error("Conversion not supported. Ask Intempora.");

        model = MAPS::IplImageModel(imageIn.width, imageIn.height, MAPS_CHANNELSEQ_RGBA, imageIn.dataOrder, imageIn.depth, imageIn.align);
        break;

    case CS_BGRA: // BGRA
        if (m_inputCS == CS_RGB24)
            m_openCVConvertCode = cv::COLOR_RGB2BGRA;
        else if (m_inputCS == CS_BGR24)
            m_openCVConvertCode = cv::COLOR_BGR2BGRA;
        else if (m_inputCS == CS_GRAY)
            m_openCVConvertCode = cv::COLOR_GRAY2BGRA;
        else
            Error("Conversion not supported.");

        model = MAPS::IplImageModel(imageIn.width, imageIn.height, MAPS_CHANNELSEQ_BGRA, imageIn.dataOrder, imageIn.depth, imageIn.align);
        break;

    default:
        Error("Unsupported image format on input. This component can only deal with GRAY, RGB, BGR, YUV and HSV images.");
    }
    Output(0).AllocOutputBufferIplImage(model);
}

void MAPSColorSpaceConverter::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();

    cv::Mat matIn = convTools::noCopyIplImage2Mat(&inElt.Data());
    cv::Mat matOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying

    try{
        // OpenCV uses YCrCb and RTMaps uses YCbCr
        if (m_inputCS == CS_YUV24)
        {
            cv::split(matIn, m_tempChannels);
            std::swap(m_tempChannels[1], m_tempChannels[2]);
            cv::merge(m_tempChannels, m_workImage);
            // Convert matIn in another color depending on the colorspace wanted (m_openCVConvertCode), and finally store the new color image in component output
            cv::cvtColor(m_workImage, matOut, m_openCVConvertCode);
        }
        else if (m_outputCS == CS_YUV24)
        {
            cv::cvtColor(matIn, m_workImage, m_openCVConvertCode);
            cv::split(m_workImage, m_tempChannels);
            std::swap(m_tempChannels[1], m_tempChannels[2]);
            cv::merge(m_tempChannels, matOut);
        }
        else
        {
            cv::cvtColor(matIn, matOut, m_openCVConvertCode);
        }
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }

    if (static_cast<void*>(matOut.data) != static_cast<void*>(imageOut.imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
        Error("cv::Mat data ptr and imageOut data ptr are different.");

    outGuard.VectorSize() = 0;
    outGuard.Timestamp() = ts;
}

void MAPSColorSpaceConverter::CheckInputColorSpace(int chanSeq)
{
    if (m_inputCS == CS_AUTO)
    { //AUTO
        switch (chanSeq)
        {
            case MAPS_CHANNELSEQ_GRAY:
                m_inputCS = CS_GRAY;
                break;
            case MAPS_CHANNELSEQ_RGBA:
                m_inputCS = CS_RGBA;
                break;
            case MAPS_CHANNELSEQ_BGRA:
                m_inputCS = CS_BGRA;
                break;
            case MAPS_CHANNELSEQ_BGR:
                m_inputCS = CS_BGR24;
                break;
            case MAPS_CHANNELSEQ_RGB:
                m_inputCS = CS_RGB24;
                break;
            case MAPS_FC('H', 'S', 'V', 000):
                m_inputCS = CS_HSV;
                break;
            case MAPS_CHANNELSEQ_YUV :
                m_inputCS = CS_YUV24;
                break;
            default :
                Error("Unsupported image format on input. This component only supports RGB24, BGR24, YUV24, HSV and GRAY images.");
        }
    }
    else if (m_inputCS == CS_GRAY)
    {
        if (chanSeq != MAPS_CHANNELSEQ_GRAY)
            Error("This component expects GRAY images on its input. See the inputColorSpace property.");
    }
    else if (m_inputCS == CS_RGB24)
    {
        if (chanSeq != MAPS_CHANNELSEQ_RGB)
            Error("This component expects RGB24 images on its input. See the inputColorSpace property.");
    }
    else if (m_inputCS == CS_BGR24)
    {
        if (chanSeq != MAPS_CHANNELSEQ_BGR)
            Error("This component expects BGR24 images on its input. See the inputColorSpace property.");
    }
    else if (m_inputCS == CS_YUV24)
    {
        if (chanSeq != MAPS_CHANNELSEQ_YUV)
            Error("This component expects YUV images on its input. See the inputColorSpace property.");
    }
    else if (m_inputCS == CS_HSV)
    {
        if (chanSeq != MAPS_FC('H', 'S', 'V', 000))
            Error("This component expects HSV images on its input. See the inputColorSpace property.");
    }
}

void MAPSColorSpaceConverter::Set(MAPSProperty& p, const MAPSString& value)
{
    if (p.ShortName() == "input_colorspace")
    {
        if (MAPSEnumStruct::IsEnumString(value))
        {
            MAPSEnumStruct enum_prop;
            enum_prop.FromString(value);
            if (enum_prop.enumValues->Size() < 8)
            {
                //certainly we are trying to load a diagram saved with a previous version (<1.10) of this component.
                //colorspaces RGBA and BGRA were not supported yet, so the content of the "input_colorspace" prop was different.
                //Let's not erase the new content but just update the selected index.
                if (enum_prop.selectedEnum == 5)
                {
                    Set(p, "AUTO");
                }
                else
                {
                    MAPSComponent::Set(p, (MAPSInt64)enum_prop.selectedEnum);
                }
                return;
            }
        }
    }
    else if (p.ShortName() == "output_colorspace")
    {
        if (MAPSEnumStruct::IsEnumString(value))
        {
            MAPSEnumStruct enum_prop;
            enum_prop.FromString(value);
            if (enum_prop.enumValues->Size() < 7)
            {
                //certainly we are trying to load a diagram saved with a previous version (<1.10) of this component.
                //colorspaces RGBA and BGRA were not supported yet, so the content of the "input_colorspace" prop was different.
                //Let's not erase the new content but just update the selected index.
                MAPSComponent::Set(p, (MAPSInt64)enum_prop.selectedEnum);
                return;
            }
        }
    }
    MAPSComponent::Set(p, value);
}

void MAPSColorSpaceConverter::Set(MAPSProperty& p, const MAPSEnumStruct& enum_prop)
{
    if (p.ShortName() == "input_colorspace")
    {
        if (enum_prop.enumValues->Size() < 8)
        {
            //certainly we are trying to load a diagram saved with a previous version (<1.10) of this component.
            //colorspaces RGBA and BGRA were not supported yet, so the content of the "input_colorspace" prop was different.
            //Let's not erase the new content but just update the selected index.
            if (enum_prop.selectedEnum == 5)
            {
                Set(p, "AUTO");
            }
            else
            {
                MAPSComponent::Set(p, (MAPSInt64)enum_prop.selectedEnum);
            }
            return;
        }
    }
    else if (p.ShortName() == "output_colorspace")
    {
        if (enum_prop.enumValues->Size() < 7)
        {
            //certainly we are trying to load a diagram saved with a previous version (<1.10) of this component.
            //colorspaces RGBA and BGRA were not supported yet, so the content of the "input_colorspace" prop was different.
            //Let's not erase the new content but just update the selected index.
            MAPSComponent::Set(p, (MAPSInt64)enum_prop.selectedEnum);
            return;
        }
    }
    MAPSComponent::Set(p, enum_prop);
}
