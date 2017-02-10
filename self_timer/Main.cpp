
#include "opencv2/opencv.hpp"
#include "handTracker.h"
#include<windows.h>
#include "time.h"


using namespace cv;
using namespace std;

int main(int argc, char* argv[])
{
	Mat frame;	
	Rect trackBox;
	Point prePoint, curPoint;
	VideoCapture capture;
	bool gotTrackBox = false;
	bool gotBB=false;
	int interCount = 0;
	int continue_fist = 0;
	int left=0;
	int right=0;
	char saveName[20];
	struct tm *ptr;
	time_t it,time_start,time_end;
	
	capture.open(0);
	cvNamedWindow("photo",CV_WINDOW_AUTOSIZE);
	capture.set(CV_CAP_PROP_FRAME_WIDTH,640);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT,480);
	if (!capture.isOpened())
	{
		cout<<"No camera!\n"<<endl;
		return -1;
	}

	HandTracker hand_tracker;

START:
	while (1)
	{
		/************ Get hand which need to be tracked ************/
		while (!gotTrackBox)
		{
			capture >> frame;	
			if(frame.empty())
				return -1;

            if (hand_tracker.init(frame, trackBox))
			{
                gotTrackBox = true;
				prePoint = Point(trackBox.x + 0.5 * trackBox.width, trackBox.y + 0.5 * trackBox.height);
			}

			imshow("photo", frame);
				
			if (waitKey(2) == 27)
				return -1;
		}

		capture >> frame;

		/****************** Tracking hand **************************/
		if (!hand_tracker.processFrame(frame, trackBox))
			gotTrackBox = false;

		/****************** Control the mouse *********************/
		curPoint = Point(trackBox.x + 0.5 * trackBox.width, trackBox.y + 0.5 * trackBox.height);
		int dx = curPoint.x - prePoint.x;
		//int dy = curPoint.y - prePoint.y;
		
		// When you made a fist, means you click left button of mouse
		// To avoid successive active within short time, when you had actived a single 
		// we will ingnore the next 10 frames
		interCount++;
		if(interCount > 10 && hand_tracker.detectFist(frame, trackBox))
		{
		 // To avoid detecting error, need to successive detect three times successfully
		 continue_fist++;
		 if (continue_fist > 3)
		  {
			interCount = 0;
			cout << "take photo three seconds later " << endl;
			time_start=time(NULL);
			/************ wait for three seconds ********************/
			while(1)
			{
			 capture >> frame;
			 time_end=time(NULL);
				if(difftime(time_end,time_start)==1)
				 {
					 putText(frame,"1", Point(0.5*frame.cols, 0.25*frame.rows), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 3);
				 }
				 if(difftime(time_end,time_start)==2)
				  {
					  putText(frame,"2",Point(0.5*frame.cols, 0.25*frame.rows), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 3);
				  }
				 if(difftime(time_end,time_start)==3)
				   {
					 putText(frame,"3",Point(0.5*frame.cols, 0.25*frame.rows), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 3);
					 imshow("photo",frame);
					 cvWaitKey(50);
					 break;
				   }
				 if ( cvWaitKey(3) == 27 )
			     break;
				 imshow("photo", frame);	
				 }
			capture >> frame;
			it=time(NULL);
			ptr=localtime(&it);
			Beep(523,500);
			strftime(saveName,sizeof(saveName),"%H.png",ptr);//save the photo,and the name of it is the local time
	        imwrite(saveName,frame);
			imshow("photo", frame);	
			cout<<saveName<<endl;
        /****************** save or delete the photo *********************/
		while(1)
		{
		    while (!gotBB)
	     {
			capture >> frame;	
			if(frame.empty())
				return -1;

            if (hand_tracker.init(frame, trackBox))
			{
                gotBB = true;
				prePoint = Point(trackBox.x + 0.5 * trackBox.width, trackBox.y + 0.5 * trackBox.height);
			}

			imshow("photo", frame);
				
			if (waitKey(2) == 27)
				return -1;
		  }

		    capture >> frame;

		    /****************** Tracking hand **************************/
		    if (!hand_tracker.processFrame(frame, trackBox))
			gotBB = false;

		    /*************** determine which direction the hand move to,left or right *********************/
		   curPoint = Point(trackBox.x + 0.5 * trackBox.width, trackBox.y + 0.5 * trackBox.height);
		   int dx = curPoint.x - prePoint.x;
		   prePoint=curPoint;
		   /******* move to the left *************/
		   if(dx<-10)
		   {
		     left++;
			 right=0;
			 dx=0;
			 cout<<left<<endl;
				if(left>5)
				{   
					Beep(523,500);
					left=0;
					
					char filename[20];
					sprintf(filename,"del %s",saveName);
					system(filename);
					putText(frame,"delete", Point(20, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 3);
					rectangle(frame, trackBox, Scalar(0, 0, 255), 3);
		            imshow("photo", frame);	
					break;

				}
		   }
		    /******* move to the right *************/
		   if(dx>10)
		   {
			   right++;
			   left=0;
			   dx=0;
			   cout<<right<<endl;
			   if(right>5)
				{
					right=0;
					Beep(523,500);
					putText(frame,"save", Point(20, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 3);
					rectangle(frame, trackBox, Scalar(0, 0, 255), 3);
		            imshow("photo", frame);	
					break;
				}
		   
		   }
		 rectangle(frame, trackBox, Scalar(0, 0, 255), 3);
		 imshow("photo", frame);	

		if ( cvWaitKey(3) == 27 )
			break;
		}
		 continue_fist = 0;

		 goto START;
	   }
		}
		else
			continue_fist = 0;
		


		/******************* Show image ****************************/
		rectangle(frame, trackBox, Scalar(0, 0, 255), 3);
		imshow("photo", frame);	

		if ( cvWaitKey(3) == 27 )
			break;
	}

	return 0;
}