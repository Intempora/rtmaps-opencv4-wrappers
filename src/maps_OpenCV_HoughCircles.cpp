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
// Purpose of this module : Finds circles in grayscale image using Hough transform.
////////////////////////////////

#include "maps_OpenCV_HoughCircles.h"   // Includes the header of this component

namespace
{
    const int MAX_DOBJS = 50;
}

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSHoughCircles)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSHoughCircles)
    MAPS_OUTPUT("circlesObjects", MAPS::DrawingObject, nullptr, nullptr, MAX_DOBJS)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSHoughCircles)
    MAPS_PROPERTY("accumulator_resolution", 2.0, false, true)
    MAPS_PROPERTY("min_dist_between_centers", 60, false, true)
    MAPS_PROPERTY("edges_threshold", 200, false, true)
    MAPS_PROPERTY("accumulator_threshold", 30, false, true)
    MAPS_PROPERTY("min_radius", 0, false, true)
    MAPS_PROPERTY("max_radius", 0, false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSHoughCircles)
    //MAPS_ACTION("aName",MAPSHoughCircles::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (HoughTransform) behaviour
MAPS_COMPONENT_DEFINITION(MAPSHoughCircles, "OpenCV_HoughCircles", "2.0.3", 128,
                            MAPS::Threaded | MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                             1, // Nb of outputs
                            -1, // Nb of properties
                            -1) // Nb of actions


void MAPSHoughCircles::Birth()
{
    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSHoughCircles::Initialization,  // Called when data is received for the first time only
        &MAPSHoughCircles::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSHoughCircles::Core()
{
    m_inputReader->Read();
}

void MAPSHoughCircles::Death()
{
    m_inputReader.reset();
}

void MAPSHoughCircles::Initialization(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();
    if (*(MAPSUInt32*)imageIn.channelSeq != MAPS_CHANNELSEQ_GRAY)
    {
        Error("This component only accepts GRAY images on its input. (8 bpp)");
    }
}

void MAPSHoughCircles::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    MAPS::OutputGuard<MAPSDrawingObject> outGuard{ this, Output(0) };
    m_tempImageIn = convTools::copyIplImage2Mat(&inElt.Data()); // Convert IplImage to cv::Mat

    try
    {
        cv::medianBlur(m_tempImageIn, m_tempImageIn, 7); // Blur the image to improve the detection

        // Detect Circles using the function HoughCircles
        cv::HoughCircles(m_tempImageIn, m_circles, cv::HOUGH_GRADIENT,
            GetFloatProperty("accumulator_resolution"),
            static_cast<int>(GetIntegerProperty("min_dist_between_centers")),
            static_cast<int>(GetIntegerProperty("edges_threshold")),
            static_cast<int>(GetIntegerProperty("accumulator_threshold")),
            static_cast<int>(GetIntegerProperty("min_radius")),
            static_cast<int>(GetIntegerProperty("max_radius")));
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }

    // Create and output the detected circles
    int nbDobjs = MIN(MAX_DOBJS, static_cast<int>(m_circles.size()));
    for (int i = 0; i < nbDobjs; i++)
    {
        MAPSDrawingObject& dobj = outGuard.Data();
        dobj.kind = MAPSDrawingObject::Circle;
        dobj.color = MAPS_RGB(255, 0, 0);
        dobj.width = 1;

        cv::Vec3f circle = m_circles[i];
        dobj.circle.x = static_cast<MAPSInt32>(circle[0]);
        dobj.circle.y = static_cast<MAPSInt32>(circle[1]);
        dobj.circle.radius = static_cast<MAPSInt32>(circle[2]);
    }

    outGuard.VectorSize() = 0;
    outGuard.Timestamp() = ts;
}
