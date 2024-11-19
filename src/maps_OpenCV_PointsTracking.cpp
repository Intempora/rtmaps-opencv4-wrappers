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
// Purpose of this module : Performs points tracking algorithms within the images flow.
//                          The points to track can be provided within the second input, or can be automatically selected via the Auto init action.
////////////////////////////////

#include "maps_OpenCV_PointsTracking.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_PointsTracking)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("pointsToTrack", MAPS::FilterInteger32, MAPS::FifoReader)
    MAPS_INPUT("autoInitTrigger", MAPS::FilterAny, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_PointsTracking)
    MAPS_OUTPUT("trackedPointsCoords", MAPS::Integer32, nullptr, nullptr, 0)
    MAPS_OUTPUT("trackedPointsObjects", MAPS::DrawingObject, nullptr, nullptr, 0)
    MAPS_OUTPUT("previousPointsCoords", MAPS::Integer32, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_PointsTracking)
    MAPS_PROPERTY("max_nb_points_to_track", 100, false, false)
    MAPS_PROPERTY("auto_init_at_start", false, false, true)
    MAPS_PROPERTY("use_auto_init_trigger_input", false, false, false)
    MAPS_PROPERTY("spots_width", 1, false, true)
    MAPS_PROPERTY("spotsm_color", 255, false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_PointsTracking)
    MAPS_ACTION2("autoInit", MAPSOpenCV_PointsTracking::AutoInitAction, false)
    MAPS_ACTION2("removeAllPoints", MAPSOpenCV_PointsTracking::RemoveAllPointsAction, false)
MAPS_END_ACTIONS_DEFINITION

//V1.1: corrected bug.

// Use the macros to declare this component (OpenCV_PointsTracking) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_PointsTracking, "OpenCV_PointsTracking", "2.0.2", 128,
                            MAPS::Threaded,MAPS::Threaded,
                             2, // Nb of inputs
                            -1, // Nb of outputs
                            -1, // Nb of properties
                            -1) // Nb of actions

void MAPSOpenCV_PointsTracking::Dynamic()
{
    m_useAutoInitInput = GetBoolProperty("use_auto_init_trigger_input");
    if (m_useAutoInitInput)
    {
        m_nbInputs = 3;
        NewInput("autoInitTrigger");
    }
    else
    {
        m_nbInputs = 2;
    }
}

void MAPSOpenCV_PointsTracking::Birth()
{
    m_nbPoints2Track = 0;
    m_flags = 0;
    m_needAutoInit = GetBoolProperty("auto_init_at_start");
    m_maxNbPoints = static_cast<int>(GetIntegerProperty("max_nb_points_to_track"));
    m_lineWidth = static_cast<int>(GetIntegerProperty("spots_width"));
    m_color = static_cast<int>(GetIntegerProperty("spotsm_color"));
    Output(0).AllocOutputBuffer(m_maxNbPoints*2);
    Output(1).AllocOutputBuffer(m_maxNbPoints);
    Output(2).AllocOutputBuffer(m_maxNbPoints*2);
    m_points[0].resize(m_maxNbPoints);
    m_points[1].resize(m_maxNbPoints);
    m_swapPoints.resize(m_maxNbPoints);


    m_inputs.push_back(&Input(0));
    m_inputs.push_back(&Input(1));
    if (m_useAutoInitInput)
        m_inputs.push_back(&Input(2));

    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        MAPS::InputReaderOption::Reactive::FirstTimeBehavior::Immediate,
        MAPS::InputReaderOption::Reactive::Buffering::Enabled,
        m_inputs,
        &MAPSOpenCV_PointsTracking::ProcessData
    );
}

void MAPSOpenCV_PointsTracking::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_PointsTracking::Death()
{
    m_inputReader.reset();
    m_inputs.clear();
}

