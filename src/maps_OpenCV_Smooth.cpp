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
// Purpose of this module : Smooths the image in one of several ways.
////////////////////////////////

#include "maps_OpenCV_Smooth.h" // Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_Smooth)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("roi", MAPS::FilterDrawingObjects, MAPS::FifoReader)
    MAPS_INPUT("rectCoordsPix", MAPS::FilterInteger32, MAPS::FifoReader)
    MAPS_INPUT("rectCoordsRel", MAPS::FilterFloat64, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_Smooth)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_Smooth)
    MAPS_PROPERTY_ENUM("type", "Simple blur|Gaussian blur|Median blur|Bilateral filter", 0, false, false)
    MAPS_PROPERTY_ENUM("use_ROI_input","Disabled|Bounding box objects|Rect. coordinates (in pixels)|Rect. coordinates (relative)", 0, false, false)
    MAPS_PROPERTY("kernel_size_x", 5, false, true)
    MAPS_PROPERTY("kernel_size_y", 5, false, true)
    MAPS_PROPERTY("gaussian_sigma", 0.0, false, true)
    MAPS_PROPERTY("gaussian_sigma_vert", 0.0, false, true)
    MAPS_PROPERTY("kernel_size", 5, false, true)
    MAPS_PROPERTY("color_sigma", 0, false, true)
    MAPS_PROPERTY("space_sigma", 0, false,true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_Smooth)
    //MAPS_ACTION("aName",MAPSOpenCV_Smooth::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Smooth) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_Smooth, "OpenCV_Smooth", "2.0.3", 128,
                             MAPS::Threaded|MAPS::Sequential, MAPS::Threaded,
                             1, // Nb of inputs
                            -1, // Nb of outputs
                             2, // Nb of properties
                            -1) // Nb of actions


void MAPSOpenCV_Smooth::Dynamic()
{
    m_type = static_cast<int>(GetIntegerProperty("type"));
    m_useRoiInput = static_cast<int>(GetIntegerProperty("use_ROI_input"));

    switch(m_useRoiInput)
    {
        case 1 :
            NewInput("roi");
            break;
        case 2 :
            NewInput("rectCoordsPix");
            break;
        case 3 :
            NewInput("rectCoordsRel");
            break;
    }
    switch(m_type)
    {
        case 0:
            NewProperty("kernel_size_x");
            NewProperty("kernel_size_y");
            break;
        case 1:
            NewProperty("kernel_size_x");
            NewProperty("kernel_size_y");
            NewProperty("gaussian_sigma");
            NewProperty("gaussian_sigma_vert");
            break;
        case 2:
            NewProperty("kernel_size");
            break;
        case 3:
            NewProperty("color_sigma");
            NewProperty("space_sigma");
            break;
    }

    m_inputs.clear();
    m_inputs.push_back(&Input(0));
    if (m_useRoiInput > 0)
    {
        m_inputs.push_back(&Input(1));
    }
}

void MAPSOpenCV_Smooth::Birth()
{
    m_pLastRoi = nullptr;
    m_type = static_cast<int>(GetIntegerProperty("type"));
    switch(m_type)
    {
        case 0:
            m_param1 = static_cast<int>(GetIntegerProperty(2));
            m_param2 = static_cast<int>(GetIntegerProperty(3));
            break;
        case 1:
            m_param1 = static_cast<int>(GetIntegerProperty(2));
            m_param2 = static_cast<int>(GetIntegerProperty(3));
            m_param3 = static_cast<int>(GetFloatProperty(4));
            m_param4 = static_cast<int>(GetFloatProperty(5));
            break;
        case 2:
            m_param1 = static_cast<int>(GetIntegerProperty(2));
            break;
        case 3:
            m_param1 = static_cast<int>(GetIntegerProperty(2));
            m_param2 = static_cast<int>(GetIntegerProperty(3));
            break;
    }
    
    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        MAPS::InputReaderOption::Reactive::FirstTimeBehavior::WaitForAllInputs,
        MAPS::InputReaderOption::Reactive::Buffering::Enabled,
        m_inputs,
        &MAPSOpenCV_Smooth::AllocateOutputBufferSize,
        &MAPSOpenCV_Smooth::ProcessData
    );
}

