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
// Purpose of this module : This component exposes the Face Detection algorithms from the image processing library from OpenCV. (http://sourceforge.net/projects/opencvlibrary/)
////////////////////////////////

#include "maps_OpenCV_PatternRecognition.h" // Includes the header of this component

namespace
{
    const int MAX_TARGETS = 10;
}

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSPatternRecognition)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSPatternRecognition)
    MAPS_OUTPUT("boundingBoxes", MAPS::DrawingObject, nullptr, nullptr, MAX_TARGETS)
    MAPS_OUTPUT("centerCoords", MAPS::Integer32, nullptr, nullptr, MAX_TARGETS*2)
    //MAPS_OUTPUT("imageOut",MAPS::IplImage,nullptr,nullptr,0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSPatternRecognition)
    MAPS_PROPERTY_SUBTYPE("cascade_xml_file", "../tools/opencv_haarcascades/haarcascade_frontalface_alt.xml", false, false, MAPS::PropertySubTypeFile|MAPS::PropertySubTypeMustExist)
    MAPS_PROPERTY_ENUM("image_down_scaling", "None|2|4|8", 2, false, false)
    MAPS_PROPERTY("scale_factor", 1.2, false, true)
    MAPS_PROPERTY("min_neighbors", 1, false, true)
    MAPS_PROPERTY_ENUM("min_size", "1/4|1/8|1/16", 0, false, true)
    MAPS_PROPERTY("output_largest_object_only", true, false, true)
    MAPS_PROPERTY("bounding_boxes_width", 1, false, true)
    MAPS_PROPERTY("bounding_boxes_color", 255, false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSPatternRecognition)
    //MAPS_ACTION("aName",MAPSPatternRecognition::ActionName)
MAPS_END_ACTIONS_DEFINITION

//V1.3: added subtype file on cascade_xml_file property.
// Use the macros to declare this component (FaceDetection) behaviour
MAPS_COMPONENT_DEFINITION(MAPSPatternRecognition, "OpenCV_PatternRecognition", "2.0.2", 128,MAPS::Threaded | MAPS::Sequential, MAPS::Threaded, -1, -1, -1, -1)


