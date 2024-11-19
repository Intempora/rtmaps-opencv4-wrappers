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
// Purpose of this module : This component is similar to the Overlay Drawing component available in section "Viewers" but the result might look
//                          nicer (particularly for text objects where this component supports some fonts).
////////////////////////////////

#include "maps_OpenCV_Overlay.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPScvOverlay)
    MAPS_INPUT("images", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("shape_fifo", MAPS::FilterDrawingObjects, MAPS::FifoReader)
    MAPS_INPUT("shape_sampling", MAPS::FilterDrawingObjects, MAPS::SamplingReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPScvOverlay)
    MAPS_OUTPUT("output", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPScvOverlay)
    MAPS_PROPERTY("nbOfInputs", 1, false, false)
    MAPS_PROPERTY_ENUM("synchronization", "on images|synchronized", 0, false, false)
    MAPS_PROPERTY("synchrotolerance", 0, false, false)
    MAPS_PROPERTY("thickness", 1, false, true)
    MAPS_PROPERTY("drawbkg", false, false, true)
    MAPS_PROPERTY_ENUM("font", "FONT_HERSHEY_SIMPLEX|FONT_HERSHEY_PLAIN|FONT_HERSHEY_DUPLEX|FONT_HERSHEY_COMPLEX|FONT_HERSHEY_TRIPLEX|FONT_HERSHEY_COMPLEX_SMALL|FONT_HERSHEY_SCRIPT_SIMPLEX|FONT_HERSHEY_SCRIPT_COMPLEX", 0, false, true)
    MAPS_PROPERTY("italic", false, false, true)
    MAPS_PROPERTY("fill_shape", false, false, false)
    MAPS_PROPERTY("override_color", false, false, false)
    MAPS_PROPERTY_SUBTYPE("color", MAPS_RGB(0xFF, 0xFF, 0xFF), false, true, MAPS::PropertySubTypeColor)
MAPS_END_PROPERTIES_DEFINITION

enum PROPERTY : uint8_t
{
    PROPERTY_NB_INPUTS,
    PROPERTY_READER_MODE,
    PROPERTY_SYNCHRO_TOL,
    PROPERTY_THICKNESS,
    PROPERTY_DRAWBKG,
    PROPERTY_FONT,
    PROPERTY_ITALIC,
    PROPERTY_FILL_SHAPE,
    PROPERTY_OVERRIDE_COLOR,
    PROPERTY_COLOR
};

enum InputReaderMode : uint8_t
{
    InputReaderMode_TriggeredByImage,
    InputReaderMode_Synchronized
};

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPScvOverlay)
    //MAPS_ACTION("aName",MAPScvOverlay::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (cvOverlay) behaviour
MAPS_COMPONENT_DEFINITION(MAPScvOverlay, "OpenCV_Overlay", "2.0.3", 128,
                            MAPS::Threaded, MAPS::Threaded,
                             1, // Nb of inputs
                            -1, // Nb of outputs
                             9, // Nb of properties
                            -1) // Nb of actions

void MAPScvOverlay::Dynamic()
{
    m_readersMode = static_cast<int>(GetIntegerProperty(PROPERTY_READER_MODE));
    m_nbInputs = static_cast<int>(GetIntegerProperty(PROPERTY_NB_INPUTS));
    m_inputs.clear();
    m_inputs.push_back(&Input(0));
    m_shapes.SetSize(m_nbInputs);

    for (int i = 0; i != m_nbInputs; i++)
    {
        MAPSStreamedString name;
        name << "shape" << i;
        MAPSInput* input = &NewInput(m_readersMode == InputReaderMode_TriggeredByImage ? 2 : 1, name);
        m_inputs.push_back(input);
    }

    if (GetBoolProperty(PROPERTY_OVERRIDE_COLOR))
    {
        NewProperty("color");
    }
}

