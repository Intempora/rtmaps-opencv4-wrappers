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
    MAPS_INPUT("roi_samp", MAPS::FilterDrawingObjects, MAPS::SamplingReader)
    MAPS_INPUT("rectCoordsPix_samp", MAPS::FilterInteger32, MAPS::SamplingReader)
    MAPS_INPUT("rectCoordsRel_samp", MAPS::FilterFloat64, MAPS::SamplingReader)
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
    MAPS_PROPERTY_ENUM("synchronization", "on images|synchronized|disabled", 0, false, false)
    MAPS_PROPERTY("sync_tolerance", 0, false, false)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_Smooth)
    //MAPS_ACTION("aName",MAPSOpenCV_Smooth::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Smooth) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_Smooth, "OpenCV_Smooth", "2.1.1", 128,
                             MAPS::Threaded|MAPS::Sequential, MAPS::Threaded,
                             1, // Nb of inputs
                            -1, // Nb of outputs
                             2, // Nb of properties
                            -1) // Nb of actions


void MAPSOpenCV_Smooth::Dynamic()
{
    m_type = static_cast<int>(GetIntegerProperty("type"));
    m_useRoiInput = static_cast<int>(GetIntegerProperty("use_ROI_input"));

    if (m_useRoiInput > 0)
    {
        m_syncMode = NewProperty("synchronization").IntegerValue();
        if(m_syncMode == 1)
            NewProperty("sync_tolerance");
    }
    else
    {
        m_syncMode = -1;
    }

    switch(m_useRoiInput)
    {
        case 1 :
            if(m_syncMode == 0)
                NewInput("roi_samp");
            else
                NewInput("roi");
            break;
        case 2 :
            if (m_syncMode == 0)
                NewInput("rectCoordsPix_samp");
            else
                NewInput("rectCoordsPix");
            break;
        case 3 :
            if (m_syncMode == 0)
                NewInput("rectCoordsRel_samp");
            else
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
    m_type = static_cast<int>(GetIntegerProperty("type"));
    switch(m_type)
    {
        case 0:
            m_param1 = static_cast<int>(GetIntegerProperty("kernel_size_x"));
            m_param2 = static_cast<int>(GetIntegerProperty("kernel_size_y"));
            break;
        case 1:
            m_param1 = static_cast<int>(GetIntegerProperty("kernel_size_x"));
            m_param2 = static_cast<int>(GetIntegerProperty("kernel_size_y"));
            m_param3 = static_cast<int>(GetFloatProperty("gaussian_sigma"));
            m_param4 = static_cast<int>(GetFloatProperty("gaussian_sigma_vert"));
            break;
        case 2:
            m_param1 = static_cast<int>(GetIntegerProperty("kernel_size"));
            break;
        case 3:
            m_param1 = static_cast<int>(GetIntegerProperty("color_sigma"));
            m_param2 = static_cast<int>(GetIntegerProperty("space_sigma"));
            break;
    }

    switch (m_syncMode)
    {
    case -1:
    case 2:
        m_inputReader = MAPS::MakeInputReader::Reactive(
            this,
            MAPS::InputReaderOption::Reactive::FirstTimeBehavior::WaitForAllInputs,
            MAPS::InputReaderOption::Reactive::Buffering::Enabled,
            m_inputs,
            &MAPSOpenCV_Smooth::AllocateOutputBufferSize,
            &MAPSOpenCV_Smooth::ProcessData
        );
        break;
    case 0:
        m_inputReader = MAPS::MakeInputReader::Triggered(
            this,
            Input(0),
            MAPS::InputReaderOption::Triggered::TriggerKind::DataInput,
            MAPS::InputReaderOption::Triggered::SamplingBehavior::AllowEmptyInputs,
            m_inputs,
            &MAPSOpenCV_Smooth::AllocateOutputBufferSize,
            &MAPSOpenCV_Smooth::ProcessDataSync
        );
        break;
    case 1:
        m_inputReader = MAPS::MakeInputReader::Synchronized(
            this,
            GetIntegerProperty("sync_tolerance"),
            MAPS::InputReaderOption::Synchronized::SyncBehavior::AllowDesyncedInputs,
            m_inputs,
            &MAPSOpenCV_Smooth::AllocateOutputBufferSize,
            &MAPSOpenCV_Smooth::ProcessDataSync
        );
        break;
    default:
        Error("Unknown sampling mode");
    }
}

void MAPSOpenCV_Smooth::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_Smooth::Death()
{
    m_vLastRois.clear();
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

void MAPSOpenCV_Smooth::ProcessDataSync(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
{
    int index = 1;
    for (auto& elt : inElts)
    {
        ProcessData(ts, index, inElts);
        index--;
    }
}

void MAPSOpenCV_Smooth::ProcessData(const MAPSTimestamp ts, const size_t inputThatAnswered, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
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
        if (m_vLastRois.empty())
        {
            m_vLastRois.reserve(inElts[inputThatAnswered].BufferSize());
        }
        if (inElts[inputThatAnswered].VectorSize() == 0)
        {
            return;
        }
        ProcessRoi(inElts[inputThatAnswered]);
    }
    break;
    }
}

void MAPSOpenCV_Smooth::ProcessIplImage(const IplImage& imageIn, IplImage& imageOut)
{
    m_tempImageIn = convTools::noCopyIplImage2Mat(&imageIn);
    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut);
    cv::Rect region(0, 0, m_width, m_height);

    std::memcpy(imageOut.imageData, imageIn.imageData, imageOut.imageSize);
    for (size_t i = 0; i < m_vLastRois.size(); i++)
    {
        int offset = m_vLastRois[i].yOffset * m_vLastRois[i].width + m_vLastRois[i].xOffset;

        if (offset >= m_width * m_height)
            Error("The defined ROI is outside the image dimension.");

        region = cv::Rect(m_vLastRois[i].xOffset, m_vLastRois[i].yOffset, m_vLastRois[i].width, m_vLastRois[i].height);

        try
        {
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
        }
        catch (const std::exception& e)
        {
            Error(e.what());
        }
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
        m_vLastRois.resize(Elt.VectorSize());
        for (size_t i = 0; i < Elt.VectorSize(); i += 1) {
            const MAPSDrawingObject& dobj = Elt.DataAs<MAPSDrawingObject>(i);
            if (dobj.kind == MAPSDrawingObject::Rectangle) {
                m_vLastRois[i].xOffset = dobj.rectangle.x1;
                m_vLastRois[i].yOffset = dobj.rectangle.y1;
                m_vLastRois[i].width = dobj.rectangle.x2 - dobj.rectangle.x1;
                m_vLastRois[i].height = dobj.rectangle.y2 - dobj.rectangle.y1;
            }
            else if (dobj.kind == MAPSDrawingObject::Ellipse)
            {
                m_vLastRois[i].xOffset = dobj.ellipse.x - dobj.ellipse.sx / 2;
                m_vLastRois[i].yOffset = dobj.ellipse.y - dobj.ellipse.sy / 2;
                m_vLastRois[i].width = dobj.ellipse.sx;
                m_vLastRois[i].height = dobj.ellipse.sy;
            }
            else if (dobj.kind == MAPSDrawingObject::Circle)
            {
                m_vLastRois[i].xOffset = dobj.circle.x - dobj.circle.radius;
                m_vLastRois[i].yOffset = dobj.circle.y - dobj.circle.radius;
                m_vLastRois[i].width = 2 * dobj.circle.radius;
                m_vLastRois[i].height = 2 * dobj.circle.radius;
            }
            else
            {
                ReportWarning("This component only accepts rectangles, circles or ellipses on its objects input.");
                m_vLastRois[i].xOffset = 0;
                m_vLastRois[i].yOffset = 0;
                m_vLastRois[i].width = 0;
                m_vLastRois[i].height = 0;
            }
        }
    }
    break;
    case 2: //Pixel coords
        if (Elt.VectorSize() < 4)
        {
            ReportWarning("The vector size for the coordinates input must be at least 4.");
            break;
        }

        m_vLastRois.resize(Elt.VectorSize() / 4);
        for (size_t i = 0; i < Elt.VectorSize(); i += 4)
        {
            m_vLastRois[i / 4].xOffset = Elt.DataAs<MAPSInt32>(i);
            m_vLastRois[i / 4].yOffset = Elt.DataAs<MAPSInt32>(i + 1);
            m_vLastRois[i / 4].width = Elt.DataAs<MAPSInt32>(i + 2);
            m_vLastRois[i / 4].height = Elt.DataAs<MAPSInt32>(i + 3);
        }
        break;
    case 3: //Relative coords
        if (Elt.VectorSize() < 4)
        {
            ReportWarning("The vector size for the coordinates input must be at least 4.");
            break;
        }

        m_vLastRois.resize(Elt.VectorSize() / 4);
        for (size_t i = 0; i < Elt.VectorSize(); i += 4)
        {
            m_vLastRois[i / 4].xOffset = static_cast<int>((Elt.DataAs<MAPSFloat64>(i) * m_width));
            m_vLastRois[i / 4].yOffset = static_cast<int>((Elt.DataAs<MAPSFloat64>(i + 1) * m_height));
            m_vLastRois[i / 4].width = static_cast<int>((Elt.DataAs<MAPSFloat64>(i + 2) * m_width));
            m_vLastRois[i / 4].height = static_cast<int>((Elt.DataAs<MAPSFloat64>(i + 3) * m_height));
        }
        break;
    }
}
