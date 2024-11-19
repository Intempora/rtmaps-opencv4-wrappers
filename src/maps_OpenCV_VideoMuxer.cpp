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
// Purpose of this module : This component allows to concatenate a number of source videos into a single one
//                          with just specifying the destination position and optionally the destination size of each input stream.
////////////////////////////////

#include "maps_OpenCV_VideoMuxer.h" // Includes the header of this component

// Use the macros to declare the inputs
MAPS_BEGIN_INPUTS_DEFINITION(MAPSOpenCV_VideoMuxer)
    MAPS_INPUT("imageIn_fifo", MAPS::FilterIplImage, MAPS::FifoReader)
    MAPS_INPUT("imageIn_sampling", MAPS::FilterIplImage, MAPS::SamplingReader)
MAPS_END_INPUTS_DEFINITION

// Use the macros to declare the outputs
MAPS_BEGIN_OUTPUTS_DEFINITION(MAPSOpenCV_VideoMuxer)
    MAPS_OUTPUT_FIFOSIZE("imageOut", MAPS::IplImage, nullptr, nullptr, 0, 8)
MAPS_END_OUTPUTS_DEFINITION

// Use the macros to declare the properties
MAPS_BEGIN_PROPERTIES_DEFINITION(MAPSOpenCV_VideoMuxer)
    MAPS_PROPERTY("nb_inputs", 2, false, false)
    MAPS_PROPERTY_ENUM("sampling_mode", "Triggered|Synchronized|Asynchronous (update at each img recv)", 0, false, false)
    MAPS_PROPERTY_ENUM("trigger_input", "None", 0, false, false)
    MAPS_PROPERTY("synchro_tolerance", 0, false, false)
    MAPS_PROPERTY("out_image_width", -1, false, false)
    MAPS_PROPERTY("out_image_height", -1, false, false)
    MAPS_PROPERTY_BEGIN_SUBSECTION("subsection_begin", "")
    MAPS_PROPERTY("left", 0, false, true)
    MAPS_PROPERTY("top", 0, false, true)
    MAPS_PROPERTY("width", -1, false, true)
    MAPS_PROPERTY("height", -1, false, true)
    MAPS_PROPERTY("z_order", -1, false, true)
    MAPS_PROPERTY_END_SUBSECTION("subsection_end_opened", true)
MAPS_END_PROPERTIES_DEFINITION

// Use the macros to declare the actions
MAPS_BEGIN_ACTIONS_DEFINITION(MAPSOpenCV_VideoMuxer)
    //MAPS_ACTION("aName",MAPSOpenCV_VideoMuxer::ActionName)
MAPS_END_ACTIONS_DEFINITION

// Use the macros to declare this component (OpenCV_VideoMuxer) behaviour
MAPS_COMPONENT_DEFINITION(MAPSOpenCV_VideoMuxer, "OpenCV_VideoMuxer", "2.0.3", 128,
                             MAPS::Sequential | MAPS::Threaded, MAPS::Threaded,
                             0, // Nb of inputs
                            -1, // Nb of outputs
                             2, // Nb of properties
                            -1) // Nb of actions

enum ReaderMode : uint8_t
{
    ReaderMode_Triggered,
    ReaderMode_Synchronized,
    ReaderMode_Async
};

enum Property : uint8_t
{
    Property_ImageName = 6,
    Property_Left,
    Property_Top,
    Property_Width,
    Property_Height,
    Property_ZOrder,
    Property_SubSectionEnd,
    Property_NumberOfProperties = 7
};

