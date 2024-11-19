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
// Purpose of this module : Finds lines in grayscale image using Hough transform.
////////////////////////////////

#include "maps_OpenCV_HoughLines.h" // Includes the header of this component

namespace
{
    const int MAX_DOBJS = 50;
}

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSHoughTransform)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSHoughTransform)
    MAPS_OUTPUT("linesObjects", MAPS::DrawingObject, nullptr, nullptr, MAX_DOBJS)
    MAPS_OUTPUT("edgesImage", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSHoughTransform)
#if MAPS_KERNEL_BUILD > 95
    MAPS_PROPERTY_ENUM("method", "standard|probabilistic|multi_scale", 0, false, false)
#else
    MAPS_PROPERTY("method", "standard", false, false)
#endif
    MAPS_PROPERTY("dist_resolution", 1.0, false, true)
    MAPS_PROPERTY("angle_resolution", 0.0314, false, true)
    MAPS_PROPERTY("threshold", 150, false, true)
    MAPS_PROPERTY("param1", 0.0, false, true)
    MAPS_PROPERTY("param2", 0.0, false, true)
    MAPS_PROPERTY("output_edges_image", false, false, false)
    MAPS_PROPERTY("edges_threshold1", 50, false, true)
    MAPS_PROPERTY("edges_threshold2", 200, false, true)
    MAPS_PROPERTY("edges_aperture", 3, false, true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSHoughTransform)
    //MAPS_ACTION("aName",MAPSHoughTransform::ActionName)
MAPS_END_ACTIONS_DEFINITION

//V1.1  20140704: check aperture size
// Use the macros to declare this component (HoughTransform) behaviour
MAPS_COMPONENT_DEFINITION(MAPSHoughTransform, "OpenCV_HoughLines", "2.0.3", 128,
                            MAPS::Threaded | MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                             1, // Nb of outputs
                            10, // Nb of properties
                             0) // Nb of actions

void MAPSHoughTransform::Dynamic()
{
    m_outputEdges = GetBoolProperty("output_edges_image");
    if (m_outputEdges)
    {
        NewOutput(1);
    }
}

void MAPSHoughTransform::Birth()
{
    MAPSString methodString = GetStringProperty("method").Uppercase();
    if (methodString == "STANDARD")
    {
        m_method = cv::HOUGH_STANDARD;
    }
    else if (methodString == "PROBABILISTIC")
    {
        m_method = cv::HOUGH_PROBABILISTIC;
    }
    else if (methodString == "MULTI_SCALE")
    {
        m_method = cv::HOUGH_MULTI_SCALE;
    }
    else
    {
        Error("Unknown method : possible strings are \"standard\", \"probabilistic\", and \"multi_scale\".");
    }

    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSHoughTransform::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSHoughTransform::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSHoughTransform::Core()
{
    m_inputReader->Read();
}

void MAPSHoughTransform::Death()
{
    m_inputReader.reset();
}

void MAPSHoughTransform::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();
    if (*(MAPSUInt32*)imageIn.channelSeq != MAPS_CHANNELSEQ_GRAY)
    {
        Error("This component only accepts GRAY images on its input. (8 bpp)");
    }

    if (m_outputEdges)
    {
        IplImage model = MAPS::IplImageModel(imageIn.width, imageIn.height, imageIn.channelSeq, imageIn.dataOrder, imageIn.depth, imageIn.align);
        Output(1).AllocOutputBufferIplImage(model);
    }
}

