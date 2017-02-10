// hand tracking using meanshift

#pragma once
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

class HandTracker
{
public:
	HandTracker(void);
	~HandTracker(void);
    bool init(Mat frame, Rect &trackBox);
	bool processFrame(Mat frame, Rect &trackBox);
	bool detectFist(Mat frame, Rect trackBox);

private:
	bool isHand(const Mat frame);
	void detectPalm( Mat img, Rect &box);
	void frameDiff(const Mat image, Mat &diff);
	void calSkinPro(Mat frame);
    void getSkinModel(const Mat img, Rect rect);
    
private:
	Mat backProject;
	MatND hist;
    CascadeClassifier palmCascade;
	CascadeClassifier fistCascade;
	Mat preGray;
    int successiveDetect;
};