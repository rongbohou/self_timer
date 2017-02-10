
#include "handTracker.h"

HandTracker::HandTracker()
{
    successiveDetect = 0;

    const char *palmCascadeName = "palm.xml";
	const char *fistCascadeName = "fist.xml";
    if (!palmCascade.load(palmCascadeName) || !fistCascade.load(fistCascadeName))
    {
        cout<<"Can not load cascade!"<<endl;
    }
}

HandTracker::~HandTracker()
{

}

// init function: detect hand region and init meanshift
bool HandTracker::init(Mat frame, Rect &trackBox)
{
    trackBox = Rect(0, 0, 0, 0);

	// detect hand
    detectPalm(frame, trackBox);

	// The detected box should large enough and not near the boundary of image
    if (trackBox.area() > 900 && 0.3 * frame.cols < trackBox.x + 0.5 * trackBox.width 
							&& trackBox.x + 0.5 * trackBox.width < 0.7 * frame.cols 
							&& 0.3 * frame.rows < trackBox.y + 0.5 * trackBox.height 
							&& trackBox.y + 0.5 * trackBox.height < 0.7 * frame.rows)
    {
		// Check skin area of the detected box to make sure it is a hand
        if (isHand(frame(trackBox)))
        {
			// To avoid detecting error, need to successive detect twice successfully
            successiveDetect++;
            if (successiveDetect > 2)
            {
                // Calculate skin probability model for meanshift
                getSkinModel(frame, trackBox);
                successiveDetect = 0;
                return true;
            }
        }
    }
    return false;
}

// detect hands and return the biggest hand
void HandTracker::detectPalm( Mat img, Rect &box)
{
    double scale = 1.3;
    Mat small_img, gray;
    vector<Rect> boxs;
    gray.create(img.rows, img.cols, CV_8UC1);
    small_img.create( cvRound (gray.rows/scale), cvRound(gray.cols/scale), CV_8UC1 );

    cvtColor( img, gray, CV_BGR2GRAY );
    resize( gray, small_img, small_img.size(), 0, 0, INTER_LINEAR );
    equalizeHist( small_img, small_img );//直方图归一化

    palmCascade.detectMultiScale( small_img, boxs, 1.1, 2, CV_HAAR_SCALE_IMAGE, Size(30, 30) );

    //Get the bigest face
    Rect maxBox(0, 0, 0, 0);
    for (vector<Rect>::const_iterator r = boxs.begin(); r != boxs.end(); r++)
    {
        if (r->area() > maxBox.area())
            maxBox = *r;
    }
    if (boxs.size() > 0)
    {
        box.x = cvRound(maxBox.x * scale); 
        box.y = cvRound(maxBox.y * scale);
        box.width = cvRound(maxBox.width * scale);
        box.height = cvRound(maxBox.height * scale);
    }
}

// check skin area of our tracking box to make sure it is a hand
bool HandTracker::isHand(const Mat frame)
{
    Mat YCbCr;
    vector<Mat> planes;
    int count = 0;

    cvtColor(frame, YCbCr, CV_RGB2YCrCb);
    split(YCbCr, planes);

    MatIterator_<uchar> it_Cb = planes[1].begin<uchar>(),
                        it_Cb_end = planes[1].end<uchar>();
    MatIterator_<uchar> it_Cr = planes[2].begin<uchar>();

	// skin satisfy: 138 <= Cr <= 170 and 100 <= Cb <= 127 (empirical value)
    for( ; it_Cb != it_Cb_end; ++it_Cr, ++it_Cb)
    {
        if (138 <= *it_Cr &&  *it_Cr <= 170 && 100 <= *it_Cb &&  *it_Cb <= 127)
            count++;
    }

    // It is a hand when contains large enough skin area
    return (count > 0.4 * frame.cols * frame.rows);
}

// Calculate skin probability model (histogram) for meanshift
void HandTracker::getSkinModel(const Mat img, Rect rect)
{
	int hue_Bins = 50;	
	float hue_Ranges[] = { 0, 180 }; 
	const float *ranges= hue_Ranges;

	Mat HSV, hue, mask;
	cvtColor(img, HSV, CV_RGB2HSV);
	inRange(HSV, Scalar(0, 30, 10), Scalar(180, 256, 256), mask);
	vector<Mat> planes;
	split(HSV, planes); 
	hue = planes[0];

	Mat roi(hue, rect), maskroi(mask, rect);
    calcHist(&roi, 1, 0, maskroi, hist, 1, &hue_Bins, &ranges);
    normalize(hist, hist, 0, 255, CV_MINMAX);//normalize histogram
}

// Calculate skin probability image (back project map) for meanshift
void HandTracker::calSkinPro(Mat frame)
{
	Mat mask, hue, HSV;
	cvtColor(frame, HSV, CV_RGB2HSV);
	inRange(HSV, Scalar(0, 30, 10), Scalar(180, 256, 256), mask);
	vector<Mat> planes;
	split(HSV, planes); 
	hue = planes[0];

	// hue varies from 0 to 179, see cvtColor
	float hue_Ranges[] = { 0, 180 };  
	const float *ranges= hue_Ranges;

	calcBackProject(&hue, 1, 0, hist, backProject, &ranges, 1.0, true);//计算hue的反向投影
	backProject &= mask;
}

//  Detect motion using frame differece
void HandTracker::frameDiff(const Mat image, Mat &diff)
{
	int thresValue = 20;
	Mat curGray;
	cvtColor(image, curGray, CV_RGB2GRAY);
	if (preGray.size != curGray.size)
		curGray.copyTo(preGray);

	absdiff(preGray, curGray, diff);
	threshold(diff, diff, thresValue, 255, CV_THRESH_BINARY);
	erode(diff, diff, Mat(3, 3, CV_8UC1), Point(-1,-1));
	dilate(diff, diff, Mat(3, 3, CV_8UC1), Point(-1,-1));
	curGray.copyTo(preGray);
}

// Tracking hand using meanshift
bool HandTracker::processFrame(Mat frame, Rect &trackBox)
{
	float rate = 0.9;
	Mat diff;

	// tracking hand
	calSkinPro(frame);        // skin information
	frameDiff(frame, diff);   // motion information

	// fusing skin and motion information using a weighted rate
	Mat handProMap = backProject * rate + (1 - rate) * diff;
	meanShift(handProMap, trackBox, TermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));
	
	// ensure the tracking result is a hand
	Mat skin = backProject(trackBox) > 100;//
	return countNonZero(skin) > 0.4 * trackBox.area();//
}

// Detect fist for the command: click the mouse
bool HandTracker::detectFist(Mat frame, Rect palmBox)
{
	Rect detectFistBox;
	detectFistBox.x = (palmBox.x - 40) > 0 ? (palmBox.x - 40) : 0;
	detectFistBox.y = (palmBox.y - 20) > 0 ? (palmBox.y - 20) : 0;
    detectFistBox.width = palmBox.width + 80;
	detectFistBox.height = palmBox.height + 40;
	detectFistBox &= Rect(0, 0, frame.cols, frame.rows);//pay attention 
    
	Mat gray;
	cvtColor(frame, gray, CV_BGR2GRAY );
	Mat tmp = gray(detectFistBox);
	vector<Rect> fists;
    fistCascade.detectMultiScale(tmp, fists, 1.1, 2, CV_HAAR_SCALE_IMAGE, Size(30, 30) );

    return fists.size();
}