void MAPScvOverlay::Birth()
{
    updateFontFace(GetIntegerProperty(PROPERTY_FONT));

    switch (m_readersMode)
    {
    case InputReaderMode_TriggeredByImage:
        m_inputReader = MAPS::MakeInputReader::Triggered(
            this,
            Input(0),
            MAPS::InputReaderOption::Triggered::TriggerKind::DataInput,
            MAPS::InputReaderOption::Triggered::SamplingBehavior::AllowEmptyInputs,
            m_inputs,
            &MAPScvOverlay::AllocateOutputBufferSize,
            &MAPScvOverlay::ProcessData
        );
        break;
    case InputReaderMode_Synchronized:
        m_inputReader = MAPS::MakeInputReader::Synchronized(
            this,
            GetIntegerProperty(PROPERTY_SYNCHRO_TOL),
            MAPS::InputReaderOption::Synchronized::SyncBehavior::AllowDesyncedInputs,
            m_inputs,
            &MAPScvOverlay::AllocateOutputBufferSize,
            &MAPScvOverlay::ProcessData
        );
        break;
    default:
        Error("Unknown sampling mode");
    }
}

void MAPScvOverlay::Core()
{
    m_inputReader->Read();
}

void MAPScvOverlay::Death()
{
    // Clean shapes from previous run
    for (int i = 0; i != m_nbInputs; i++)
    {
        m_shapes[i].Clear();
    }
    
    m_inputReader.reset();
}

void MAPScvOverlay::Set(MAPSProperty &p, MAPSInt64 value)
{
    MAPSComponent::Set(p,value);
    if (&p == &Property(PROPERTY_FONT))
        updateFontFace(value);
}

//... selecting the desired string ...
void MAPScvOverlay::Set(MAPSProperty &p, const MAPSString& value)
{
    MAPSComponent::Set(p,value);
    if (&p == &Property(PROPERTY_FONT))
    {
        MAPSString	val;

        if (MAPSEnumStruct::IsEnumString(value))
        {
            MAPSEnumStruct	enumStruct;

            enumStruct.FromString(value, false);
            updateFontFace(enumStruct.GetSelected());
        }
        else
        {
            if (value == "FONT_HERSHEY_SIMPLEX")
                updateFontFace(0);
            else if (value == "FONT_HERSHEY_PLAIN")
                updateFontFace(1);
            else if (value == "FONT_HERSHEY_DUPLEX")
                updateFontFace(2);
            else if (value == "FONT_HERSHEY_COMPLEX")
                updateFontFace(3);
            else if (value == "FONT_HERSHEY_TRIPLEX")
                updateFontFace(4);
            else if (value == "FONT_HERSHEY_COMPLEX_SMALL")
                updateFontFace(5);
            else if (value == "FONT_HERSHEY_SCRIPT_SIMPLEX")
                updateFontFace(6);
            else if (value == "FONT_HERSHEY_SCRIPT_COMPLEX")
                updateFontFace(7);
            else
            {
                MAPSStreamedString	sx;
                sx << "Trying to use an unknown font: " << value;
                ReportWarning(sx);
            }
        }
    }
}

void MAPScvOverlay::Set(MAPSProperty &p, const MAPSEnumStruct& enumStruct)
{
    MAPSComponent::Set(p, enumStruct);
    if (&p == &Property(PROPERTY_FONT))
        updateFontFace(enumStruct.GetSelected());
}

void MAPScvOverlay::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
{
    Output(0).AllocOutputBufferIplImage(inElts[0].DataAs<IplImage>());
}

