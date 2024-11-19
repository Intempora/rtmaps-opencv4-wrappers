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
// Date: 2022
////////////////////////////////

////////////////////////////////
// Purpose of this module : Object detection using Yolov4
// To get the necessary files please visit https://github.com/AlexeyAB/darknet
////////////////////////////////

#include "maps_OpenCV_Yolo.h"

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_Yolo)
    MAPS_INPUT("imageIn", MAPS::FilterIplImage, MAPS::FifoReader)
MAPS_END_INPUTS_DEFINITION

#define MAX_DOBJS_OUT 64
// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_Yolo)
	MAPS_OUTPUT("bounding_boxes",MAPS::DrawingObject, nullptr, nullptr, MAX_DOBJS_OUT)
	MAPS_OUTPUT("labels", MAPS::DrawingObject, nullptr, nullptr, MAX_DOBJS_OUT)
	MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_Yolo)
    MAPS_PROPERTY_SUBTYPE("cfg_path", "", false, false, MAPS::PropertySubTypeFile | MAPS::PropertySubTypeMustExist) // Path to the .cfg file
    MAPS_PROPERTY_SUBTYPE("names_path", "", false, false, MAPS::PropertySubTypeFile | MAPS::PropertySubTypeMustExist) // Path to the .names file
    MAPS_PROPERTY_SUBTYPE("weights_path", "", false, false, MAPS::PropertySubTypeFile | MAPS::PropertySubTypeMustExist) // Path to the .weights file
    MAPS_PROPERTY("confidence_threshold", 0.5, false, true) // A threshold to filter detection confidence
    MAPS_PROPERTY("nms_threshold", 0.4, false, true) // A threshold to filter overlapping detection boxes (non maximum suppression)
    MAPS_PROPERTY("text_thickness", 1.0, false, true) 
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_Yolo)
//MAPS_ACTION("aName",MAPSOpenCV_Resize::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_Resize) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_Yolo, "OpenCV_Yolo", "1.1.0", 128,
    MAPS::Threaded, MAPS::Threaded,
    -1, // Nb of inputs
    -1, // Nb of outputs
    -1, // Nb of properties
    -1) // Nb of actions

void MAPSOpenCV_Yolo::Birth()
{
    std::string namesPath(static_cast<const char*>(GetStringProperty("names_path")));
    std::string configPath(static_cast<const char*>(GetStringProperty("cfg_path")));
    std::string weightPath(static_cast<const char*>(GetStringProperty("weights_path")));

    if (namesPath.empty() || configPath.empty() || weightPath.empty())
        Error("Please specify the needed files (.weights, .cfg and .names");

    //Get the names of the objects it was trained for
    m_classes.clear();
    std::ifstream file(namesPath);
    std::string line;
    while (std::getline(file, line)) {
        m_classes.push_back(line);
    }

    //Create the model thanks to the config and the weight files
    m_model = cv::dnn::DetectionModel(configPath, weightPath);

    m_inputReader = MAPS::MakeInputReader::Reactive(
        this,
        Input(0),
        &MAPSOpenCV_Yolo::AllocateOutputBufferSize,  // Called when data is received for the first time only
        &MAPSOpenCV_Yolo::ProcessData      // Called when data is received for the first time AND all subsequent times
    );
}

void MAPSOpenCV_Yolo::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_Yolo::Death()
{
    m_inputReader.reset();
}

void MAPSOpenCV_Yolo::AllocateOutputBufferSize(const MAPSTimestamp, const MAPS::InputElt<IplImage> imageInElt)
{
    const IplImage& imageIn = imageInElt.Data();

    //Set the model inputs with the image size
    m_model.setInputParams(1 / 255.0, cv::Size(imageIn.width, imageIn.height), cv::Scalar(), true);
}

void MAPSOpenCV_Yolo::ProcessData(const MAPSTimestamp ts, const MAPS::InputElt<IplImage> inElt)
{
    try
    {
        const IplImage& imageIn = inElt.Data();
		MAPS::OutputGuard<MAPSDrawingObject> outGuardBB{ this, Output(0) };
		MAPS::OutputGuard<MAPSDrawingObject> outGuardLabels{ this, Output(1) };

        cv::Mat cvImageIn = convTools::noCopyIplImage2Mat(&imageIn);

        std::vector<int> classIds;
        std::vector<float> scores;
        std::vector<cv::Rect> boxes;
        float conf_thres = static_cast<float>(GetFloatProperty("confidence_threshold"));
        float nms_thres = static_cast<float>(GetFloatProperty("nms_threshold"));

		// Create a new image using the data of the input image, the new size and the interpollation method choose
		//Detect the known objects in the image with a dedicated box and confidence score
		m_model.detect(cvImageIn, classIds, scores, boxes, conf_thres, nms_thres);

        //For all the detected objetcs
        int n_objs = MIN(static_cast<int>(classIds.size()), MAX_DOBJS_OUT);
        for (int i = 0; i < n_objs; i++) {

            std::string label = cv::format("%.2f", scores[i]);
            label = m_classes[classIds[i]] + ":" + label;

			MAPSDrawingObject& bb = outGuardBB.Data(i);
			MAPS::Memset(&bb, 0, sizeof(MAPSDrawingObject));
			bb.kind = MAPSDrawingObject::Rectangle;
			bb.id = classIds[i];
			bb.color = MAPS_RGB(s_label_colors[classIds[i] % NB_LABEL_COLORS][0], s_label_colors[classIds[i] % NB_LABEL_COLORS][1], s_label_colors[classIds[i] % NB_LABEL_COLORS][2]);
			bb.width = 2;
			bb.rectangle.x1 = boxes[i].x;
			bb.rectangle.x2 = boxes[i].width + boxes[i].x;
			bb.rectangle.y1 = boxes[i].y;
			bb.rectangle.y2 = boxes[i].y + boxes[i].height;

			MAPSDrawingObject& label_dobj = outGuardLabels.Data(i);
			MAPS::Memset(&label_dobj, 0, sizeof(MAPSDrawingObject));
			label_dobj.kind = MAPSDrawingObject::Text;
			label_dobj.id = classIds[i];
			label_dobj.color = bb.color;
			label_dobj.width = 2;
			label_dobj.text.x = bb.rectangle.x1 + 10;
			label_dobj.text.y = bb.rectangle.y1 + 10;
			label_dobj.text.cheight = 10;
			label_dobj.text.cwidth = 10;
			MAPS::Strcpy(label_dobj.text.text, label.c_str());
        }

		outGuardBB.VectorSize() = n_objs;
		outGuardLabels.VectorSize() = n_objs;
		outGuardBB.Timestamp() = ts;
		outGuardLabels.Timestamp() = ts;
    }
    catch (std::exception& e)
    {
        Error(e.what());
    }
}
