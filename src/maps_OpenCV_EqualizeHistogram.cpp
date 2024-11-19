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
// Purpose of this module : Equalizes histogram of grayscale image.
////////////////////////////////


#include "maps_OpenCV_EqualizeHistogram.h"  // Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_EqualizeHistogram)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_EqualizeHistogram)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_EqualizeHistogram)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_EqualizeHistogram)
    //MAPS_ACTION("aName",MAPSOpenCV_EqualizeHistogram::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Resize) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_EqualizeHistogram,"OpenCV_HistogramEqualize", "2.0.3", 128,
                            MAPS::Threaded|MAPS::Sequential, MAPS::Threaded,
                            -1, // Nb of inputs
                            -1, // Nb of outputs
                            -1, // Nb of properties
                            -1) // Nb of actions


void MAPSOpenCV_EqualizeHistogram::Birth()
{
    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSOpenCV_EqualizeHistogram::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSOpenCV_EqualizeHistogram::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSOpenCV_EqualizeHistogram::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_EqualizeHistogram::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_EqualizeHistogram::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    Output(0).AllocOutputBufferIplImage(imageInElt.Data());
}

void MAPSOpenCV_EqualizeHistogram::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    const IplImage& imageIn = inElt.Data();
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();
    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying
    m_tempImageIn = convTools::noCopyIplImage2Mat(&imageIn); // Convert IplImage to cv::Mat without copying

    try
    {
        if (m_tempImageIn.channels() == 1) // If there is only one channel, equalize it
        {
            cv::equalizeHist(m_tempImageIn, m_tempImageOut);
        }
        else
        {
            cv::split(m_tempImageIn, m_planesMatImages); // Split the channels
            for (int i = 0; i < imageIn.nChannels; i++)
            {
                cv::equalizeHist(m_planesMatImages[i], m_planesMatImages[i]); // Equalize all single-channel
            }
            cv::merge(m_planesMatImages, m_tempImageOut); // Merge to produce the equalize image
        }
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
