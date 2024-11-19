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
// Purpose of this module : Take an image in input, resize and output it.
////////////////////////////////

#include "maps_OpenCV_Resize.h"	// Includes the header of this component
#include "maps_io_access.hpp"

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_Resize)
MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_Resize)
MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_Resize)
MAPS_PROPERTY("new_size_x", 320, false, false)
MAPS_PROPERTY("new_size_y", 240, false, false)
MAPS_PROPERTY_ENUM("interpolation", "Nearest Neighbor|Bilinear|Bicubic|Area|Lanczos|Linear Exact", 1, false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_Resize)
    //MAPS_ACTION("aName",MAPSOpenCV_Resize::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Resize) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_Resize, "OpenCV_Resize", "2.1.1", 128,
                            MAPS::Threaded | MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                            3, // Nb of properties
                            -1) // Nb of actions

void MAPSOpenCV_Resize::Birth()
{
    m_firsttime = true;

    m_newSize = cv::Size(static_cast<int>(GetIntegerProperty("new_size_x")), static_cast<int>(GetIntegerProperty("new_size_y")));
    UpdateInterp(GetIntegerProperty("interpolation"));
   
    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSOpenCV_Resize::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSOpenCV_Resize::ProcessData      // Called when data is received for the first time AND all subsequent times
    );


}

void MAPSOpenCV_Resize::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_Resize::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_Resize::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();
    IplImage model = MAPS::IplImageModel(m_newSize.width, m_newSize.height, imageIn.channelSeq, imageIn.dataOrder, imageIn.depth, imageIn.align);
    Output(0).AllocOutputBufferIplImage(model);
}

void MAPSOpenCV_Resize::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    try
    {
        MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
        IplImage& imageOut = outGuard.Data();


        cv::Mat tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying
        cv::Mat tempImageIn = convTools::noCopyIplImage2Mat(&inElt.Data());

        // Create a new image using the data of the input image, the new size and the interpollation method choose
        cv::resize(tempImageIn, tempImageOut, m_newSize, 0, 0, m_method);

        outGuard.VectorSize() = 0;
        outGuard.Timestamp() = ts;
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

void MAPSOpenCV_Resize::UpdateInterp(MAPSInt64 selectedEnum)
{
    switch (selectedEnum)
    {
        case 0: // Nearest Neighbor
            m_method = cv::INTER_NEAREST;
            break;
        case 1: // Bilinear
            m_method = cv::INTER_LINEAR;
            break;
        case 2: // Bicubic
            m_method = cv::INTER_CUBIC;
            break;
        case 3: // Area
            m_method = cv::INTER_AREA;
            break;
        case 4: // Lanczos
            m_method = cv::INTER_LANCZOS4;
            break;
        case 5: // Linear Exact
            m_method = cv::INTER_LINEAR_EXACT;
            break;
        default:
            Error("Unknown interpolation method.");
    }
}

void MAPSOpenCV_Resize::Set(MAPSProperty& p, MAPSInt64 value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "interpolation")
    {
        UpdateInterp(value);
    }
}

void MAPSOpenCV_Resize::Set(MAPSProperty& p, const MAPSString& value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "interpolation")
    {
        UpdateInterp(GetEnumProperty("interpolation").selectedEnum);
    }
}

void MAPSOpenCV_Resize::Set(MAPSProperty& p, const MAPSEnumStruct& enumStruct)
{
    MAPSComponent::Set(p, enumStruct);
    if (p.ShortName() == "interpolation")
    {
        UpdateInterp(enumStruct.selectedEnum);
    }
}