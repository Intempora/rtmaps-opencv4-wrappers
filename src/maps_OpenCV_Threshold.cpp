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
// Purpose of this module : Applies fixed-level or adaptive threshold to images
////////////////////////////////

#include "maps_OpenCV_Threshold.h" // Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_Threshold)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_Threshold)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_Threshold)
    MAPS_PROPERTY_ENUM("mode", "Fixed level|Adpative", 0, false, false)
    MAPS_PROPERTY("threshold", 128, false, true)
    MAPS_PROPERTY("max_value", 255, false, true)
    MAPS_PROPERTY_ENUM("fixed_level_type", "Binary|Binary inverted|Truncate|To zero|To zero inverted", 0, false, true)
    MAPS_PROPERTY_ENUM("adaptive_type", "Binary|Binary inverted", 0, false, true)
    MAPS_PROPERTY_ENUM("adaptive_method", "Mean|Gaussian", 0, false, true)
    MAPS_PROPERTY("block_size", 3, false, true)
    MAPS_PROPERTY("param1", 5, false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_Threshold)
    //MAPS_ACTION("aName",MAPSOpenCV_Threshold::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Resize) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_Threshold, "OpenCV_Threshold", "2.0.3", 128,
                             MAPS::Threaded|MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                             1, // Nb of properties
                            -1) // Nb of actions


void MAPSOpenCV_Threshold::Dynamic()
{
    m_mode = static_cast<int>(GetIntegerProperty("mode"));
    if (m_mode == 1)  //Adaptive
    {
        NewProperty("max_value");
        NewProperty("adaptive_type", "type");
        NewProperty("adaptive_method");
        NewProperty("block_size");
        NewProperty("param1");
    }
    else
    {
        NewProperty("threshold");
        NewProperty("max_value");
        NewProperty("fixed_level_type", "type");
    }
}

void MAPSOpenCV_Threshold::Birth()
{
    m_maxValue = static_cast<int>(GetIntegerProperty("max_value"));
    UpdateType(static_cast<int>(GetIntegerProperty("type")));
    if (m_mode == 0)
    {
        m_threshold = static_cast<int>(GetIntegerProperty("threshold"));
    }
    else
    {
        UpdateAdaptiveMethod(static_cast<int>(GetIntegerProperty("adaptive_method")));
        m_blockSize = static_cast<int>(GetIntegerProperty("block_size"));
        m_param1 = static_cast<int>(GetIntegerProperty("param1"));
    }

    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSOpenCV_Threshold::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSOpenCV_Threshold::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSOpenCV_Threshold::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_Threshold::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_Threshold::Set(MAPSProperty& p, MAPSInt64 value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "max_value")
    {
        m_maxValue = static_cast<int>(value);
    }
    else if (p.ShortName() == "type")
    {
        UpdateType(static_cast<int>(value));
    }
    else if (m_mode == 0)  //fixed level
    {
        if (p.ShortName() == "threshold")
        {
            m_threshold = static_cast<int>(value);
        }
    }
    else
    {
        if (p.ShortName() == "adaptive_method")
        {
            UpdateAdaptiveMethod(static_cast<int>(value));
        }
        else if (p.ShortName() == "block_size")
        {
            m_blockSize = static_cast<int>(value);
        }
        else if (p.ShortName() == "param1")
        {
            m_param1 = static_cast<int>(value);
        }
    }
}

void MAPSOpenCV_Threshold::Set(MAPSProperty& p, const MAPSString& value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "type")
    {
        UpdateType(GetEnumProperty("type").selectedEnum);
    }
    else if (m_mode == 1)
    {
        if (p.ShortName() == "adaptive_method")
        {
            UpdateAdaptiveMethod(GetEnumProperty("adaptive_method").selectedEnum);
        }
    }
}

void MAPSOpenCV_Threshold::Set(MAPSProperty& p, const MAPSEnumStruct& enumStruct)
{
    MAPSComponent::Set(p, enumStruct);
    if (p.ShortName() == "type")
    {
        UpdateType(enumStruct.selectedEnum);
    }
    else if (m_mode == 1)
    {
        if (p.ShortName() == "adaptive_method")
        {
            UpdateAdaptiveMethod(enumStruct.selectedEnum);
        }
    }
}

void MAPSOpenCV_Threshold::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();
    Output(0).AllocOutputBufferIplImage(imageIn);
    m_nChans = imageIn.nChannels;
    if (m_nChans > 1)
    {
        m_planesImages.resize(m_nChans);
    }
}

void MAPSOpenCV_Threshold::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    try
    {
        const IplImage& imageIn = inElt.Data();
        m_image = convTools::noCopyIplImage2Mat(&imageIn); // Convert IplImage to cv::Mat

        MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
        IplImage& imageOut = outGuard.Data();
        m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying

        if (m_nChans == 1)
        {
            if (m_mode == 0)
                cv::threshold(m_image, m_tempImageOut, m_threshold, m_maxValue, m_type);
            else
                cv::adaptiveThreshold(m_image, m_tempImageOut, m_maxValue, m_adaptiveMethod, m_type, m_blockSize, m_param1);
        }
        else
        {
            cv::split(m_image, m_planesImages);
            for (int i = 0; i < imageIn.nChannels; i++)
            {
                if (m_mode == 0)
                    cv::threshold(m_planesImages[i], m_planesImages[i], m_threshold, m_maxValue, m_type);
                else
                    cv::adaptiveThreshold(m_planesImages[i], m_planesImages[i], m_maxValue, m_adaptiveMethod, m_type, m_blockSize, m_param1);

            }
            cv::merge(m_planesImages, m_tempImageOut);
        }

        if (static_cast<void*>(m_tempImageOut.data) != static_cast<void*>(imageOut.imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
            Error("cv::Mat data ptr and imageOut data ptr are different.");

        outGuard.VectorSize() = 0;
        outGuard.Timestamp() = ts;
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

void MAPSOpenCV_Threshold::UpdateType(int val)
{
    switch (val)
    {
        case 0:
            m_type = cv::THRESH_BINARY;
            break;
        case 1:
            m_type = cv::THRESH_BINARY_INV;
            break;
        case 2:
            m_type = cv::THRESH_TRUNC;
            break;
        case 3:
            m_type = cv::THRESH_TOZERO;
            break;
        case 4:
            m_type = cv::THRESH_TOZERO_INV;
            break;
    }
}

void MAPSOpenCV_Threshold::UpdateAdaptiveMethod(int val)
{
    switch (val)
    {
        case 0:
            m_adaptiveMethod = cv::ADAPTIVE_THRESH_MEAN_C;
            break;
        case 1:
            m_adaptiveMethod = cv::ADAPTIVE_THRESH_GAUSSIAN_C;
            break;
    }
}
