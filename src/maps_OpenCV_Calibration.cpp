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
// Purpose of this module :	Camera Calibration and Image undistorsion
// @ Bogdan Stanciulescu 18/02/2014
////////////////////////////////

#include "maps_OpenCV_Calibration.h"    // Includes the header of this component

#include <cstdio>  // sprintf...

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_Calibration)
    MAPS_INPUT("trigger", MAPS::FilterAny, MAPS::FifoReader)
    MAPS_INPUT("imageIn_fifo", MAPS::FilterIplImage, MAPS::LastOrNextReader)
    MAPS_INPUT("imageIn_sampling", MAPS::FilterIplImage, MAPS::SamplingReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_Calibration)
    MAPS_OUTPUT("oImage", MAPS::IplImage, nullptr, nullptr, 0)
    MAPS_OUTPUT("intrinsic_matrix", MAPS::Matrix, nullptr, nullptr, 0)
    MAPS_OUTPUT("distortion_coeffs", MAPS::Matrix, nullptr, nullptr, 0)
    MAPS_OUTPUT("reprojection_error", MAPS::Float64, nullptr, nullptr, 1)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_Calibration)
    MAPS_PROPERTY_ENUM("mode", "Run calibration procedure, save calib files and undistort|Load calib files and undistort", 0, false, false)

    // Run calibration procedure, save calib files and undistort
    MAPS_PROPERTY("number_of_snapshots_of_the_chessboard", 8, false, false)
    MAPS_PROPERTY("enclosed_corners_horizontally_on_the_chessboard", 8, false, false)
    MAPS_PROPERTY("enclosed_corners_vertically_on_the_chessboard", 5, false, false)
    MAPS_PROPERTY("pattern_rectangle_width_meters", 0.032, false, false)
    MAPS_PROPERTY("pattern_rectangle_height_meters", 0.032, false, false)
    MAPS_PROPERTY_SUBTYPE("folder_path", static_cast<const char*>(nullptr), false, true, MAPS::PropertySubTypePath|MAPS::PropertySubTypeMustExist)
    MAPS_PROPERTY("create_subfolders", true, false, false)
    MAPS_PROPERTY("save_calibration_images", true, false, false)

    MAPS_PROPERTY_ENUM("capture_mode", "Periodic|Triggered", 0, false, false)
    MAPS_PROPERTY("capture_period_ms", 5000, false, false)

    // Load calib files and undistort
    MAPS_PROPERTY_SUBTYPE("file_path", static_cast<const char*>(nullptr), false, true, MAPS::PropertySubTypeFile|MAPS::PropertySubTypeMustExist)
    MAPS_PROPERTY_ENUM("undist_mode","Default|No cropping (all pixels, extra black zones - alpha = 1.0)|Optimized cropping (minimum unwanted pixels, pixels removed at corners - alpha = 0.0)|Custom",0, false, false)
    MAPS_PROPERTY("alpha",0.0,false,false)
MAPS_END_PROPERTIES_DEFINITION

#define UNDIST_MODE_DEFAULT     0
#define UNDIST_MODE_NO_CROPPING 1
#define UNDIST_MODE_CROPPING    2
#define UNDIST_MODE_CUSTOM      3

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_Calibration)
    //MAPS_ACTION("aName",MAPSOpenCV_Calibration::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (Calibration) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_Calibration, "OpenCV_Calibration", "2.2.0", 128,
              MAPS::Threaded, MAPS::Threaded,
               0, // Nb of inputs. Leave -1 to use the number of declared input definitions
              -1, // Nb of outputs. Leave -1 to use the number of declared output definitions
               1, // Nb of properties. Leave -1 to use the number of declared property definitions
              -1) // Nb of actions. Leave -1 to use the number of declared action definitions