void MAPScvOverlay::ProcessData(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
{
    const IplImage& imageIn = inElts[0].DataAs<IplImage>();
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();

    m_chanSeq = imageIn.channelSeq;

    const int inputCount = static_cast<int>(inElts.size());

    for (int i = 1; i < inputCount; ++i)
    {
        if (inElts[i].IsValid())
        {
            int vectorSize = static_cast<int>(inElts[i].VectorSize());
            m_shapes[i].SetSize(vectorSize);
            for (int j = 0; j < vectorSize; ++j)
            {
                m_shapes[i](j) = inElts[i].DataAs<MAPSDrawingObject>(j);
            }
        }
    }

    std::memcpy(imageOut.imageData, imageIn.imageData, imageIn.imageSize);

    try
    {
        for (int i = 0; i != m_nbInputs; ++i)
        {
            MAPSArray<MAPSDrawingObject>& shape = m_shapes[i];
            for (int j = 0; j < shape.Size(); ++j)
            {
                overlayShape(convTools::noCopyIplImage2Mat(&imageOut), shape(j)); // Convert IplImage to cv::Mat without copying
            }
        }
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }

    outGuard.VectorSize() = 0;
    outGuard.Timestamp() = ts;
}

void MAPScvOverlay::updateFontFace(MAPSInt64 index)
{
    switch (index)
    {
    case 0: // SIMPLEX
        m_fontFace = cv::FONT_HERSHEY_SIMPLEX;
        break;

    case 1: // PLAIN
        m_fontFace = cv::FONT_HERSHEY_PLAIN;
        break;

    case 2: // DUPLEX
        m_fontFace = cv::FONT_HERSHEY_DUPLEX;
        break;

    case 3: // COMPLEX
        m_fontFace = cv::FONT_HERSHEY_COMPLEX;
        break;

    case 4: // TRIPLEX
        m_fontFace = cv::FONT_HERSHEY_TRIPLEX;
        break;

    case 5: // COMPLEX SMALL
        m_fontFace = cv::FONT_HERSHEY_COMPLEX_SMALL;
        break;

    case 6: // SCRIPT SIMPLEX
        m_fontFace = cv::FONT_HERSHEY_SCRIPT_SIMPLEX;
        break;

    case 7: // SCRIPT COMPLEX
        m_fontFace = cv::FONT_HERSHEY_SCRIPT_COMPLEX;
        break;

    default:
        ReportWarning("Unknown font specified");
        return;
    }
}

cv::Scalar MAPScvOverlay::setColorOverlay(int r, int g, int b)
{
    switch (*(const MAPSUInt32*)(m_chanSeq))
    {
        case MAPS_CHANNELSEQ_RGB:
        case MAPS_CHANNELSEQ_RGBA:
            return cv::Scalar(r, g, b);
            break;

        case MAPS_CHANNELSEQ_BGR:
        case MAPS_CHANNELSEQ_BGRA:
            return cv::Scalar(b, g, r);
            break;

        case MAPS_CHANNELSEQ_YUV:
        case MAPS_CHANNELSEQ_YUVA:
        {
            MAPSInt32 y = static_cast<MAPSInt32>((0.257 * r) + (0.504 * g) + (0.098 * b) + 16);
            MAPSInt32 u = static_cast<MAPSInt32>(-(0.148 * r) - (0.291 * g) + (0.439 * b) + 128);
            MAPSInt32 v = static_cast<MAPSInt32>((0.439 * r) - (0.368 * g) - (0.071 * b) + 128);
            return cv::Scalar(y, u, v);
        }
        break;

        case MAPS_CHANNELSEQ_GRAY:
        {
            MAPSInt32 luminosity = static_cast<MAPSInt32>(0.21 * r + 0.72 * g + 0.07 * b);
            return cv::Scalar(luminosity);
        }
        break;
        default:
            return 0;
    }
}

void MAPScvOverlay::overlayShape(cv::Mat output, MAPSDrawingObject& todraw)
{
    MAPSInt32 r, g, b;
    if (GetBoolProperty(PROPERTY_OVERRIDE_COLOR))
    {
        const MAPSInt32 color_i32 = MAPSInt32(GetIntegerProperty(PROPERTY_COLOR));
        r = MAPS_RGB_EXTRACT_R(color_i32);
        g = MAPS_RGB_EXTRACT_G(color_i32);
        b = MAPS_RGB_EXTRACT_B(color_i32);
    }
    else
    {
        r = MAPS_RGB_EXTRACT_R(todraw.color);
        g = MAPS_RGB_EXTRACT_G(todraw.color);
        b = MAPS_RGB_EXTRACT_B(todraw.color);
    }

    cv::Scalar color = setColorOverlay(r, g, b);

    MAPSInt32 thickness = GetBoolProperty(PROPERTY_FILL_SHAPE)
        ? MAPSInt32(cv::FILLED)
        : MAPSInt32(GetIntegerProperty(PROPERTY_THICKNESS));

    // Draw the corresponding structure depending on the kind of object is in inputs
    if (MAPSDrawingObject::Circle == todraw.kind)
    {
        MAPSCircle circle = todraw.circle;
        cv::circle(output, cv::Point(circle.x, circle.y), circle.radius, color, thickness); // Circle
    }
    else if (MAPSDrawingObject::Ellipse == todraw.kind)
    {
        MAPSEllipse	el = todraw.ellipse;
        cv::ellipse(output, cv::Point(el.x, el.y), cv::Size(el.sx, el.sy), 0, 0, 360, color, thickness); // Ellipse
    }
    else if (MAPSDrawingObject::Line == todraw.kind)
    {
        MAPSLine line = todraw.line;
        cv::line(output, cv::Point(line.x1, line.y1), cv::Point(line.x2, line.y2), color, thickness); // Line
    }
    else if (MAPSDrawingObject::Rectangle == todraw.kind)
    {
        MAPSRectangle rect = todraw.rectangle;
        cv::rectangle(output, cv::Point(rect.x1, rect.y1), cv::Point(rect.x2, rect.y2), color, thickness); // Rectangle
    }
    else if (MAPSDrawingObject::Spot == todraw.kind) // Spot
    {
        MAPSSpot spot = todraw.spot;

        if (MAPSSpot::Point == spot.kind)
            cv::circle(output, cv::Point(spot.x, spot.y), todraw.width, color, -1);
        else if (MAPSSpot::Circle == spot.kind)
            cv::circle(output, cv::Point(spot.x, spot.y), todraw.width, color, thickness);
        else if (MAPSSpot::Cross == spot.kind)
        {
            MAPSInt32	cw = MAX(1, todraw.width / 2);
            cv::line(output, cv::Point(spot.x - cw, spot.y - cw), cv::Point(spot.x + cw, spot.y + cw), color, thickness);
            cv::line(output, cv::Point(spot.x - cw, spot.y + cw), cv::Point(spot.x + cw, spot.y - cw), color, thickness);
        }
        else if (MAPSSpot::CircledPoint == spot.kind)
        {
            cv::circle(output, cv::Point(spot.x, spot.y), thickness, color, -1);
            cv::circle(output, cv::Point(spot.x, spot.y), MAX(3, todraw.width), color, thickness);
        }
        else if (MAPSSpot::CircledCross == spot.kind)
        {
            MAPSInt32	len = MAX(1, todraw.width / 2);
            MAPSInt32	cw = MAPSInt32(len * 0.707106); // circle_ray * cos(45°)
            cv::line(output, cv::Point(spot.x - cw, spot.y - cw), cv::Point(spot.x + cw, spot.y + cw), color, thickness);
            cv::line(output, cv::Point(spot.x - cw, spot.y + cw), cv::Point(spot.x + cw, spot.y - cw), color, thickness);
            cv::circle(output, cv::Point(spot.x, spot.y), len, color, thickness);
        }
        else
            ReportWarning("Unknown spot kind");
    }
    else if (MAPSDrawingObject::Text == todraw.kind) // Text
    {
        thickness = static_cast<int>(GetIntegerProperty(PROPERTY_THICKNESS));
        MAPSText txt = todraw.text;
        cv::Size txtsize;
        int baseline;

        bool drawbkg = false;
        GetProperty(PROPERTY_DRAWBKG, drawbkg);

        if (GetBoolProperty(PROPERTY_ITALIC))
            txtsize = cv::getTextSize(txt.text, m_fontFace | cv::FONT_ITALIC, 1, thickness, &baseline);
        else
            txtsize = cv::getTextSize(txt.text, m_fontFace, 1, thickness, &baseline);
        int i = 0;
        while (txt.text[i] != '\0')
            i++;
        if (drawbkg)
        {
            r = MAPS_RGB_EXTRACT_R(todraw.text.bkcolor);
            g = MAPS_RGB_EXTRACT_G(todraw.text.bkcolor);
            b = MAPS_RGB_EXTRACT_B(todraw.text.bkcolor);

            cv::Scalar bkcolor = setColorOverlay(r, g, b);
            cv::rectangle(output, cv::Point(txt.x, txt.y), cv::Point(txt.x + txtsize.width, txt.y + txtsize.height), bkcolor, cv::FILLED);
        }
        if (GetBoolProperty(PROPERTY_ITALIC))
            cv::putText(output, txt.text, cv::Point(txt.x, txt.y + txtsize.height), m_fontFace | cv::FONT_ITALIC, 1, color, thickness);
        else
            cv::putText(output, txt.text, cv::Point(txt.x, txt.y + txtsize.height), m_fontFace, 1, color, thickness);

    }
    else
    {
        MAPSStreamedString	str;
        str << "cvOverlay: Unknown shape drawing requested: " << todraw.kind;
        ReportWarning(str);
    }
}