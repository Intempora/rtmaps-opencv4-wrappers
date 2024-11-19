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
// Purpose of this module : This component can perform some standard Morphological operations such as Erode, Dilate, Open, Close, etc...
////////////////////////////////

#include "maps_OpenCV_Morphology.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_Morphology)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_Morphology)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_Morphology)
    MAPS_PROPERTY_ENUM("operation", "Erode|Dilate|Open|Close|Gradient|TopHat|BlackHat|HitMiss", 0, false, true)
    MAPS_PROPERTY_ENUM("structuring_element_shape", "Rectangle|Cross|Ellipse|Custom", 0, false, true)
    MAPS_PROPERTY("structuring_element_cols", 3, false, true)
    MAPS_PROPERTY("structuring_element_rows", 3, false, true)
    MAPS_PROPERTY("structuring_element_anchor_x", 1, false, true)
    MAPS_PROPERTY("structuring_element_anchor_y", 1, false, true)
    MAPS_PROPERTY("iterations", 1, false, true)
    MAPS_PROPERTY("custom_structuring_element", "[0,1,0;1,1,1;0,1,0]", false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_Morphology)
    //MAPS_ACTION("aName",MAPSOpenCV_Morphology::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Resize) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_Morphology, "OpenCV_Morphology", "2.0.3", 128,
                            MAPS::Threaded|MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                             7, // Nb of properties
                            -1) // Nb of actions

MAPSOpenCV_Morphology::MAPSOpenCV_Morphology(const char* name, MAPSComponentDefinition& cd)
:MAPSComponent(name, cd)
{
    m_operation = 0;
    m_shape = 0;
    m_cols = 3;
    m_rows = 3;
    m_anchorx = 1;
    m_anchory = 1;
    m_iterations = 1;
    m_customStructElt = "";
    UpdateConvKernel();
}

void MAPSOpenCV_Morphology::Dynamic()
{
    m_shape = static_cast<int>(GetIntegerProperty("structuring_element_shape"));
    if (m_shape == 3) //Custom
        NewProperty("custom_structuring_element");

}

void MAPSOpenCV_Morphology::Birth()
{
    m_operation = static_cast<int>(GetIntegerProperty("operation"));
    m_shape = static_cast<int>(GetIntegerProperty("structuring_element_shape"));
    m_cols = static_cast<int>(GetIntegerProperty("structuring_element_cols"));
    m_rows = static_cast<int>(GetIntegerProperty("structuring_element_rows"));
    m_anchorx = static_cast<int>(GetIntegerProperty("structuring_element_anchor_x"));
    m_anchory = static_cast<int>(GetIntegerProperty("structuring_element_anchor_y"));
    m_iterations = static_cast<int>(GetIntegerProperty("iterations"));
    if (m_shape == 3) //custom
        m_customStructElt = GetStringProperty("custom_structuring_element");
    if (m_anchorx >= m_cols || m_anchory >= m_rows)
        Error("Unable to create the structuring element : anchor_x and anchor_y have to be respectively less than cols and rows.");
    UpdateConvKernel();

    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSOpenCV_Morphology::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSOpenCV_Morphology::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSOpenCV_Morphology::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_Morphology::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_Morphology::UpdateConvKernel() // Update the cv::Mat m_convKernel to use a rectangle, cross or ellipse as structuring element
{
    try
    {
        if (!MAPS::IsRunning())
            return;
        if (m_anchorx >= m_cols || m_anchory >= m_rows)
        {
            ReportError("Unable to create the structuring element : anchor_x and anchor_y have to be respectively less than cols and rows.");
            return;
        }
        m_convKernelMutex.Lock();
        switch (m_shape)
        {
            case 0: // RECTANGLE
                m_convKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(m_cols, m_rows), cv::Point(m_anchorx, m_anchory));
                break;
            case 1: // CROSS
                m_convKernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(m_cols, m_rows), cv::Point(m_anchorx, m_anchory));
                break;
            case 2: // ELLIPSE
                m_convKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(m_cols, m_rows), cv::Point(m_anchorx, m_anchory));
                break;
            case 3: //Custom structuring element.
            {
                //Decode the structuring element.
                if (!m_convKernel.empty())
                {
                    m_convKernel = cv::Mat();
                }
                m_convKernel = cv::Mat(m_rows, m_cols, CV_8U);
                char* strStructElt = const_cast<char*>(static_cast<const char*>(m_customStructElt));
                int length = m_customStructElt.Len();
                int col = 0, row = 0;
                if (length < 5)
                {
                    ReportError("Bad syntax for the custom structuring element definition.");
                    m_convKernel = cv::Mat();
                }
                else
                {
                    int index = 0;
                    bool syntaxOk = true;
                    while (index < length && row < m_rows && col < m_cols)
                    {
                        char c = strStructElt[index];
                        if (index == 0)
                        {
                            if (c != '[')
                            {
                                syntaxOk = false;
                                break;
                            }
                            index++;
                        }
                        else
                        {
                            if (c == ',')
                            {
                                col++;
                                index++;
                            }
                            else if (c == ';')
                            {
                                col = 0;
                                row++;
                                index++;
                            }
                            else if (c == ']')
                            {
                                if (col != m_cols - 1 && row != m_rows - 1)
                                {
                                    syntaxOk = false;
                                    break;
                                }
                                break;
                            }
                            else
                            {
                                if (!isdigit(strStructElt[index]))
                                {
                                    syntaxOk = false;
                                    break;
                                }
                                m_convKernel.at<int>(row, col) = strStructElt[index];
                                index++;
                                while (strStructElt[index] != ',' && strStructElt[index] != ';' && strStructElt[index] != ']')
                                {
                                    index++;
                                }
                            }
                        }
                    }
                    if (syntaxOk == false)
                    {
                        m_convKernel = cv::Mat();
                        ReportError("Bad syntax for the custom structuring element string.");
                    }
                }
                if (m_convKernel.empty())
                {
                    m_convKernel = cv::Mat(m_rows, m_cols, CV_8U);
                    for (int i = 0; i < m_rows; i++)
                    {
                        for (int j = 0; j < m_cols; j++)
                            m_convKernel.at<int>(i, j) = 1;
                    }
                }
                break;
            }
        }
        m_convKernelMutex.Release();
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

