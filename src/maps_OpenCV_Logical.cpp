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
// Purpose of this module : Performs logical binary operations on the input images (such as AND, OR and XOR).
////////////////////////////////

#include "maps_OpenCV_Logical.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_Logical)
    MAPS_INPUT("imageIn1", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("imageIn2_fifo", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("imageIn2_sampling", MAPS::FilterIplImage, MAPS::SamplingReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_Logical)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_Logical)
    MAPS_PROPERTY_ENUM("operation", "AND|OR|XOR", 0, false, true)
    MAPS_PROPERTY_ENUM("inputs_reader_mode", "Synchronized|Triggered by First Input|Reactive", 0, false, false)
    MAPS_PROPERTY("synchro_tolerance", 0, false, false)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_Logical)
    //MAPS_ACTION("aName",MAPSOpenCV_Add::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Resize) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_Logical, "OpenCV_Logical", "2.0.3", 128,
                            MAPS::Threaded,MAPS::Threaded,
                             1, // Nb of inputs
                            -1, // Nb of outputs
                             2, // Nb of properties
                            -1) // Nb of actions

    enum InputReaderMode : uint8_t
{
    InputReaderMode_Synchronized,
    InputReaderMode_TriggeredByFirstInput,
    InputReaderMode_Reactive
};

void MAPSOpenCV_Logical::Dynamic()
{
    m_readersMode = static_cast<int>(GetIntegerProperty("inputs_reader_mode"));

    switch (m_readersMode)
    {
    case InputReaderMode_Synchronized:
        NewInput(1, "imageIn2");
        NewProperty("synchro_tolerance");
        break;
    case InputReaderMode_TriggeredByFirstInput:
        NewInput(2, "imageIn2");
        break;
    case InputReaderMode_Reactive:
        NewInput(1, "imageIn2");
        break;
    }
}

void MAPSOpenCV_Logical::Birth()
{
    m_operation = static_cast<int>(GetIntegerProperty("operation"));

    switch (m_readersMode)
    {
    case InputReaderMode_Synchronized:
        m_inputReader = MAPS::MakeInputReader::Synchronized(
            this,
            GetIntegerProperty("synchro_tolerance"),
            MAPS::InputReaderOption::Synchronized::SyncBehavior::SyncAllInputs,
            MAPS::MakeArray(&Input(0), &Input(1)),
            &MAPSOpenCV_Logical::AllocateOutputBufferSize,
            &MAPSOpenCV_Logical::ProcessData
        );
        break;
    case InputReaderMode_TriggeredByFirstInput:
        m_inputReader = MAPS::MakeInputReader::Triggered(
            this,
            Input(0),
            MAPS::InputReaderOption::Triggered::TriggerKind::DataInput,
            MAPS::InputReaderOption::Triggered::SamplingBehavior::WaitForAllInputs,
            MAPS::MakeArray(&Input(0), &Input(1)),
            &MAPSOpenCV_Logical::AllocateOutputBufferSize,
            &MAPSOpenCV_Logical::ProcessData
        );
        break;
    case InputReaderMode_Reactive:
        m_inputReader = MAPS::MakeInputReader::Reactive(
            this,
            MAPS::InputReaderOption::Reactive::FirstTimeBehavior::WaitForAllInputs,
            MAPS::InputReaderOption::Reactive::Buffering::Enabled,
            MAPS::MakeArray(&Input(0), &Input(1)),
            &MAPSOpenCV_Logical::AllocateOutputBufferSize,
            &MAPSOpenCV_Logical::ProcessData
        );
        break;
    case 3:
    default:
        Error("Unknown sampling mode");
    }
}

void MAPSOpenCV_Logical::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_Logical::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_Logical::Set(MAPSProperty& p, MAPSInt64 value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "operation")
    {
        m_operation = static_cast<int>(value);
    }
}

void MAPSOpenCV_Logical::Set(MAPSProperty& p, const MAPSString& value)
{
    MAPSComponent::Set(p, value);
    if (p.ShortName() == "operation")
    {
        m_operation = GetEnumProperty("operation").selectedEnum;
    }
}

void MAPSOpenCV_Logical::Set(MAPSProperty& p, const MAPSEnumStruct &enumStruct)
{
    MAPSComponent::Set(p, enumStruct);
    if (p.ShortName() == "operation")
    {
        m_operation = GetEnumProperty("operation").selectedEnum;
    }
}

void MAPSOpenCV_Logical::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::ArrayView<MAPS::InputElt<IplImage>> imageInElt)
{
    Output(0).AllocOutputBufferIplImage(imageInElt[0].Data());
}

void MAPSOpenCV_Logical::ProcessData(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data(); // Convert IplImage to cv::Mat without copying

    const size_t inputCount = inElts.size();

    if (inputCount != 2)
        Error("Number of inputs must be 2.");

    for (size_t i = 0; i < inputCount; ++i)
    {
        m_imageInputs[i] = convTools::noCopyIplImage2Mat(&inElts[i].Data()); // Convert IplImage to cv::Mat without copying
    }

    m_tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying

    switch (m_operation)
    {
    case 0:
        cv::bitwise_and(m_imageInputs[0], m_imageInputs[1], m_tempImageOut); // Bitwise AND
        break;
    case 1:
        cv::bitwise_or(m_imageInputs[0], m_imageInputs[1], m_tempImageOut); // Bitwise OR
        break;
    case 2:
        cv::bitwise_xor(m_imageInputs[0], m_imageInputs[1], m_tempImageOut); // Bitwise XOR
        break;
    default:
        Error("Unknown operation.");
    }

    if (static_cast<void*>(m_tempImageOut.data) != static_cast<void*>(imageOut.imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
        Error("cv::Mat data ptr and imageOut data ptr are different.");

    outGuard.VectorSize() = 0;
    outGuard.Timestamp() = ts;
}