void MAPSOpenCV_PointsTracking::OutputCurrentPoints(MAPSTimestamp t)
{
    MAPS::OutputGuard<MAPSInt32> outGuard{ this, Output(0) };
    outGuard.Timestamp() = t;

    for (int i = 0; i < m_nbPoints2Track; i++)
    {
        outGuard.Data(2 * i) = static_cast<int>(m_points[1][i].x);
        outGuard.Data(2 * i + 1) = static_cast<int>(m_points[1][i].y);
    }
    outGuard.VectorSize() = m_nbPoints2Track * 2;
}

void MAPSOpenCV_PointsTracking::OutputPreviousPoints(MAPSTimestamp t)
{
    MAPS::OutputGuard<MAPSInt32> outGuard{ this, Output(2) };
    outGuard.Timestamp() = t;

    for (int i = 0; i < m_nbPoints2Track; i++)
    {
        outGuard.Data(2 * i) = static_cast<int>(m_points[0][i].x);
        outGuard.Data(2 * i + 1) = static_cast<int>(m_points[0][i].y);
    }
    outGuard.VectorSize() = m_nbPoints2Track * 2;
}

void MAPSOpenCV_PointsTracking::OutputCurrentObjects(MAPSTimestamp t)
{
    MAPS::OutputGuard<MAPSDrawingObject> outGuard{ this, Output(1) };
    outGuard.Timestamp() = t;

    for (int i = 0; i < m_nbPoints2Track; i++)
    {
        MAPSDrawingObject& dobj = outGuard.Data(i);
        dobj.kind = MAPSDrawingObject::Spot;
        dobj.width = m_lineWidth;
        dobj.color = m_color;
        dobj.spot.x = static_cast<int>(m_points[1][i].x);
        dobj.spot.y = static_cast<int>(m_points[1][i].y);
        dobj.spot.kind = MAPSSpot::CircledCross;
    }
    outGuard.VectorSize() = m_nbPoints2Track;
}

void MAPSOpenCV_PointsTracking::AutoInitAction(MAPSModule *module, int actionNb)
{
    (void)actionNb;
    MAPSOpenCV_PointsTracking* c = reinterpret_cast<MAPSOpenCV_PointsTracking*>(module);
    c->m_needAutoInit = true;
}

void MAPSOpenCV_PointsTracking::RemoveAllPointsAction(MAPSModule *module, int actionNb)
{
    (void)actionNb;
    MAPSOpenCV_PointsTracking* c = reinterpret_cast<MAPSOpenCV_PointsTracking*>(module);
    c->m_nbPoints2Track = 0;
}