void MAPSOpenCV_Morphology::Set(MAPSProperty& p, MAPSInt64 value)
{
    if (p.ShortName() == "operation")
    {
        m_operation = static_cast<int>(value);
    }
    else if (p.ShortName() == "structuring_element_shape")
    {
        m_shape = static_cast<int>(value);
        UpdateConvKernel();
    }
    else if (p.ShortName() == "structuring_element_cols")
    {
        m_cols = static_cast<int>(value);
        UpdateConvKernel();
    }
    else if (p.ShortName() == "structuring_element_rows")
    {
        m_rows = static_cast<int>(value);
        UpdateConvKernel();
    }
    else if (p.ShortName() == "structuring_element_anchor_x")
    {
        m_anchorx = static_cast<int>(value);
        UpdateConvKernel();
    }
    else if (p.ShortName() == "structuring_element_anchor_y")
    {
        m_anchory = static_cast<int>(value);
        UpdateConvKernel();
    }
    else if (p.ShortName() == "iterations")
    {
        m_iterations = static_cast<int>(value);
    }
    MAPSComponent::Set(p, value);
}

void MAPSOpenCV_Morphology::Set(MAPSProperty& p, const MAPSString& value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "operation")
    {
        m_operation = GetEnumProperty("operation").selectedEnum;
    }
    else if (p.ShortName() == "structuring_element_shape")
    {
        m_shape = GetEnumProperty("structuring_element_shape").selectedEnum;
        UpdateConvKernel();
    }
    else if (m_shape == 3)  //custom
    {
        if (p.ShortName() == "custom_structuring_element")
        {
            m_customStructElt = GetStringProperty("custom_structuring_element");
        }
        UpdateConvKernel();
    }
}

void MAPSOpenCV_Morphology::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    Output(0).AllocOutputBufferIplImage(imageInElt.Data());
}

void MAPSOpenCV_Morphology::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();

    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying
    m_tempImageIn = convTools::noCopyIplImage2Mat(&inElt.Data());

    try
    {
        m_convKernelMutex.Lock();
        switch (m_operation) // Use different opencv function depending on the selected one
        {
        case 0: // Erode
            cv::erode(m_tempImageIn, m_tempImageOut, m_convKernel, cv::Size(m_anchorx, m_anchory), m_iterations);
            break;
        case 1: // Dilate
            cv::dilate(m_tempImageIn, m_tempImageOut, m_convKernel, cv::Size(m_anchorx, m_anchory), m_iterations);
            break;
        case 2: // OPEN
            cv::morphologyEx(m_tempImageIn, m_tempImageOut, cv::MORPH_OPEN, m_convKernel, cv::Size(m_anchorx, m_anchory), m_iterations);
            break;
        case 3: // CLOSE
            cv::morphologyEx(m_tempImageIn, m_tempImageOut, cv::MORPH_CLOSE, m_convKernel, cv::Size(m_anchorx, m_anchory), m_iterations);
            break;
        case 4: // GRADIENT
            cv::morphologyEx(m_tempImageIn, m_tempImageOut, cv::MORPH_GRADIENT, m_convKernel, cv::Size(m_anchorx, m_anchory), m_iterations);
            break;
        case 5: // TOPHAT
            cv::morphologyEx(m_tempImageIn, m_tempImageOut, cv::MORPH_TOPHAT, m_convKernel, cv::Size(m_anchorx, m_anchory), m_iterations);
            break;
        case 6: // BLACKHAT
            cv::morphologyEx(m_tempImageIn, m_tempImageOut, cv::MORPH_BLACKHAT, m_convKernel, cv::Size(m_anchorx, m_anchory), m_iterations);
            break;
        case 7: // HITMISS
            cv::morphologyEx(m_tempImageIn, m_tempImageOut, cv::MORPH_HITMISS, m_convKernel, cv::Size(m_anchorx, m_anchory), m_iterations);
            break;
        }
        m_convKernelMutex.Release();
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

void MAPSOpenCV_Morphology::Set(MAPSProperty& p, const MAPSEnumStruct& enumStruct)
{
    MAPSComponent::Set(p, enumStruct);
    if (p.ShortName() == "operation")
    {
        m_operation = enumStruct.selectedEnum;
    }
    else if (p.ShortName() == "structuring_element_shape")
    {
        m_shape = enumStruct.selectedEnum;
        UpdateConvKernel();
    }
}