void MAPSHoughTransform::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    const IplImage& imageIn = inElt.Data();
    m_tempImageIn = convTools::noCopyIplImage2Mat(&imageIn); // Convert IplImage to cv::Mat without copying

    double rhoRes = GetFloatProperty("dist_resolution");
    double thetaRes = GetFloatProperty("angle_resolution");
    int threshold = static_cast<int>(GetIntegerProperty("threshold"));
    double param1 = 0.0, param2 = 0.0;

    if (m_method != cv::HOUGH_STANDARD)
    {
        param1 = GetFloatProperty("param1");
        param2 = GetFloatProperty("param2");
    }

    int edgesThreshold1 = static_cast<int>(GetIntegerProperty("edges_threshold1"));
    int edgesThreshold2 = static_cast<int>(GetIntegerProperty("edges_threshold2"));
    int edgesAperture = static_cast<int>(GetIntegerProperty("edges_aperture"));

    if (edgesAperture != 3 && edgesAperture != 5 && edgesAperture != 7)
        Error("Aperture size must be 3, 5 or 7 for cvCanny");

    std::unique_ptr<MAPS::OutputGuard<IplImage>> outGuardEdges;
    if (m_outputEdges)
    {
        outGuardEdges.reset(new MAPS::OutputGuard<IplImage>(this, Output(1)));
        outGuardEdges->Timestamp() = ts;
        IplImage& edgesImageOut = outGuardEdges->Data();
        m_tempImageOut = convTools::noCopyIplImage2Mat(&edgesImageOut); // Convert IplImage to cv::Mat without copying
    }

    cv::Canny(m_tempImageIn, m_tempImageOut, edgesThreshold1, edgesThreshold2, edgesAperture);

    MAPS::OutputGuard<MAPSDrawingObject> outGuard{ this, Output(0) };

    if (m_method != cv::HOUGH_PROBABILISTIC)
    {
        // Detect Lines using the function HoughLines
        cv::HoughLines(m_tempImageOut, m_lines, rhoRes, thetaRes, threshold, param1, param2);

        // Create and output the detected lines
        int nbDobjs = MIN(MAX_DOBJS, static_cast<int>(m_lines.size()));
        for (int i = 0; i < nbDobjs; i++)
        {
            MAPSDrawingObject& dobj = outGuard.Data();
            dobj.kind = MAPSDrawingObject::Line;
            dobj.color = MAPS_RGB(255, 0, 0);
            dobj.width = 1;

            cv::Vec2f line = m_lines[i];
            float rho = line[0];
            float theta = line[1];
            double a = cos(theta), b = sin(theta);
            if (fabs(a) < 0.001)
            {
                dobj.line.x1 = dobj.line.x2 = cvRound(rho);
                dobj.line.y1 = 0;
                dobj.line.y2 = imageIn.height;
            }
            else if (fabs(b) < 0.001)
            {
                dobj.line.y1 = dobj.line.y2 = cvRound(rho);
                dobj.line.x1 = 0;
                dobj.line.x2 = imageIn.width;
            }
            else
            {
                dobj.line.x1 = 0;
                dobj.line.y1 = cvRound(rho / b);
                dobj.line.x2 = cvRound(rho / a);
                dobj.line.y2 = 0;
            }
        }
        outGuard.VectorSize() = nbDobjs;
    }
    else // Probalistic
    {
        // Detect Lines using the function HoughLinesP
        cv::HoughLinesP(m_tempImageOut, m_linesP, rhoRes, thetaRes, threshold, param1, param2);

        // Create and output the detected lines
        int nbDobjs = MIN(MAX_DOBJS, static_cast<int>(m_linesP.size()));
        for (int i = 0; i < nbDobjs; i++)
        {
            MAPSDrawingObject& dobj = outGuard.Data();
            dobj.kind = MAPSDrawingObject::Line;
            dobj.color = MAPS_RGB(255, 0, 0);
            dobj.width = 1;

            cv::Vec4i lineP = m_linesP[i];
            float rho = static_cast<float>(lineP[0]);
            float theta = static_cast<float>(lineP[1]);
            double a = cos(theta), b = sin(theta);
            if (fabs(a) < 0.001)
            {
                dobj.line.x1 = dobj.line.x2 = cvRound(rho);
                dobj.line.y1 = 0;
                dobj.line.y2 = imageIn.height;
            }
            else if (fabs(b) < 0.001)
            {
                dobj.line.y1 = dobj.line.y2 = cvRound(rho);
                dobj.line.x1 = 0;
                dobj.line.x2 = imageIn.width;
            }
            else
            {
                dobj.line.x1 = 0;
                dobj.line.y1 = cvRound(rho / b);
                dobj.line.x2 = cvRound(rho / a);
                dobj.line.y2 = 0;
            }
        }
        outGuard.VectorSize() = nbDobjs;
    }

    outGuard.Timestamp() = ts;

    if (m_outputEdges)
    {
        if (static_cast<void*>(m_tempImageOut.data) != static_cast<void*>(outGuardEdges->Data().imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
            Error("cv::Mat data ptr and imageOut data ptr are different.");

        outGuardEdges->VectorSize() = 0;
    }
}