void MAPSOpenCV_VideoMuxer::Dynamic()
{
    m_nbInputs = static_cast<int>(GetIntegerProperty("nb_inputs"));
    m_readersMode = static_cast<int>(GetIntegerProperty("sampling_mode"));

    switch (m_readersMode)
    {
    case ReaderMode_Triggered:
        NewProperty("trigger_input");
        m_firstPositionPropRuntime = 5;
        m_trigger = static_cast<int>(GetIntegerProperty("trigger_input"));
        break;
    case ReaderMode_Synchronized:
        NewProperty("synchro_tolerance");
        m_firstPositionPropRuntime = 5;
        break;
    case ReaderMode_Async:
        m_firstPositionPropRuntime = 4;
        break;
    default:
        Error("Reader mode not supported");
        break;
    }

    NewProperty("out_image_width");
    NewProperty("out_image_height");

    //Store the current status of the trigger_input prop.
    MAPSEnumStruct trigger_enum;

    m_inputs.clear();
    //Create the necessary inputs and properties.
    for (int i = 0; i < m_nbInputs; i++)
    {
        int nb = i + 1;
        MAPSStreamedString iname;
        iname << "image_in_" << nb;

        switch (m_readersMode)
        {
        case ReaderMode_Triggered:
            if (i == 0)
            {
                m_inputs.push_back(&NewInput(0, iname));
            }
            else
            {
                m_inputs.push_back(&NewInput(1, iname));
            }
            trigger_enum.enumValues->Append() = iname;
            break;
        case ReaderMode_Synchronized:
        case ReaderMode_Async:
            m_inputs.push_back(&NewInput(0, iname));
            break;
        }

        MAPSStreamedString im_name, im_pretty_name, xpos_name, ypos_name, width_name, height_name, z_o_name, end_sub_name;
        im_name << "image_" << nb;
        im_pretty_name << "Image " << nb;
        xpos_name << "image_" << nb << "_left";
        ypos_name << "image_" << nb << "_top";
        width_name << "image_" << nb << "_width";
        height_name << "image_" << nb << "_height";
        z_o_name << "image_" << nb << "_z_order";
        end_sub_name << "image_" << nb << "_end";
        DirectSet(NewProperty(Property_ImageName, im_name), im_pretty_name);
        NewProperty(Property_Left, xpos_name);
        NewProperty(Property_Top, ypos_name);
        NewProperty(Property_Width, width_name);
        NewProperty(Property_Height, height_name);
        NewProperty(Property_ZOrder, z_o_name);
        NewProperty(Property_SubSectionEnd, end_sub_name);
        if (GetIntegerProperty(i*Property_NumberOfProperties + m_firstPositionPropRuntime + 5) == -1)
            DirectSet(Property(i*Property_NumberOfProperties + m_firstPositionPropRuntime + 5), nb);
    }

    //Fill in the trigger_input property enumerations.
    if (m_readersMode == ReaderMode_Triggered)
    {
        if (m_nbInputs == 0)
            trigger_enum.enumValues->Append() = "None";
        if (m_trigger >= trigger_enum.enumValues->Size())
            m_trigger = 0;
        trigger_enum.selectedEnum = m_trigger;
        DirectSet(Property("trigger_input"), trigger_enum);
    }
}

void MAPSOpenCV_VideoMuxer::Birth()
{
    //Initialize member variables
    m_ioEltImage.SetSize(m_nbInputs);
    m_sizeInitialized.resize(m_nbInputs);
    m_posX.resize(m_nbInputs);
    m_posY.resize(m_nbInputs);
    m_width.resize(m_nbInputs);
    m_height.resize(m_nbInputs);
    m_zOrder.resize(m_nbInputs);
    m_ioeltsOrder.resize(m_nbInputs);
    m_firstTimeAllInit = true;
    m_outputInitialized = false;
    m_outmWidth = static_cast<int>(GetIntegerProperty("out_image_width"));
    m_outmHeight = static_cast<int>(GetIntegerProperty("out_image_height"));
    m_bgInitialized.clear();

    for (int i = 0; i < m_nbInputs; i++)
    {
        m_ioEltImage[i] = nullptr;
        m_posX[i] = static_cast<int>(GetIntegerProperty(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 1));
        m_posY[i] = static_cast<int>(GetIntegerProperty(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 2));
        m_width[i] = static_cast<int>(GetIntegerProperty(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 3));
        m_height[i] = static_cast<int>(GetIntegerProperty(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 4));
        m_zOrder[i] = static_cast<int>(GetIntegerProperty(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 5));
        if (m_width[i] >= 0 && m_height[i] >= 0)
            m_sizeInitialized[i] = true;
        else
            m_sizeInitialized[i] = false;
    }
    SortIOEltsByZOrder();
    m_allSizesInitialized = true;
    for (int i = 0; i < m_nbInputs; i++)
    {
        if (m_sizeInitialized[i] == false)
        {
            m_allSizesInitialized = false;
            break;
        }
    }

    switch (m_readersMode)
    {
    case ReaderMode_Synchronized:
        m_inputReader = MAPS::MakeInputReader::Synchronized(
            this,
            GetIntegerProperty("synchro_tolerance"),
            MAPS::InputReaderOption::Synchronized::SyncBehavior::SyncAllInputs,
            m_inputs,
            &MAPSOpenCV_VideoMuxer::Initialization,
            &MAPSOpenCV_VideoMuxer::ProcessData
        );
        break;
    case ReaderMode_Triggered:
        m_inputReader = MAPS::MakeInputReader::Triggered(
            this,
            Input(m_trigger),
            MAPS::InputReaderOption::Triggered::TriggerKind::DataInput,
            MAPS::InputReaderOption::Triggered::SamplingBehavior::AllowEmptyInputs,
            m_inputs,
            &MAPSOpenCV_VideoMuxer::Initialization_Trigerred,
            &MAPSOpenCV_VideoMuxer::ProcessData_Reactive
        );
        break;
    case ReaderMode_Async:
        m_inputReader = MAPS::MakeInputReader::Reactive(
            this,
            MAPS::InputReaderOption::Reactive::FirstTimeBehavior::Immediate,
            MAPS::InputReaderOption::Reactive::Buffering::Enabled,
            m_inputs,
            &MAPSOpenCV_VideoMuxer::Initialization_Reactive,
            &MAPSOpenCV_VideoMuxer::ProcessData_Reactive
        );
        break;
    case 3:
    default:
        Error("Unknown sampling mode");
    }
}

