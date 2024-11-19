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
// Purpose of this module : This component can perform rotations and/or flip operations on the input image (+90deg, -90deg, 180 deg, horizontal flip, vertical flip).
////////////////////////////////

#include "maps_OpenCV_RotateAndFlip.h"	// Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_RotateAndFlip)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("angle_in", MAPS::FilterInteger32, MAPS::SamplingReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_RotateAndFlip)
    MAPS_OUTPUT("imageOut", MAPS::IplImage, nullptr, nullptr, 0)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_RotateAndFlip)
    MAPS_PROPERTY_ENUM("operation", "None|90 deg clockwise|90 deg counter-clockwise|180 deg|Flip up-down|Flip left-right|Specify in degrees", 0, false, false)
    MAPS_PROPERTY_ENUM("angle_input_mode", "Property|Input", 0, false, false)
    MAPS_PROPERTY("angle", 0, false, true)
    MAPS_PROPERTY("use_gpu", false, false, false)
    MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_RotateAndFlip)
    //MAPS_ACTION("aName",MAPSOpenCV_RotateAndFlip::ActionName)
MAPS_END_ACTIONS_DEFINITION

//Version 1.1: added rotation with certain angle, eventually provided on input.
//Version 1.2: corrected rotation for 90 deg counter clockwise.

// Use the macros to declare this component (OpenCV_RotateAndFlip) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_RotateAndFlip, "OpenCV_RotateAndFlip", "2.0.4", 128,
                         MAPS::Threaded, MAPS::Threaded,
                         1, // Nb of inputs. Leave -1 to use the number of declared input definitions
                        -1, // Nb of outputs. Leave -1 to use the number of declared output definitions
                         1, // Nb of properties. Leave -1 to use the number of declared property definitions
                        -1) // Nb of actions. Leave -1 to use the number of declared action definitions

enum Operation : uint8_t
{
    Operation_None,
    Operation_Rotation_90_ClockWise,
    Operation_Rotation_90_CounterClockWise,
    Operation_Rotation_180,
    Operation_Flip_Up_Down,
    Operation_Flip_Left_Right,
    Operation_Rotation_SpecifiedDegrees
};

void MAPSOpenCV_RotateAndFlip::Dynamic()
{
    m_operation = static_cast<int>(GetIntegerProperty("operation"));
    if (m_operation == 6)
    {
        NewProperty("angle_input_mode");
        m_angleInputMode = static_cast<int>(GetIntegerProperty("angle_input_mode"));

        if (m_angleInputMode == 0)
            NewProperty("angle");
        else
            NewInput("angle_in");
    }

    m_useGpu = false;

    if (cv::ocl::haveOpenCL())
    {
        std::vector<cv::ocl::PlatformInfo> platform_info;
        cv::ocl::getPlatfomsInfo(platform_info);
        if (platform_info.size() > 0)
        {
            NewProperty("use_gpu");
            m_useGpu = GetBoolProperty("use_gpu");
            cv::ocl::setUseOpenCL(m_useGpu);
        }
    }
}

void MAPSOpenCV_RotateAndFlip::Birth()
{
    m_inputs.push_back(&Input(0));
    if (m_operation == 6 && m_angleInputMode != 0)
        m_inputs.push_back(&Input(1));

    m_inputReader = MAPS::MakeInputReader::Triggered(
        this,
        Input(0),
        MAPS::InputReaderOption::Triggered::TriggerKind::DataInput,
        MAPS::InputReaderOption::Triggered::SamplingBehavior::WaitForAllInputs,
        m_inputs,
        &MAPSOpenCV_RotateAndFlip::AllocateOutputBufferSize,
        &MAPSOpenCV_RotateAndFlip::ProcessData
    );
}

void MAPSOpenCV_RotateAndFlip::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_RotateAndFlip::Death()
{
    m_inputReader.reset();
    m_inputs.clear();
}

void MAPSOpenCV_RotateAndFlip::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::ArrayView <MAPS::InputElt<>> inElts)
{
    IplImage model;
    const IplImage& imageIn = inElts[0].DataAs<IplImage>();

    switch (m_operation)
    {
    case Operation_None: // None
    case Operation_Rotation_180: // 180 deg
    case Operation_Flip_Up_Down: // Flip up-down
    case Operation_Flip_Left_Right: // Flip left-right
    case Operation_Rotation_SpecifiedDegrees: // Specify in degrees
        model = imageIn;
        break;

    case Operation_Rotation_90_ClockWise: // 90 deg clockwise
    case Operation_Rotation_90_CounterClockWise: // 90 deg counter-clockwise
        model = MAPS::IplImageModel(imageIn.height, imageIn.width, imageIn.channelSeq, imageIn.dataOrder, imageIn.depth, imageIn.align);
        break;

    default:
        Error("Unknown operation.");
    }
    Output(0).AllocOutputBufferIplImage(model);
}