void MAPSOpenCV_Calibration::UpdateOperationModeProperty()
{
    switch (GetEnumProperty("mode").GetSelected())
    {
    case 0:
        m_operationMode = OperationMode_CalibSaveUndistort;

        NewProperty("number_of_snapshots_of_the_chessboard");
        NewProperty("enclosed_corners_horizontally_on_the_chessboard");
        NewProperty("enclosed_corners_vertically_on_the_chessboard");
        NewProperty("pattern_rectangle_width_meters");
        NewProperty("pattern_rectangle_height_meters");
        NewProperty("folder_path");
        NewProperty("create_subfolders");
        NewProperty("save_calibration_images");
        NewProperty("capture_mode");
        break;
    case 1:
        m_operationMode = OperationMode_LoadUndistort;

        NewProperty("file_path");
        NewProperty("undist_mode");
        m_undistMode = (int)GetIntegerProperty("undist_mode");
        if (m_undistMode == UNDIST_MODE_CUSTOM)
        {
            NewProperty("alpha");
            m_alpha = GetFloatProperty("alpha");
        }

        break;
    }
}

void MAPSOpenCV_Calibration::UpdateCaptureModeProperty()
{
    m_captureMode = static_cast<int>(GetIntegerProperty("capture_mode"));
    switch (m_captureMode)
    {
    case CaptureMode_Periodic:
        NewProperty("capture_period_ms");
        m_capturePeriodUs = static_cast<MAPSDelay>(GetIntegerProperty("capture_period_ms") * 1000);
        break;
    case CaptureMode_Triggered:
        NewInput("trigger");
        break;
    }
}

void MAPSOpenCV_Calibration::Dynamic()
{
    UpdateOperationModeProperty();

    switch (m_operationMode)
    {
    case OperationMode_CalibSaveUndistort:
        m_createSubfolders       = GetBoolProperty("create_subfolders");
        m_saveCalibrationImages = GetBoolProperty("save_calibration_images");

        UpdateFolderPathProperty();
        UpdateCaptureModeProperty();

        // pattern info
        m_rectWidth  = GetFloatProperty("pattern_rectangle_width_meters");
        m_rectHeight = GetFloatProperty("pattern_rectangle_height_meters");

        m_nBoards   = static_cast<int>(GetIntegerProperty("number_of_snapshots_of_the_chessboard"));
        m_boardW    = static_cast<int>(GetIntegerProperty("enclosed_corners_horizontally_on_the_chessboard"));
        m_boardH    = static_cast<int>(GetIntegerProperty("enclosed_corners_vertically_on_the_chessboard"));

        NewInput(2, "imageIn");
        break;

    case OperationMode_LoadUndistort:
        UpdateFilePathProperty();
        NewInput(1, "imageIn");
        break;
    }

}

void MAPSOpenCV_Calibration::UpdateFolderPathProperty()
{
    if (m_operationMode == OperationMode_CalibSaveUndistort)
    {
        m_folderPath = GetStringProperty("folder_path").Beginning();
        if (m_folderPath.Len() == 0)
        {
            m_folderPath = MAPS::GetUserHomeFolder();
            DirectSet(Property("folder_path"),m_folderPath);
        }
        if (m_createSubfolders)
        {
            MAPS::GetAbsoluteTimeUTC(&m_dateUtc);
            MAPSStreamedString sx;
            char dateString[4 + 2 + 2 + 1 + 2 + 2 + 2 + 3 + 1];
            sprintf(dateString,
                "%04u%02u%02u_%02u%02u%02u%03u",
                m_dateUtc.year, m_dateUtc.month, m_dateUtc.day,
                m_dateUtc.hour, m_dateUtc.minutes, m_dateUtc.seconds, m_dateUtc.milliseconds
            );
            sx << m_folderPath << "/" << dateString;
            m_folderPath = sx;
        }
    }
}

void MAPSOpenCV_Calibration::UpdateFilePathProperty()
{
    m_filePath = GetStringProperty("file_path").Beginning();
    if (m_filePath.Len() == 0)
    {
        m_filePath = MAPS::GetUserHomeFolder() + "/calibration.xml";
        DirectSet(Property("file_path"),m_filePath);
    }
}

void MAPSOpenCV_Calibration::CreateSubFolder()
{
    if (m_operationMode == OperationMode_CalibSaveUndistort && m_createSubfolders)
    {
        MAPSIconv::localeChar* path = MAPSIconv::UTF8ToLocale(m_folderPath);
        if (!MAPS::CreateFolder(path))
        {
            MAPSIconv::releaseLocale(path);
            MAPSStreamedString sx;
            MAPS::ReportError(sx << "Failed to create subdirectory [" << path << "]");
            return;
        }
        MAPSIconv::releaseLocale(path);
    }
}