void MAPSOpenCV_VideoMuxer::Core()
{
    m_inputReader->Read();
}

void MAPSOpenCV_VideoMuxer::Death()
{
    m_inputReader.reset();
}

int compare_sortingelts(SortingElt* t1, SortingElt* t2)
{
    return t1->zorder - t2->zorder;
}

void MAPSOpenCV_VideoMuxer::ComputeSizesAndAllocOutBuffer()
{
    //Alloc output buffer.
    int max_right = 0, max_bottom = 0;
    for (int i = 0; i < m_nbInputs; i++)
    {
        int right = m_posX[i] + m_width[i];
        int bottom = m_posY[i] + m_height[i];

        if (right > max_right)
        {
            max_right = right;
        }

        if (bottom > max_bottom)
        {
            max_bottom = bottom;
        }
    }

    if (0 == max_right)
    {
        Error("Output image width <= 0. Please check image_N_left and image_N_width properties.");
    }
    if (0 == max_bottom)
    {
        Error("Output image height <= 0. Please check image_N_top and image_N_height properties.");
    }

    m_totalmWidth = max_right;
    m_totalmHeight = max_bottom;

    if (m_outmWidth <= 0)
    {
        m_outmWidth = m_totalmWidth;
    }

    if (m_outmHeight <= 0)
    {
        m_outmHeight = m_totalmHeight;
    }

    m_outNeedResize = (m_outmWidth != m_totalmWidth) || (m_outmHeight != m_totalmHeight);
    if (m_outNeedResize)
    {
        m_tempImage = MAPS::IplImageModel(m_totalmWidth, m_totalmHeight, *reinterpret_cast<MAPSUInt32*>(&m_chanSeq), m_dataOrder, m_depth, m_align);
        m_tempImageData.reserve(m_tempImage.imageSize);
        m_tempImage.imageData = m_tempImageData.data();

        m_tempImage.roi = &m_tempROI;

        MAPS::Memset(m_tempImage.imageData, 0, m_tempImage.imageSize);
        MAPS::Memset(m_tempImage.roi, 0, sizeof(IplROI));
        m_tempImage.roi->width = m_tempImage.width;
        m_tempImage.roi->height = m_tempImage.height;
    }

    IplImage model = MAPS::IplImageModel(m_outmWidth, m_outmHeight, *reinterpret_cast<MAPSUInt32*>(&m_chanSeq), m_dataOrder, m_depth, m_align);
    Output(0).AllocOutputBufferIplImage(model);
    m_black = cv::Mat(cv::Mat::zeros(cv::Size(m_totalmWidth, m_totalmHeight), CV_MAKETYPE(m_depth, model.nChannels)));
    m_outputInitialized = true;
}