void MAPSOpenCV_PointsTracking::ProcessData(const MAPSTimestamp ts, const size_t inputThatAnswered, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
{
    try
    {
        switch (inputThatAnswered)
        {
        case 0: //IplImage
        {
            ProcessIplImage(inElts[inputThatAnswered].DataAs<IplImage>(), ts);
            break;
        }
        case 1: // Points.
        {
            ProcessPoints(inElts[inputThatAnswered].VectorSize(), &inElts[inputThatAnswered].DataAs<MAPSInt32>(), ts);
            break;
        }
        case 2: // Auto init input
            m_needAutoInit = true;
            break;
        }
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

void MAPSOpenCV_PointsTracking::ProcessIplImage(const IplImage& imageIn, const MAPSTimestamp& ts)
{
    const MAPSUInt32 imageInChannelSeq = *(MAPSUInt32*)imageIn.channelSeq;
    m_image = convTools::noCopyIplImage2Mat(&imageIn); // Convert IplImage to cv::Mat
    if (m_gray.empty())
    {
        if (imageIn.depth != IPL_DEPTH_8U)
            Error("This component only accepts 8 bits depth images.");
        if (imageInChannelSeq != MAPS_CHANNELSEQ_RGB &&
            imageInChannelSeq != MAPS_CHANNELSEQ_BGR &&
            imageInChannelSeq != MAPS_CHANNELSEQ_GRAY)
            Error("This component only accepts RGB24, BGR24 and GRAY images.");

        cv::Size imgSize;
        imgSize.width = imageIn.width;
        imgSize.height = imageIn.height;
        m_gray = cv::Mat(imgSize, 8, 1);
        m_prevGray = cv::Mat(imgSize, 8, 1);
        m_flags = 0;
        if (m_gray.empty() || m_prevGray.empty())
            Error("Not enough memory.");
        if (m_nbPoints2Track > 0)
        {
            // Convert image to grayscale if necessary.
            if (imageInChannelSeq == MAPS_CHANNELSEQ_RGB)
                cv::cvtColor(m_image, m_gray, cv::COLOR_RGB2GRAY);
            else if (imageInChannelSeq == MAPS_CHANNELSEQ_BGR)
                cv::cvtColor(m_image, m_gray, cv::COLOR_BGR2GRAY);
            else
                m_image.copyTo(m_gray);
            cv::cornerSubPix(m_gray, m_points[1], cv::Size(10, 10), cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03));
            OutputCurrentPoints(ts);
            OutputCurrentObjects(ts);
            cv::swap(m_prevGray, m_gray);
            cv::swap(m_points[0], m_points[1]);
            return;
        }
    }

    // Convert image to grayscale if necessary.
    if (imageInChannelSeq == MAPS_CHANNELSEQ_RGB)
        cv::cvtColor(m_image, m_gray, cv::COLOR_RGB2GRAY);
    else if (imageInChannelSeq == MAPS_CHANNELSEQ_BGR)
        cv::cvtColor(m_image, m_gray, cv::COLOR_BGR2GRAY);
    else
        m_image.copyTo(m_gray);

    // Auto Initialisation of the tracked points.
    if (m_needAutoInit)
    {
        m_needAutoInit = false;
        m_nbPoints2Track = m_maxNbPoints;
        cv::goodFeaturesToTrack(m_gray, m_points[1], m_nbPoints2Track, 0.01, 10, cv::noArray(), 3, false, 0.04);
        cv::cornerSubPix(m_gray, m_points[1], cv::Size(10, 10), cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03));
        OutputCurrentPoints(ts);
    }
    else if (m_nbPoints2Track > 0)
    {
        cv::calcOpticalFlowPyrLK(m_prevGray, m_gray, m_points[0], m_points[1],
            m_status, cv::noArray(), cv::Size(10, 10), 3,
            cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03), m_flags);
        m_flags |= cv::OPTFLOW_LK_GET_MIN_EIGENVALS;

        for (int i = 0; i < m_nbPoints2Track; i++)
        {
            if (m_status.empty())
            {
                m_points[1][i].x = -10;
                m_points[1][i].y = -10;
            }
        }

        OutputCurrentPoints(ts);
        OutputCurrentObjects(ts);
        OutputPreviousPoints(ts);
    }
    else
    {
        OutputCurrentPoints(ts);
        OutputCurrentObjects(ts);
        OutputPreviousPoints(ts);
    }
    cv::swap(m_prevGray, m_gray);
    cv::swap(m_points[0], m_points[1]);
}

void MAPSOpenCV_PointsTracking::ProcessPoints(const size_t vectorSize, const MAPSInt32* dataPtr, const MAPSTimestamp& ts)
{
    m_nbPoints2Track = static_cast<int>(vectorSize / 2);

    for (int i = 0; i < m_nbPoints2Track; i++)
    {
        m_points[1][i].x = static_cast<float>(dataPtr[2 * i]);
        m_points[1][i].y = static_cast<float>(dataPtr[2 * i + 1]);
    }

    if (!m_gray.empty())
    {
        cv::cornerSubPix(m_gray, m_points[1], cv::Size(10, 10), cv::Size(-1, -1), cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03));
        OutputCurrentPoints(ts);
        OutputCurrentObjects(ts);
        cv::swap(m_points[0], m_points[1]);
    }
}

void MAPSOpenCV_PointsTracking::Set(MAPSProperty& p, MAPSInt64 value)
{
    if (p.ShortName() == "spots_width")
    {
        m_lineWidth = static_cast<int>(value);
    }
    else if (p.ShortName() == "spotsm_color")
    {
        m_color = static_cast<int>(value);
    }
    MAPSComponent::Set(p, value);
}