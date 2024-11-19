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
// Purpose of this module : This component can carry out several type of gradients and edges calculations from the OpenCV library such as Sobel, Laplacian and Canny filters.
////////////////////////////////

#include "maps_OpenCV_GradientsAndEdges.h"  // Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_GradientsAndEdges)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_GradientsAndEdges)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_GradientsAndEdges)
    MAPS_PROPERTY_ENUM("type", "Sobel|Laplace|Canny", 2, false, false)
    MAPS_PROPERTY_ENUM("aperture_size", "3|5|7", 2, false, true)
    MAPS_PROPERTY("xorder", 1, false, true)
    MAPS_PROPERTY("yorder", 0, false, true)
    MAPS_PROPERTY("threshold1", 50, false, true)
    MAPS_PROPERTY("threshold2", 150, false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_GradientsAndEdges)
    //MAPS_ACTION("aName",MAPSOpenCV_GradientsAndEdges::ActionName)
MAPS_END_ACTIONS_DEFINITION

//V1.1: aperture size is limited to 3, 5 and 7 (no more 1).
// Use the macros to declare this component (OpenCV_GradientsAndEdges) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_GradientsAndEdges, "OpenCV_GradientsAndEdges", "2.0.3", 128,
                            MAPS::Threaded|MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                             2, // Nb of properties
                            -1) // Nb of actions


void MAPSOpenCV_GradientsAndEdges::Dynamic()
{
    m_type = static_cast<int>(GetIntegerProperty("type"));
    switch (m_type)
    {
    case 0: // Sobel
        NewProperty(2);
        NewProperty(3);
        break;
    case 1 : // Laplace
        break;
    case 2 : // Canny
        NewProperty(4);
        NewProperty(5);
        break;
    }
}

void MAPSOpenCV_GradientsAndEdges::Birth()
{
    m_convertInputToGray = false;
    m_isBGR = false;
    int aperturePropVal = static_cast<int>(GetIntegerProperty("aperture_size"));
    m_apertureSize = aperturePropVal * 2 + 3;
    m_xorder = m_yorder = m_threshold1 = m_threshold2 = 0;

    if (m_type == 0)
    {
        m_xorder = static_cast<int>(GetIntegerProperty("xorder"));
        m_yorder = static_cast<int>(GetIntegerProperty("yorder"));
    }
    else if (m_type == 2)
    {
        m_threshold1 = static_cast<int>(GetIntegerProperty("threshold1"));
        m_threshold2 = static_cast<int>(GetIntegerProperty("threshold2"));
    }

    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSOpenCV_GradientsAndEdges::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSOpenCV_GradientsAndEdges::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSOpenCV_GradientsAndEdges::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_GradientsAndEdges::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_GradientsAndEdges::Set(MAPSProperty& p, const MAPSEnumStruct& enumStruct)
{
    MAPSComponent::Set(p, enumStruct);
    if (p.ShortName() == "type")
    {
        m_type = enumStruct.selectedEnum;
    }
    else if (p.ShortName() == "aperture_size")
    {
        if (enumStruct.enumValues->Size() >= 4)
        { // value 1 is not valid. Removed. This is for backwards compatibilty when loading old diagrams.
            MAPSEnumStruct new_enum(enumStruct);
            new_enum.enumValues->Shift();
            if (new_enum.selectedEnum > 0)
                new_enum.selectedEnum--;
            Set(p, new_enum);
            return;
        }
        m_apertureSize = enumStruct.selectedEnum * 2 + 3;
    }
}

void MAPSOpenCV_GradientsAndEdges::Set(MAPSProperty& p, MAPSInt64 value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "type")
    {
        m_type = static_cast<int>(value);
    }
    else if (p.ShortName() == "aperture_size")
    {
        m_apertureSize = static_cast<int>(value * 2 + 3);
    }
    if (m_type == 0)
    {
        if (p.ShortName() == "xorder")
        {
            m_xorder = static_cast<int>(value);
        }
        else if (p.ShortName () == "yorder")
        {
            m_yorder = static_cast<int>(value);
        }
    }
    else if (m_type == 2)
    {
        if (p.ShortName() == "threshold1")
        {
            m_threshold1 = static_cast<int>(value);
        }
        else if (p.ShortName() == "threshold2")
        {
            m_threshold2 = static_cast<int>(value);
        }
    }
}

void MAPSOpenCV_GradientsAndEdges::Set(MAPSProperty& p, const MAPSString& value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "type")
    {
        m_type = GetEnumProperty("type").selectedEnum;
    }
    else if (p.ShortName() == "aperture_size")
    {
        m_apertureSize = GetEnumProperty("aperture_size").selectedEnum * 2 + 3;
    }
}

void MAPSOpenCV_GradientsAndEdges::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();
    int chanSeq = *(MAPSUInt32*)imageIn.channelSeq;
    if (chanSeq != MAPS_CHANNELSEQ_RGB && chanSeq != MAPS_CHANNELSEQ_BGR && chanSeq != MAPS_CHANNELSEQ_GRAY)
        Error("This component only accets RGB24, BGR24 and GRAY images on its input.");

    if (m_type == 0 || m_type == 1)
    {
        Output(0).AllocOutputBufferIplImage(imageIn);
    }
    else if (m_type == 2) // In the case of the function Canny the input array need to be GRAY
    {
        if (chanSeq == MAPS_CHANNELSEQ_RGB)
        {
            m_isBGR = false;
            m_convertInputToGray = true;
        }
        else if (chanSeq == MAPS_CHANNELSEQ_BGR)
        {
            m_isBGR = true;
            m_convertInputToGray = true;
        }
        IplImage model = MAPS::IplImageModel(imageIn.width, imageIn.height, MAPS_CHANNELSEQ_GRAY, imageIn.dataOrder, imageIn.depth, imageIn.align);
        Output(0).AllocOutputBufferIplImage(model);
    }
}

void MAPSOpenCV_GradientsAndEdges::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();
    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying
    m_tempImageIn = convTools::noCopyIplImage2Mat(&inElt.Data()); // Convert IplImage to cv::Mat without copying

    try
    {
        switch (m_type)
        {
        case 0: // Sobel
            cv::Sobel(m_tempImageIn, m_tempImageOut, CV_8U, m_xorder, m_yorder, m_apertureSize); // Sobel function using the threshold and aperturesize to detect the edges
            break;

        case 1: // Laplace
            cv::Laplacian(m_tempImageIn, m_tempImageOut, CV_8U, m_apertureSize); // Laplacian function using the threshold and aperturesize to detect the edges
            break;

        case 2: // Canny
            if (m_convertInputToGray) // Convert if needed
            {
                if (m_isBGR)
                    cv::cvtColor(m_tempImageIn, m_tempImageOut, cv::COLOR_BGR2GRAY); // BGR to GRAY
                else
                    cv::cvtColor(m_tempImageIn, m_tempImageOut, cv::COLOR_RGB2GRAY); // RGB to GRAY
                cv::Canny(m_tempImageOut, m_tempImageOut, m_threshold1, m_threshold2, m_apertureSize); // Canny function using the threshold and aperturesize to detect the edges
            }
            else
            {
                cv::Canny(m_tempImageIn, m_tempImageOut, m_threshold1, m_threshold2, m_apertureSize);
            }
            break;
        default:
            Error("Unknown operator type.");
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