void MAPSOpenCV_VideoMuxer::BackgroundInitialization(IplImage* imageOut)
{
    if (m_outNeedResize)
    {
        imageOut = &m_tempImage;
    }

    const std::lock_guard<std::mutex> lock(m_zorderMutex);
    //Did we already encounter this ioEltOut ?
    if (std::find(m_bgInitialized.begin(), m_bgInitialized.end(), imageOut->imageData) == m_bgInitialized.end())
    {
        //This is the first time we will write something in this output image.
        //Let's set the background to black...
        cv::copyTo(m_black, convTools::noCopyIplImage2Mat(imageOut), cv::noArray());

        //... and make sure we will not do that again. Once is enough.
        m_bgInitialized.push_back(imageOut->imageData);
    }
}

void MAPSOpenCV_VideoMuxer::OutputResultImage(MAPSTimestamp t, const MAPS::ArrayView<MAPS::InputElt<IplImage>>& inElts)
{
    if (!m_outputInitialized)
        return;
    MAPS::OutputGuard<IplImage> outGuard{ this, Output(0) };
    IplImage& imageOut = outGuard.Data(); // Convert IplImage to cv::Mat without copying
    outGuard.Timestamp() = t;

    BackgroundInitialization(&imageOut);

    IplImage intermediate_image;
    if (m_outNeedResize)
    {
        intermediate_image = m_tempImage;
    }
    else
    {
        intermediate_image = imageOut;
    }


    {
        const std::lock_guard<std::mutex> lock(m_zorderMutex);
        for (int i = 0; i < inElts.size(); i++)
        {
            int image_index = m_ioeltsOrder[i];
            if (!inElts[image_index].IsValid())
            {
                ReportWarning("Invalid data on input");
                continue;
            }

            const IplImage& imageIn = inElts[image_index].Data();
            bool resize = false;

            int pos_x = m_posX[image_index];
            int pos_y = m_posY[image_index];
            int width = m_width[image_index];
            if (width == -1)
            {
                width = imageIn.width;
            }
            else if (width <= 0)
            {
                ReportWarning("Image N width should be positive or -1 for auto");
                continue;
            }

            int height = m_height[image_index];
            if (height == -1)
            {
                height = imageIn.height;
            }
            else if (height <= 0)
            {
                ReportWarning("Image N height should be positive or -1 for auto");
                continue;
            }

            if (imageIn.width != width || imageIn.height != height)
                resize = true;

            if (pos_x > m_totalmWidth || pos_y > m_totalmHeight)
            {
                MAPSStreamedString s;
                s << "Input image [" << i << "] out of bounds of output image. Please check left and top properties.";
                ReportWarning(s);
                continue;
            }
            if ((pos_x + width < 0) || (pos_y + height) < 0)
            {
                MAPSStreamedString s;
                s << "Input image [" << i << "] out of bounds of output image. Please check left, top, width and height properties.";
                ReportWarning(s);
                continue;
            }

            IplROI temp_roi;
            intermediate_image.roi = &temp_roi;
            IplROI& out_roi = *(intermediate_image.roi);

            IplImage src_image = imageIn; //copy the header, not the data, this will allow to change the ROI.
            IplROI src_roi;
            src_roi.coi = 0;
            src_image.roi = &src_roi; // Use a temporary ROI to avoid modifying the input image ROI


            if (!resize)
            {
                src_roi.xOffset = pos_x < 0 ? -pos_x : 0; // In case the input image should be offset to the left, we cut out the left part of it
                src_roi.yOffset = pos_y < 0 ? -pos_y : 0;
                src_roi.width = MIN(pos_x + width, intermediate_image.width) - MAX(0, pos_x);
                src_roi.height = MIN(pos_y + height, intermediate_image.height) - MAX(0, pos_y);

                out_roi.coi = 0;
                out_roi.xOffset = MAX(0, pos_x);
                out_roi.yOffset = MAX(0, pos_y);
                out_roi.width = src_roi.width;
                out_roi.height = src_roi.height;

                // If image has overlay channel, create a mask and apply it.
                if (*(MAPSUInt32*)imageIn.channelSeq == MAPS_CHANNELSEQ_BGRA || *(MAPSUInt32*)imageIn.channelSeq == MAPS_CHANNELSEQ_RGBA)
                {
                    cv::Mat mask = cv::Mat(cv::Size(imageIn.width, imageIn.height), CV_8U, 1);
                    unsigned int* p, * line; // original image
                    uchar* p_mask, * line_mask; // mask
                    line = reinterpret_cast<unsigned int*>(imageIn.imageData);
                    line_mask = reinterpret_cast<uchar*>(mask.data);
                    for (; line < reinterpret_cast<unsigned int*>((imageIn.imageData + imageIn.imageSize)); line += imageIn.widthStep / 4, line_mask += mask.rows)
                    {
                        p = line;
                        p_mask = line_mask;
                        for (; p_mask < line_mask + mask.cols; ++p, ++p_mask)
                        {
                            int alpha = (*p >> 24) & 0xFF;
                            if (alpha > 0)
                                *p_mask = 1;
                            else
                                *p_mask = 0;
                        }
                    }
                    cv::copyTo(convTools::noCopyIplImage2Mat(&src_image), convTools::noCopyIplImage2Mat(&intermediate_image), mask);
                }
                else
                {
                    cv::Mat matIn = convTools::noCopyIplImage2Mat(&src_image);
                    cv::Mat matOut =  convTools::noCopyIplImage2Mat(&intermediate_image);
                    cv::copyTo(matIn, matOut, cv::noArray());
                }

            }
            else
            {
                IplROI resized_img_roi;

                resized_img_roi.coi = 0;
                resized_img_roi.xOffset = -(MIN(0, pos_x));
                resized_img_roi.yOffset = -(MIN(0, pos_y));
                resized_img_roi.width = MIN(pos_x + width, intermediate_image.width) - MAX(0, pos_x);
                resized_img_roi.height = MIN(pos_y + height, intermediate_image.height) - MAX(0, pos_y);
                out_roi.coi = 0;
                out_roi.xOffset = MAX(0, pos_x);
                out_roi.yOffset = MAX(0, pos_y);
                out_roi.width = resized_img_roi.width;
                out_roi.height = resized_img_roi.height;

                src_image.roi->xOffset = resized_img_roi.xOffset * src_image.width / width;
                src_image.roi->yOffset = resized_img_roi.yOffset * src_image.height / height;
                src_image.roi->width = resized_img_roi.width * src_image.width / width;
                src_image.roi->height = resized_img_roi.height * src_image.height / height;

                cv::Mat src_mat = convTools::noCopyIplImage2Mat(&src_image);
                cv::Mat intermediate_mat = convTools::noCopyIplImage2Mat(&intermediate_image);
                cv::resize(src_mat, intermediate_mat, intermediate_mat.size());

            }
        }
    }
    if (m_outNeedResize)
    {
        cv::Mat out_mat = convTools::noCopyIplImage2Mat(&imageOut);
        intermediate_image.roi = nullptr;
        cv::Mat intermediate_mat = convTools::noCopyIplImage2Mat(&intermediate_image);
        cv::resize(intermediate_mat, out_mat, out_mat.size());
    }
}