void MAPSOpenCV_Smooth::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_Smooth::Death()
{
    m_pLastRoi.reset();
    m_inputReader.reset();
}

void MAPSOpenCV_Smooth::Set(MAPSProperty& p, MAPSInt64 value)
{
    int val = static_cast<int>(value);
    if (p.ShortName() == "type")
    {
        m_type = static_cast<int>(GetIntegerProperty("type"));
        switch (m_type)
        {
        case 0:
        case 1:
            if (p.ShortName() == "kernel_size_x")
            {
                if ((val & 1) == 0)
                {
                    val++;
                    MAPSStreamedString sx;
                    sx << "Kernel dimensions must be odd. Setting horizontal kernel size to " << val;
                    ReportWarning(sx);
                }
                m_param1 = val;
            }
            else if (p.ShortName() == "kernel_size_y")
            {
                if ((val & 1) == 0)
                {
                    val++;
                    MAPSStreamedString sx;
                    sx << "Kernel dimensions must be odd. Setting vertical kernel size to " << val;
                    ReportWarning(sx);
                }
                m_param2 = val;
            }
            break;
        case 2:
            if (p.ShortName() == "kernel_size")
            {
                if ((val & 1) == 0)
                {
                    val++;
                    MAPSStreamedString sx;
                    sx << "Kernel dimensions must be odd. Setting kernel size to " << val;
                    ReportWarning(sx);
                }
                m_param1 = val;
            }
            break;
        case 3:
            if (p.ShortName() == "color_sigma")
            {
                m_param1 = val;
            }
            else if (p.ShortName() == "space_sigma")
            {
                m_param2 = val;
            }
            break;
        }
    }
    MAPSComponent::Set(p, (MAPSInt64)val);
}

void MAPSOpenCV_Smooth::Set(MAPSProperty& p, MAPSFloat64 value)
{
    m_type = static_cast<int>(GetIntegerProperty("type"));
    if (m_type == 1)
    {
        if (p.ShortName() == "gaussian_sigma")
        {
            m_param3 = value;
        }
        else if (p.ShortName() == "gaussian_sigma_vert")
        {
            m_param4 = value;
        }
    }
    MAPSComponent::Set(p, value);
}

void MAPSOpenCV_Smooth::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
{
    const IplImage& imageIn = inElts[0].DataAs<IplImage>();

    if (imageIn.nChannels != 1 && imageIn.nChannels != 3)
        Error("This component only accepts images with 1 or 3 channels.");
    if (imageIn.depth != 8)
        Error("This component only accepts 8 bits depth images.");
    Output(0).AllocOutputBufferIplImage(imageIn);
    m_width = imageIn.width;
    m_height = imageIn.height;
}

