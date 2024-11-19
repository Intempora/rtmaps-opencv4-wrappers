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

#pragma once

// Includes maps sdk library header
#include "maps/input_reader/maps_input_reader.hpp"
#include "maps_OpenCV_Conversion.h"

// Declares a new MAPSComponent child class
class MAPSOpenCV_Calibration
: public  MAPSComponent
{
    enum OperationMode
    {
        OperationMode_CalibSaveUndistort = 0,
        OperationMode_LoadUndistort      = 1
    };

    enum CaptureMode
    {
        CaptureMode_Periodic  = 0,
        CaptureMode_Triggered = 1
    };

    // Use standard header definition macro
    MAPS_COMPONENT_STANDARD_HEADER_CODE(MAPSOpenCV_Calibration)
    MAPS_COMPONENT_DYNAMIC_HEADER_CODE(MAPSOpenCV_Calibration)


    void UpdateFolderPathProperty();
    void UpdateFilePathProperty();
    void CreateSubFolder();
private:
    // Place here your specific methods and attributes
    bool m_calibrated;
    OperationMode m_operationMode;
    int m_captureMode;
    bool m_createSubfolders;
    bool m_saveCalibrationImages;
    MAPSDelay m_capturePeriodUs;
    int m_nBoards;
    int m_boardW;
    int m_boardH;
    int m_boardTotal;
    int m_successes;
    double m_rectWidth;
    double m_rectHeight;

    std::unique_ptr<MAPS::InputReader> m_inputReader;

    MAPSAbsoluteTime m_dateUtc;

    MAPSString m_folderPath;
    MAPSString m_filePath;
    int m_undistMode;
    double m_alpha;

    cv::Size m_boardSz;
    cv::Size m_imageSize;

    cv::Mat m_intrinsicMatrix;
    cv::Mat m_distortionCoeffs;
    double  m_reprojectionError;

    std::vector<cv::Mat> m_extrinsicMatrices;
    std::vector<cv::Mat> m_rvecs;
    std::vector<cv::Mat> m_tvecs;

    std::vector<cv::Point3f> m_realGrid;

    std::vector<std::vector<cv::Point2f> > m_imagePoints;
    std::vector<std::vector<cv::Point3f> > m_objectPoints;

    void Core_Calibrate_Actual(MAPSTimestamp ts, const IplImage& imageIn);
    void UpdateOperationModeProperty();
    void UpdateCaptureModeProperty();
    void ComputeRealGrid();
    void AllocateBuffers(const IplImage& iplImageIn);
    bool DetectPattern(cv::Mat& grayImage, std::vector<cv::Point2f>& detectedCorners);
    void CollectImage(MAPSTimestamp ts, const IplImage& iplImageIn);
    void SaveCollectedImage(cv::Mat& original, cv::Mat& withCorners);
    void CalibrateCamera();
    void SaveCalibration();
    void LoadCalibration();
    void UndistortImage(MAPSTimestamp ts, const IplImage& iplImageIn);
    void WriteCalibrationData(MAPSTimestamp ts);

    void AllocateOutputBufferSize_Triggered(const MAPSTimestamp /*ts*/, const MAPS::ArrayView <MAPS::InputElt<>> inElts);
    void ProcessData_Triggered(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<>> inElts);
    void AllocateOutputBufferSize_Periodic(const MAPSTimestamp /*ts*/, const MAPS::InputElt<IplImage> inElt);
    void ProcessData_Periodic(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt);
    void ProcessData_Reactive(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt);
};