void MAPSOpenCV_VideoMuxer::SortIOEltsByZOrder()
{
    MAPSList<SortingElt> sortlist;
    for (int i = 0; i < m_nbInputs; i++)
    {
        SortingElt elt;
        elt.index = i;
        elt.zorder = m_zOrder[i];
        sortlist.Append(elt);
    }

    sortlist.Bubblesort(compare_sortingelts);

    MAPSListIterator it;
    int i = 0;
    MAPSForallItems(it, sortlist)
    {
        m_ioeltsOrder[i] = sortlist[it].index;
        i++;
    }
}

void MAPSOpenCV_VideoMuxer::Set(MAPSProperty& p, MAPSInt64 value)
{
    MAPSComponent::Set(p, value);
    if (IsStarted())
    {
        const std::lock_guard<std::mutex> lock(m_zorderMutex);
        for (int i = 0; i < m_nbInputs; i++)
        {
            if (&p == &Property(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 1))
            {
                m_posX[i] = static_cast<int>(value);
                m_bgInitialized.clear();
                break;
            }
            else if (&p == &Property(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 2))
            {
                m_posY[i] = static_cast<int>(value);
                m_bgInitialized.clear();
                break;
            }
            else if (&p == &Property(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 3))
            {
                m_width[i] = static_cast<int>(value);
                m_bgInitialized.clear();
                break;
            }
            else if (&p == &Property(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 4))
            {
                m_height[i] = static_cast<int>(value);
                m_bgInitialized.clear();
                break;
            }
            else if (&p == &Property(i* Property_NumberOfProperties + m_firstPositionPropRuntime + 5))
            {
                m_zOrder[i] = static_cast<int>(value);
                SortIOEltsByZOrder();
                break;
            }
        }
    }
}