void MAPSPatternRecognition::Birth()
{
    m_faceCascadeName = GetStringProperty("cascade_xml_file");

    MAPSIconv::localeChar*	localeCascade = MAPSIconv::UTF8ToLocale(m_faceCascadeName);
    localeCascade ? m_faceCascade.load(localeCascade): 0;
    MAPSIconv::releaseLocale(localeCascade);

    m_scaleFactor = GetFloatProperty("scale_factor");
    m_minNeighbors = static_cast<int>(GetIntegerProperty("min_neighbors"));
    m_minSize = static_cast<int>(GetIntegerProperty("min_size"));
    m_lineWidth = static_cast<int>(GetIntegerProperty("bounding_boxes_width"));
    m_color = static_cast<int>(GetIntegerProperty("bounding_boxes_color"));
    m_outputLargestFaceOnly = GetBoolProperty("output_largest_object_only");

    if(m_faceCascade.empty())
    {
        MAPSStreamedString sx;
        sx << "Could not load classifier cascade from file : " << m_faceCascadeName;
        Error(sx);
    }
    int scale = static_cast<int>(GetIntegerProperty("image_down_scaling"));
    m_scale = 1 << scale;

    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSPatternRecognition::Initialization,  // Called when data is received for the first time only
        &MAPSPatternRecognition::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSPatternRecognition::Core()
{
    m_inputReader->Read();
}

void MAPSPatternRecognition::Death()
{
    m_inputReader.reset();
}

void MAPSPatternRecognition::Initialization(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();
    const MAPSUInt32 imageInChannelSeq = *(MAPSUInt32*)imageIn.channelSeq;

    if (imageIn.depth != IPL_DEPTH_8U)
        Error("This component only accepts 8 bits depth images.");
    if (imageIn.dataOrder != IPL_DATA_ORDER_PIXEL)
        Error("This component only accepts pixel oriented images.");

    if (imageInChannelSeq != MAPS_CHANNELSEQ_GRAY
        && imageInChannelSeq != MAPS_CHANNELSEQ_BGR
        && imageInChannelSeq != MAPS_CHANNELSEQ_RGB)
    {
        Error("This component only accepts RGB24, BGR24 and GRAY images.");
    }
}

void MAPSPatternRecognition::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    const IplImage& imageIn = inElt.Data();
    const MAPSUInt32 imageInChannelSeq = *(MAPSUInt32*)imageIn.channelSeq;
    MAPS::OutputGuard<MAPSDrawingObject> outGuard1{ this, Output(0) };
    MAPS::OutputGuard<MAPSInt32> outGuard2{ this, Output(1) };
    m_tempImageIn = convTools::noCopyIplImage2Mat(&imageIn);

    try
    {
        if (imageInChannelSeq == MAPS_CHANNELSEQ_RGB)
            cv::cvtColor(m_tempImageIn, m_tempImageDownScaledGray, cv::COLOR_RGB2GRAY); // Convert RBG to GRAY
        else if (imageInChannelSeq == MAPS_CHANNELSEQ_BGR)
            cv::cvtColor(m_tempImageIn, m_tempImageDownScaledGray, cv::COLOR_BGR2GRAY); // Convert BGR to GRAY

        if (m_scale != 1)
        {
            if (!m_tempImageDownScaledGray.empty())
                cv::resize(m_tempImageDownScaledGray, m_tempImageDownScaledGray, cv::Size(imageIn.width / m_scale, imageIn.height / m_scale), 0, 0, cv::INTER_NEAREST); // Resize image to get better results
            else
                cv::resize(m_tempImageIn, m_tempImageDownScaledGray, cv::Size(imageIn.width / m_scale, imageIn.height / m_scale), 0, 0, cv::INTER_NEAREST); // Resize image to get better results
        }

        int vectSize;
        if (!m_tempImageDownScaledGray.empty())
            vectSize = detectAndDraw(m_tempImageDownScaledGray, &outGuard1.Data(), &outGuard2.Data());
        else
            vectSize = detectAndDraw(m_tempImageIn, &outGuard1.Data(), &outGuard2.Data());

        outGuard1.Timestamp() = ts;
        outGuard2.Timestamp() = ts;
        outGuard1.VectorSize() = vectSize;
        outGuard2.VectorSize() = vectSize * 2;
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

int MAPSPatternRecognition::detectAndDraw(cv::Mat imgIn, MAPSDrawingObject* dobjs, MAPSInt32* ints )
{
    if (!m_faceCascade.empty())
    {
        std::vector<cv::Rect> faces;
        int min_size = imgIn.size[0] >> (m_minSize + 2);
        m_faceCascade.detectMultiScale(imgIn, faces, m_scaleFactor, m_minNeighbors, cv::CASCADE_DO_CANNY_PRUNING, cv::Size(min_size, m_minSize));

        //Add check on number of m_faces
        if (faces.empty())
        {
            return 0;
        }
        int nbDobjs;
        if (false == m_outputLargestFaceOnly)
        {
            nbDobjs = ((MAX_TARGETS < static_cast<int>(faces.size())) ? MAX_TARGETS : static_cast<int>(faces.size()));
            for (int i = 0; i < nbDobjs; i++)
            {
                cv::Rect r = faces[i];
                dobjs[i].kind = MAPSDrawingObject::Rectangle;
                dobjs[i].color = m_color;
                dobjs[i].width = m_lineWidth;
                dobjs[i].rectangle.x1 = r.x*m_scale;
                dobjs[i].rectangle.y1 = r.y*m_scale;
                dobjs[i].rectangle.x2 = (r.x + r.width)*m_scale;
                dobjs[i].rectangle.y2 = (r.y + r.height)*m_scale;
                ints[i * 2] = (dobjs[i].rectangle.x1 + dobjs[i].rectangle.x2) >> 1;
                ints[i * 2 + 1] = (dobjs[i].rectangle.y1 + dobjs[i].rectangle.y2) >> 1;
            }
            return nbDobjs;
        }
        else
        {
            nbDobjs = static_cast<int>(faces.size());
            cv::Rect largestRect;
            int maxSurf = 0;
            for (int i = 0; i < nbDobjs; i++)
            {
                cv::Rect r = faces[i];
                int surf = r.height*r.width;
                if (surf > maxSurf)
                {
                    maxSurf = surf;
                    largestRect = r;
                }
            }
            if (!largestRect.empty())
            {
                dobjs[0].kind = MAPSDrawingObject::Rectangle;
                dobjs[0].color = m_color;
                dobjs[0].width = m_lineWidth;
                dobjs[0].rectangle.x1 = largestRect.x*m_scale;
                dobjs[0].rectangle.y1 = largestRect.y*m_scale;
                dobjs[0].rectangle.x2 = (largestRect.x + largestRect.width)*m_scale;
                dobjs[0].rectangle.y2 = (largestRect.y + largestRect.height)*m_scale;
                ints[0] = (dobjs[0].rectangle.x1 + dobjs[0].rectangle.x2) >> 1;
                ints[1] = (dobjs[0].rectangle.y1 + dobjs[0].rectangle.y2) >> 1;

                return 1;
            }
            else
            {
                return 0;
            }
        }
    }
    return 0;
}

void MAPSPatternRecognition::Set(MAPSProperty& p, MAPSInt64 value)
{
    if (p.ShortName() == "min_neighbors")
    {
        m_minNeighbors = static_cast<int>(value);
    }
    else if (p.ShortName() == "min_size")
    {
        m_minSize = static_cast<int>(value);
    }
    else if (p.ShortName() == "bounding_boxes_width")
    {
        m_lineWidth = static_cast<int>(value);
    }
    else if (p.ShortName() == "bounding_boxes_color")
    {
        m_color = static_cast<int>(value);
    }
    MAPSComponent::Set(p, value);
}

void MAPSPatternRecognition::Set(MAPSProperty& p, const MAPSEnumStruct& enumStruct)
{
    if (p.ShortName() == "min_size")
    {
        m_minSize = enumStruct.selectedEnum;
    }
    MAPSComponent::Set(p, enumStruct);
}

void MAPSPatternRecognition::Set(MAPSProperty& p, const MAPSString& value)
{
    if (p.ShortName() == "min_size")
    {
        m_minSize = GetEnumProperty("min_size").selectedEnum;
    }
    MAPSComponent::Set(p, value);
}

void MAPSPatternRecognition::Set(MAPSProperty& p, MAPSFloat64 value)
{
    if (p.ShortName() == "scale_factor")
        m_scaleFactor = value;
    MAPSComponent::Set(p, value);
}

void MAPSPatternRecognition::Set(MAPSProperty& p, bool value)
{
    if (p.ShortName() == "output_largest_object_only")
    {
        m_outputLargestFaceOnly = value;
    }
    MAPSComponent::Set(p, value);
}