void MAPSOpenCV_Calibration::Birth()
{
    try
    {
        UpdateFolderPathProperty();
        CreateSubFolder();

        m_calibrated = false;

        if (m_operationMode == OperationMode_CalibSaveUndistort)
        {
            m_successes = 0;

            m_boardTotal = m_boardW * m_boardH;
            m_boardSz = cv::Size(m_boardW, m_boardH);

            m_imagePoints.clear();  m_imagePoints.reserve(m_nBoards);
            m_objectPoints.clear(); m_objectPoints.reserve(m_nBoards);
            ComputeRealGrid();

            m_rvecs.clear(); m_rvecs.reserve(m_nBoards);
            m_tvecs.clear(); m_tvecs.reserve(m_nBoards);

            switch (m_captureMode)
            {
            case CaptureMode_Triggered:
                m_inputReader = MAPS::MakeInputReader::Triggered(
                    this,
                    Input("trigger"),
                    MAPS::InputReaderOption::Triggered::TriggerKind::DataInput,
                    MAPS::InputReaderOption::Triggered::SamplingBehavior::WaitForAllInputs,
                    MAPS::MakeArray(&Input(0), &Input(1)),
                    &MAPSOpenCV_Calibration::AllocateOutputBufferSize_Triggered,
                    &MAPSOpenCV_Calibration::ProcessData_Triggered
                );
                break;
            case CaptureMode_Periodic:
                m_inputReader = MAPS::MakeInputReader::PeriodicSampling(
                    this,
                    m_capturePeriodUs,
                    Input("imageIn"),
                    &MAPSOpenCV_Calibration::AllocateOutputBufferSize_Periodic,
                    &MAPSOpenCV_Calibration::ProcessData_Periodic
                );
                break;
            default:
                break;
            }
        }
        else
        {
            m_inputReader = MAPS::MakeInputReader::Reactive(
                this,
                Input("imageIn"),
                &MAPSOpenCV_Calibration::AllocateOutputBufferSize_Periodic,
                &MAPSOpenCV_Calibration::ProcessData_Reactive
            );
        }

        m_intrinsicMatrix = cv::Mat::eye(3, 3, CV_64F);
        m_distortionCoeffs = cv::Mat::zeros(5, 1, CV_64F);
        m_reprojectionError = 0.0;
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

void MAPSOpenCV_Calibration::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_Calibration::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_Calibration::Core_Calibrate_Actual(MAPSTimestamp ts, const IplImage& imageIn)
{
    if (m_successes < m_nBoards)
    {
        CollectImage(ts, imageIn);
    }

    if (m_successes >= m_nBoards && !m_calibrated)
    {
        CalibrateCamera();
        SaveCalibration();

        m_calibrated = true;

        ReportInfo("Starting image correction...");
    }
}

void MAPSOpenCV_Calibration::ComputeRealGrid()
{
    // compute the coordinates of the vertices in the real world
    m_realGrid.clear();
    for (int row = 0; row < m_boardH; ++row)
    {
        for (int col = 0; col < m_boardW; ++col)
        {
            m_realGrid.push_back(
                cv::Point3f(
                    static_cast<float>(col * m_rectWidth),
                    static_cast<float>(row * m_rectHeight),
                    0.f
                )
            );
        }
    }
}

void MAPSOpenCV_Calibration::AllocateBuffers(const IplImage& iplImageIn)
{
    const MAPSUInt32 imageInChannelSeq = *reinterpret_cast<const MAPSUInt32*>(iplImageIn.channelSeq);

    //Example of how to test the incoming images color format,
    //and abort if the input images color format is not supported.
    if (imageInChannelSeq != MAPS_CHANNELSEQ_GRAY &&
        imageInChannelSeq != MAPS_CHANNELSEQ_BGR &&
        imageInChannelSeq != MAPS_CHANNELSEQ_RGB &&
        imageInChannelSeq != MAPS_CHANNELSEQ_BGRA &&
        imageInChannelSeq != MAPS_CHANNELSEQ_RGBA)
    {
        //The Error method will throw an exception, which will be caught by the RTMaps
        //engine outside of the Core() function, so that execution of this component
        //will stop and the Death function will be called immediately.
        //The error message is displayed in red in the console as it is with the ReportError method.
        //(All the other components in the diagram keep-on working).
        ReportError("Unsupported image format.");
    }

    Output(0).AllocOutputBufferIplImage(iplImageIn);

    Output("intrinsic_matrix").AllocOutputBufferMatrix(3, 3);
    Output("distortion_coeffs").AllocOutputBufferMatrix(5, 1);
}

bool MAPSOpenCV_Calibration::DetectPattern(cv::Mat& grayImage, std::vector<cv::Point2f>& detectedCorners)
{
    detectedCorners.clear();
    detectedCorners.reserve(m_boardTotal);

    const bool found = cv::findChessboardCorners(
        grayImage,
        m_boardSz,
        detectedCorners,
        cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_FILTER_QUADS + cv::CALIB_CB_NORMALIZE_IMAGE
    );

    if (found)
    {
        cv::cornerSubPix(
            grayImage,
            detectedCorners,
            cv::Size(11, 11),
            cv::Size(-1, -1),
            cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1)
        );
    }

    return found;
}

void MAPSOpenCV_Calibration::CollectImage(MAPSTimestamp ts, const IplImage& iplImageIn)
{
    const MAPSUInt32 imageInChannelSeq = *(MAPSUInt32*)iplImageIn.channelSeq;
    cv::Mat          imageIn = convTools::noCopyIplImage2Mat(&iplImageIn);

    ReportInfo("Collecting images...\n");

    cv::Mat gray_image;

    switch (imageInChannelSeq)
    {
        case MAPS_CHANNELSEQ_BGR: cv::cvtColor(imageIn, gray_image, cv::COLOR_BGR2GRAY); break;
        case MAPS_CHANNELSEQ_RGB: cv::cvtColor(imageIn, gray_image, cv::COLOR_RGB2GRAY); break;
        case MAPS_CHANNELSEQ_BGRA: cv::cvtColor(imageIn, gray_image, cv::COLOR_BGRA2GRAY); break;
        case MAPS_CHANNELSEQ_RGBA: cv::cvtColor(imageIn, gray_image, cv::COLOR_RGBA2GRAY); break;
        default: gray_image = imageIn; break;
    }

    std::vector<cv::Point2f> detectedCorners;
    const bool found = DetectPattern(gray_image, detectedCorners);

    // If we got a good board, add it to our data
    if (found && detectedCorners.size() == m_boardTotal)
    {
        ++m_successes;

        m_imagePoints.push_back(detectedCorners);
        m_objectPoints.push_back(m_realGrid);

        MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
        outGuard.Timestamp() = ts;
        outGuard.VectorSize() = 0;

        cv::Mat imageOut = convTools::noCopyIplImage2Mat(&outGuard.Data());
        imageIn.copyTo(imageOut);
        cv::drawChessboardCorners(imageOut, m_boardSz, cv::Mat(detectedCorners), found);

        SaveCollectedImage(imageIn, imageOut);

        MAPSStreamedString sx;
        ReportInfo(sx
            << m_successes << " successful Snapshots out of " << m_nBoards << " collected."
        );
    }
    else
    {
        ReportInfo("No corners detected");
    }
}

void MAPSOpenCV_Calibration::SaveCollectedImage(cv::Mat& original, cv::Mat& withCorners)
{
    if (m_saveCalibrationImages)
    {
        MAPSIconv::localeChar* path = MAPSIconv::UTF8ToLocale(m_folderPath);
        char buf[512];

        sprintf(buf, "%s/original_%04d-%04d.png", path, m_successes, m_nBoards);
        if (!cv::imwrite(buf, original))
        {
            MAPSIconv::releaseLocale(path);
            MAPSStreamedString sx;
            Error(sx << "Failed to save image [" << buf << "]");
        }

        sprintf(buf, "%s/corners_%04d-%04d.png", path, m_successes, m_nBoards);
        if (!cv::imwrite(buf, withCorners))
        {
            MAPSIconv::releaseLocale(path);
            MAPSStreamedString sx;
            Error(sx << "Failed to save image [" << buf << "]");
        }

        MAPSIconv::releaseLocale(path);
    }
}

void MAPSOpenCV_Calibration::CalibrateCamera()
{
    ReportInfo("\n\n *** Calibrating the camera now...\n");

    m_intrinsicMatrix = cv::Mat::eye(3, 3, CV_64F);
    m_distortionCoeffs = cv::Mat::zeros(8, 1, CV_64F);
    m_reprojectionError = cv::calibrateCamera(
        m_objectPoints,
        m_imagePoints,
        m_imageSize,
        m_intrinsicMatrix,
        m_distortionCoeffs,
        m_rvecs,
        m_tvecs
    );

    //_____________________________________________________________________________________

    //Extrinsic parameters

    //cv::solvePnP(m_objectPoints2,m_imagePoints2, m_intrinsicMatrix,m_distortionCoeffs,m_rvecs,m_tvecs);

    //convert the rotation matrix into a rotation vector
    //cv::Rodrigues(m_rvec,m_rotmat);

    // Save the intrinsics, distortions and extrinsic

    //Save values to file
    ReportInfo(" *** Calibration Done!\n\n");
}

void MAPSOpenCV_Calibration::SaveCalibration()
{
    MAPSStreamedString sx;
    ReportInfo(sx
        << "Storing calibration.xml file in folder: " << m_folderPath << "..."
    );

    {
        MAPSIconv::localeChar* path_locale = MAPSIconv::UTF8ToLocale(static_cast<const char*>(m_folderPath + "/calibration.xml"));
        cv::FileStorage fs(path_locale, cv::FileStorage::WRITE);
        MAPSIconv::releaseLocale(path_locale);

        char dateString[128];
        sprintf(dateString,
            "%04u/%02u/%02u %02u:%02u:%02u:%03u",
            m_dateUtc.year, m_dateUtc.month, m_dateUtc.day,
            m_dateUtc.hour, m_dateUtc.minutes, m_dateUtc.seconds, m_dateUtc.milliseconds
        );

        fs << "calibration_date" << dateString

            << "frame_count" << m_nBoards

            << "image_width" << m_imageSize.width
            << "image_height" << m_imageSize.height

            << "board_width" << m_boardW
            << "board_height" << m_boardH

            << "rectangle_width" << m_rectWidth
            << "rectangle_height" << m_rectHeight

            << "intrinsic_matrix" << m_intrinsicMatrix
            << "distortion_coeffs" << m_distortionCoeffs
            << "reprojection_error" << m_reprojectionError

            << "rvecs" << m_rvecs
            << "tvecs" << m_tvecs;

        fs.release();
    }

    ReportInfo("Files saved.\n\n");
}

void MAPSOpenCV_Calibration::LoadCalibration()
{
    if (!m_calibrated)
    {
        MAPSIconv::localeChar* path_locale = MAPSIconv::UTF8ToLocale(static_cast<const char*>(m_filePath));
        cv::FileStorage fs(path_locale, cv::FileStorage::READ);
        MAPSIconv::releaseLocale(path_locale);
        if (fs.isOpened())
        {
            fs["intrinsic_matrix"] >> m_intrinsicMatrix;
            fs["distortion_coeffs"] >> m_distortionCoeffs;
            fs["reprojection_error"] >> m_reprojectionError;

            m_calibrated = true;
            fs.release();
        }
        else
        {
            MAPSStreamedString sx;
            Error(sx
                << "Failed to load the calibration data from [" << m_filePath << "]"
            );
        }
    }
}

void MAPSOpenCV_Calibration::UndistortImage(MAPSTimestamp ts, const IplImage& iplImageIn)
{
    cv::Mat imageIn = convTools::noCopyIplImage2Mat(&iplImageIn);
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    cv::Mat imageOut = convTools::noCopyIplImage2Mat(&outGuard.Data());
    switch (m_undistMode)
    {
        case UNDIST_MODE_DEFAULT:
            cv::undistort(imageIn, imageOut, m_intrinsicMatrix, m_distortionCoeffs);
            break;
        case UNDIST_MODE_NO_CROPPING:
        {
            cv::Mat newMatrix = cv::getOptimalNewCameraMatrix(m_intrinsicMatrix, m_distortionCoeffs, imageIn.size(), 1.0, imageIn.size());
            cv::undistort(imageIn, imageOut, m_intrinsicMatrix, m_distortionCoeffs, newMatrix);
            break;
        }
        case UNDIST_MODE_CROPPING:
        {
            cv::Mat newMatrix = cv::getOptimalNewCameraMatrix(m_intrinsicMatrix, m_distortionCoeffs, imageIn.size(), 0.0, imageIn.size());
            cv::undistort(imageIn, imageOut, m_intrinsicMatrix, m_distortionCoeffs, newMatrix);
            break;
        }
        case UNDIST_MODE_CUSTOM:
        {
            cv::Mat newMatrix = cv::getOptimalNewCameraMatrix(m_intrinsicMatrix, m_distortionCoeffs, imageIn.size(), m_alpha);
            cv::undistort(imageIn, imageOut, m_intrinsicMatrix, m_distortionCoeffs, newMatrix);
            break;
        }
    }

    outGuard.Timestamp() = ts;
    outGuard.VectorSize() = 0;
}

void MAPSOpenCV_Calibration::WriteCalibrationData(MAPSTimestamp ts)
{
    MAPS::OutputGuard<MAPSMatrix> outGuard1{ this, Output("intrinsic_matrix") };
    MAPS::OutputGuard<MAPSMatrix> outGuard2{ this, Output("distortion_coeffs") };
    MAPS::OutputGuard<MAPSFloat64> outGuard3{ this, Output("reprojection_error") };

    MAPSMatrix&  intrinsic = outGuard1.Data();
    MAPSMatrix&  distortion = outGuard2.Data();
    MAPSFloat64& error = outGuard3.Data();

    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            intrinsic.Real(row, col) = m_intrinsicMatrix.at<double>(row, col);
            intrinsic.Im(row, col) = 0;
        }
    }

    for (int row = 0; row < 5; ++row)
    {
        distortion.Real(row, 0) = m_distortionCoeffs.at<double>(row, 0);
        distortion.Im(row, 0) = 0;
    }

    error = m_reprojectionError;

    outGuard1.Timestamp() = ts;
    outGuard2.Timestamp() = ts;
    outGuard3.Timestamp() = ts;
}