void MAPSOpenCV_VideoMuxer::Initialization(const MAPSTimestamp, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts)
{
    const IplImage& imageIn = inElts[0].Data();
    m_chanSeq = *reinterpret_cast<const MAPSUInt32*>(imageIn.channelSeq);
    m_dataOrder = imageIn.dataOrder;
    m_depth = imageIn.depth;
    m_align = imageIn.align;

    for (int i = 0; i < m_nbInputs; i++)
    {
        if (m_width[i] <= 0)
        {
            m_width[i] = inElts[i].Data().width;
        }
        if (m_height[i] <= 0)
        {
            m_height[i] = inElts[i].Data().height;
        }
        if (*reinterpret_cast<const MAPSUInt32*>(inElts[i].Data().channelSeq) != m_chanSeq)
        {
            MAPSStreamedString sx;
            sx << "Image format error on input " << Input(i).ShortName() << ". All input images must have the same color format.";
            Error(sx);
        }
    }
    ComputeSizesAndAllocOutBuffer();
}

void MAPSOpenCV_VideoMuxer::Initialization_Reactive(const MAPSTimestamp, const size_t inputThatAnswered, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts)
{
    const IplImage& imageIn = inElts[inputThatAnswered].Data();
    m_chanSeq = *reinterpret_cast<const MAPSUInt32*>(imageIn.channelSeq);
    m_dataOrder = imageIn.dataOrder;
    m_depth = imageIn.depth;
    m_align = imageIn.align;
}

void MAPSOpenCV_VideoMuxer::Initialization_Trigerred(const MAPSTimestamp, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts)
{
    const IplImage& imageIn = inElts[0].Data();
    m_chanSeq = *reinterpret_cast<const MAPSUInt32*>(imageIn.channelSeq);
    m_dataOrder = imageIn.dataOrder;
    m_depth = imageIn.depth;
    m_align = imageIn.align;
}

void MAPSOpenCV_VideoMuxer::ProcessData(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts)
{
    OutputResultImage(ts, inElts);
}

void MAPSOpenCV_VideoMuxer::ProcessData_Reactive(const MAPSTimestamp ts, const MAPS::ArrayView<MAPS::InputElt<IplImage>> inElts)
{
    if (!m_allSizesInitialized)
    {
        for (size_t i = 0; i < inElts.size(); ++i)
        {
            if (!inElts[i].IsValid())
                continue;

            const IplImage& imageIn = inElts[i].Data();

            //Check image format.
            if (*reinterpret_cast<const MAPSUInt32*>(imageIn.channelSeq) != m_chanSeq)
            {
                MAPSStreamedString sx;
                sx << "Image format error on input " << inElts[i].Input()->ShortName() << ". All input images must have the same color format.";
                Error(sx);
            }

           
            if (!m_sizeInitialized[i])
            {
                if (m_width[i] <= 0)
                {
                    m_width[i] = imageIn.width;
                }
                if (m_height[i] <= 0)
                {
                    m_height[i] = imageIn.height;
                }
                m_sizeInitialized[i] = true;
            }

            m_allSizesInitialized = true;
            for (int j = 0; j < m_nbInputs; j++)
            {
                if (!m_sizeInitialized[j])
                {
                    m_allSizesInitialized = false;
                    break;
                }
            }
        }

        return;
    }

    if (m_firstTimeAllInit)
    {
        m_firstTimeAllInit = false;
        ComputeSizesAndAllocOutBuffer();
    }

    //Do the job.
    OutputResultImage(ts, inElts);
}