void MAPSOpenCV_Smooth::ProcessData(const MAPSTimestamp ts, const size_t inputThatAnswered, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
{
    try
    {
        switch (inputThatAnswered)
        {
        case 0:
        {
            MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };

            ProcessIplImage(inElts[inputThatAnswered].DataAs<IplImage>(), outGuard.Data());

            outGuard.VectorSize() = 0;
            outGuard.Timestamp() = ts;
        }
        break;
        case 1:
        {
            if (!m_pLastRoi)
            {
                m_pLastRoi.reset(new IplROI);
                m_pLastRoi->coi = 0;
                m_pLastRoi->xOffset = 0;
                m_pLastRoi->yOffset = 0;
                m_pLastRoi->width = 0;
                m_pLastRoi->height = 0;
            }

            if (inElts[inputThatAnswered].VectorSize() == 0)
            {
                m_pLastRoi->xOffset = 0;
                m_pLastRoi->yOffset = 0;
                m_pLastRoi->width = 0;
                m_pLastRoi->height = 0;
                return;
            }
            ProcessRoi(inElts[inputThatAnswered]);
        }
        break;
        }
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

void MAPSOpenCV_Smooth::ProcessIplImage(const IplImage& imageIn, IplImage& imageOut)
{
    m_tempImageIn = convTools::noCopyIplImage2Mat(&imageIn);
    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut);
    cv::Rect region(0, 0, m_width, m_height);

    if (m_pLastRoi)
    {
        int offset = m_pLastRoi->yOffset * m_pLastRoi->width + m_pLastRoi->xOffset;

        if (offset >= m_width * m_height)
            Error("The defined ROI is outside the image dimension.");

        region = cv::Rect(m_pLastRoi->xOffset, m_pLastRoi->yOffset, m_pLastRoi->width, m_pLastRoi->height);

        std::memcpy(imageOut.imageData, imageIn.imageData, imageOut.imageSize);
    }

    switch (m_type)
    {
    case 0:
        cv::blur(m_tempImageIn(region), m_tempImageOut(region), cv::Size(m_param1, m_param2));
        break;
    case 1:
        cv::GaussianBlur(m_tempImageIn(region), m_tempImageOut(region), cv::Size(m_param1, m_param2), m_param3, m_param4);
        break;
    case 2:
        cv::medianBlur(m_tempImageIn(region), m_tempImageOut(region), m_param1);
        break;
    case 3:
        cv::bilateralFilter(m_tempImageIn(region), m_tempImageOut(region), -1, m_param1, m_param2);
        break;
    }

    if (static_cast<void*>(m_tempImageOut.data) != static_cast<void*>(imageOut.imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
        Error("cv::Mat data ptr and imageOut data ptr are different.");
}

void MAPSOpenCV_Smooth::ProcessRoi(const MAPS::InputElt<>& Elt)
{
    switch (m_useRoiInput)
    {
    case 1: //Drawing objects
    {
        const MAPSDrawingObject& dobj = Elt.DataAs<MAPSDrawingObject>();
        if (dobj.kind == MAPSDrawingObject::Rectangle) {
            m_pLastRoi->xOffset = dobj.rectangle.x1;
            m_pLastRoi->yOffset = dobj.rectangle.y1;
            m_pLastRoi->width = dobj.rectangle.x2 - dobj.rectangle.x1;
            m_pLastRoi->height = dobj.rectangle.y2 - dobj.rectangle.y1;
        }
        else if (dobj.kind == MAPSDrawingObject::Ellipse)
        {
            m_pLastRoi->xOffset = dobj.ellipse.x - dobj.ellipse.sx / 2;
            m_pLastRoi->yOffset = dobj.ellipse.y - dobj.ellipse.sy / 2;
            m_pLastRoi->width = dobj.ellipse.sx;
            m_pLastRoi->height = dobj.ellipse.sy;
        }
        else if (dobj.kind == MAPSDrawingObject::Circle)
        {
            m_pLastRoi->xOffset = dobj.circle.x - dobj.circle.radius;
            m_pLastRoi->yOffset = dobj.circle.y - dobj.circle.radius;
            m_pLastRoi->width = 2 * dobj.circle.radius;
            m_pLastRoi->height = 2 * dobj.circle.radius;
        }
        else
        {
            ReportWarning("This component only accepts rectangles, circles or ellipses on its objects input.");
            m_pLastRoi->xOffset = 0;
            m_pLastRoi->yOffset = 0;
            m_pLastRoi->width = 0;
            m_pLastRoi->height = 0;
        }
    }
    break;
    case 2: //Pixel coords
        if (Elt.VectorSize() < 4)
            ReportWarning("The vector size for the coordinates input must be at least 4.");
        m_pLastRoi->xOffset = Elt.DataAs<MAPSInt32>(0);
        m_pLastRoi->yOffset = Elt.DataAs<MAPSInt32>(1);
        m_pLastRoi->width = Elt.DataAs<MAPSInt32>(2);
        m_pLastRoi->height = Elt.DataAs<MAPSInt32>(3);
        break;
    case 3: //Relative coords
        if (Elt.VectorSize() < 4)
            ReportWarning("The vector size for the coordinates input must be at least 4.");

        m_pLastRoi->xOffset = static_cast<int>((Elt.DataAs<MAPSFloat64>(0) * m_width));
        m_pLastRoi->yOffset = static_cast<int>((Elt.DataAs<MAPSFloat64>(1) * m_height));
        m_pLastRoi->width = static_cast<int>((Elt.DataAs<MAPSFloat64>(2) * m_width));
        m_pLastRoi->height = static_cast<int>((Elt.DataAs<MAPSFloat64>(3) * m_height));
        break;
    }
}