void MAPSOpenCV_RotateAndFlip::ProcessData(const MAPSTimestamp ts, const MAPS::ArrayView <MAPS::InputElt<>> inElts)
{
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data();
    const IplImage& imageIn = inElts[0].DataAs<IplImage>();

    cv::Mat tempImageOut = convTools::noCopyIplImage2Mat(&imageOut); // Convert IplImage to cv::Mat without copying
    cv::Mat tempImageIn = convTools::noCopyIplImage2Mat(&imageIn);

    try
    {
        switch (m_operation)
        {
        case Operation_None: // None
            memcpy(imageOut.imageData, imageIn.imageData, imageIn.imageSize);
            break;

        case Operation_Rotation_90_ClockWise: // 90 deg clockwise
            if (m_useGpu)
            {
                //Due to the copy of CPU memory to GPU memory, this part is not efficient but it's just an example
                cv::UMat in = tempImageIn.getUMat(cv::ACCESS_READ, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::UMat out = tempImageOut.getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::rotate(in, out, cv::RotateFlags::ROTATE_90_CLOCKWISE);
            }
            else
            {
                cv::rotate(tempImageIn, tempImageOut, cv::RotateFlags::ROTATE_90_CLOCKWISE);
            }
            break;

        case Operation_Rotation_90_CounterClockWise: // 90 deg counter-clockwise
            if (m_useGpu)
            {
                //Due to the copy of CPU memory to GPU memory, this part is not efficient but it's just an example
                cv::UMat in = tempImageIn.getUMat(cv::ACCESS_READ, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::UMat out = tempImageOut.getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::rotate(in, out, cv::RotateFlags::ROTATE_90_COUNTERCLOCKWISE);
            }
            else
            {
                cv::rotate(tempImageIn, tempImageOut, cv::RotateFlags::ROTATE_90_COUNTERCLOCKWISE);
            }
            break;

        case Operation_Rotation_180: // 180 deg
            if (m_useGpu)
            {
                //Due to the copy of CPU memory to GPU memory, this part is not efficient but it's just an example
                cv::UMat in = tempImageIn.getUMat(cv::ACCESS_READ, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::UMat out = tempImageOut.getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::flip(in, out, -1);
            }
            else
            {
                cv::flip(tempImageIn, tempImageOut, -1);
            }
            break;

        case Operation_Flip_Up_Down: // Flip up-down
            if (m_useGpu)
            {
                //Due to the copy of CPU memory to GPU memory, this part is not efficient but it's just an example
                cv::UMat in = tempImageIn.getUMat(cv::ACCESS_READ, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::UMat out = tempImageOut.getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::flip(in, out, 0);
            }
            else
            {
                cv::flip(tempImageIn, tempImageOut, 0);
            }
            break;

        case Operation_Flip_Left_Right: // Flip left-right
            if (m_useGpu)
            {
                //Due to the copy of CPU memory to GPU memory, this part is not efficient but it's just an example
                cv::UMat in = tempImageIn.getUMat(cv::ACCESS_READ, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::UMat out = tempImageOut.getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::flip(in, out, 1);
            }
            else
            {
                cv::flip(tempImageIn, tempImageOut, 1);
            }
            break;

        case Operation_Rotation_SpecifiedDegrees: // Specify in degrees
        {
            int rotationDegrees = 0;
            if (m_angleInputMode == 0)
                rotationDegrees = static_cast<int>(GetIntegerProperty("angle"));

            else if (DataAvailableInFIFO(Input(1)))
            {
                MAPSIOElt* ioeltRot = StartReading(Input(1));
                rotationDegrees = static_cast<int>(ioeltRot->Integer32());
            }
            cv::Mat rotationMatrix = cv::Mat(2, 3, CV_32FC1);
            cv::Point2f center;
            center.x = imageIn.width / 2.0f;
            center.y = imageIn.height / 2.0f;

            rotationMatrix = cv::getRotationMatrix2D(center, rotationDegrees, 1.0);
            if (m_useGpu)
            {
                //Due to the copy of CPU memory to GPU memory, this part is not efficient but it's just an example
                cv::UMat in = tempImageIn.getUMat(cv::ACCESS_READ, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::UMat out = tempImageOut.getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
                cv::warpAffine(in, out, rotationMatrix, tempImageIn.size());
            }
            else
            {
                cv::warpAffine(tempImageIn, tempImageOut, rotationMatrix, tempImageIn.size());
            }
        }
        break;

        default:
            Error("Unknown operation.");
        }
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }

    if (static_cast<void*>(tempImageOut.data) != static_cast<void*>(imageOut.imageData)) // if the ptr are different then opencv reallocated memory for the cv::Mat
        Error("cv::Mat data ptr and imageOut data ptr are different.");

    outGuard.VectorSize() = 0;
    outGuard.Timestamp() = ts;
}