void MAPSOpenCV_Calibration::AllocateOutputBufferSize_Triggered(const MAPSTimestamp, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
{
    const IplImage& imageIn = inElts[1].DataAs<IplImage>();
    m_imageSize = cv::Size(imageIn.width, imageIn.height);
    AllocateBuffers(imageIn);
}

void MAPSOpenCV_Calibration::ProcessData_Triggered(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<>> inElts)
{
    try
    {
        if (m_operationMode == OperationMode_CalibSaveUndistort && !m_calibrated)
        {
            Core_Calibrate_Actual(ts, inElts[1].DataAs<IplImage>());
        }
        else
        {
            LoadCalibration();
            UndistortImage(ts, inElts[1].DataAs<IplImage>());
            WriteCalibrationData(ts);
        }
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

void MAPSOpenCV_Calibration::AllocateOutputBufferSize_Periodic(const MAPSTimestamp, const MAPS::InputElt<IplImage> inElt)
{
    const IplImage& imageIn = inElt.Data();
    m_imageSize = cv::Size(imageIn.width, imageIn.height);
    AllocateBuffers(imageIn);
}

void MAPSOpenCV_Calibration::ProcessData_Periodic(const MAPSTimestamp ts, MAPS::InputElt<IplImage> inElt)
{
    try
    {
        if (m_operationMode == OperationMode_CalibSaveUndistort && !m_calibrated)
        {
            Core_Calibrate_Actual(ts, inElt.Data());
        }
        else
        {
            LoadCalibration();
            UndistortImage(ts, inElt.Data());
            WriteCalibrationData(ts);
        }
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}

void MAPSOpenCV_Calibration::ProcessData_Reactive(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    try
    {
        LoadCalibration();
        UndistortImage(ts, inElt.Data());
        WriteCalibrationData(ts);
    }
    catch (const std::exception& e)
    {
        Error(e.what());
    }
}
