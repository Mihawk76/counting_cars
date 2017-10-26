//opencv
#include <opencv2/opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/video/tracking.hpp>
//C
#include <stdio.h>
//C++
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
using namespace cv;//a
using namespace std;
// Global variables
int totalMovingObject;
Mat fore;
Mat frame; //current frame
Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
//ptr<backgroundsubtractor> pmog2; //mog2 background subtractor
Ptr<BackgroundSubtractorMOG2> pMOG2; //mog2 background subtractor
std::vector<std::vector<cv::Point> > contours;
int insideCircle;
int totalObject = 1;
RNG rng(12345);
// Blob Tracking Algorithm 
// for logfile
ofstream data;
ofstream pusat;
ofstream area;
// Vehicle inputted by user
string vehicleType = "";
string vehicleScreen = "";
Point pusLin[2000];
//int areaObject[2000];
Point boundary1 = Point(149,9);
Point boundary2 = Point(314,235);
//float pusLinY[2000];
//Put Var in image
string text = "Funny text inside the box";
int fontFace = FONT_HERSHEY_PLAIN;
double fontScale = 2;
int thickness = 2;
int idObject [2000];
int countPusLin;
int countMovingObject = 0;
int keyboard; //input from keyboard
// struck for moving object
struct object {
	int id;
	int areaObject;
	Point positionObject;
};
Point click[2];
struct object movingObject[2000];
//using vector
vector<int>::size_type sz;
std::vector<int> areaObject;
int countAreaObject = 0;

void help();
void processVideo(char* videoFilename);
void processImages(char* firstFrameFilename);
void help()
{
    cout
    << "--------------------------------------------------------------------------" << endl
    << "This program shows how to use background subtraction methods provided by "  << endl
    << " OpenCV. You can process both videos (-vid) and images (-img)."             << endl
                                                                                    << endl
    << "Usage:"                                                                     << endl
    << "./bs {-vid <video filename>}"                         											<< endl
    << "for example: ./bs -vid video.avi"                                           << endl
    << "--------------------------------------------------------------------------" << endl
    << endl;
}
void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
     //int i;
     if  ( event == EVENT_LBUTTONDOWN )
     {   
          if(click[0].x == 0 && click[1].x == 0){ 
            click[0] = Point(x,y);
          }
          cout << "0 " << click[0] << " 1 " << click[1] << endl;
          if (click[0].x != 0){ 
            click[1] = Point(x,y);
          }   
          cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
     }   
     else if  ( event == EVENT_RBUTTONDOWN )
     {
          cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
     }   
     else if  ( event == EVENT_MBUTTONDOWN )
     {   
          cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
     }   
     else if ( event == EVENT_MOUSEMOVE )
     {   
          cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;

     }   
}
int main(int argc, char* argv[])
{
		sz = areaObject.capacity();
		areaObject.reserve(10);
    //emptying id
    //getline(cin, vehicleType);
     //emptying id
   
    /* preparing the log file
       clearing the file and then opening it */
    data.clear();
    data.open("data.log",ios::out);
    pusat.clear();
    pusat.open("pusat.log",ios::out);
    area.clear();
    area.open("area.log",ios::out);	
    //print help information
    help();
    //check for the input parameter correctness
    if(argc != 3) {
        cerr <<"Incorret input list" << endl;
        cerr <<"exiting..." << endl;
        return EXIT_FAILURE;
    }
    //create GUI windows
    namedWindow("Frame");
		setMouseCallback("Frame", CallBackFunc, NULL);
    //namedWindow("FG Mask MOG 2");
    //create Background Subtractor objects
    pMOG2 = createBackgroundSubtractorMOG2(); //MOG2 approach
    if(strcmp(argv[1], "-vid") == 0) {
        //input data coming from a video
        //processVideo(argv[2]);
        processVideo(argv[2]);
    }
    else {
        //error in reading input parameters
        cerr <<"Please, check the input parameters." << endl;
        cerr <<"Exiting..." << endl;
        return EXIT_FAILURE;
    }
    //destroy GUI windows
    destroyAllWindows();
    return EXIT_SUCCESS;
}
void processVideo(char* videoFilename) 
{
  HOGDescriptor hog;
	const std::string videoStreamAddress = "http://admin:@192.168.1.57:80/video.cgi?.mjpg";
  hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
	int totalPeople;
	int totalArea;
	int frame_count = 0;
	int loop = 0;
	Point boundarylow(5,199);
	Point boundaryhigh(626,471);
	Point boundarylow1(13,21);
	Point boundaryhigh1(546,188);
	//Point boundaryhigh(1140,400);
	//create the capture object
	VideoCapture capture(videoFilename);
	//VideoCapture capture(videoStreamAddress);
	if(!capture.isOpened())
	{
		//error in opening the video input
		cerr << "Unable to open video file: " << videoFilename << endl;
		exit(EXIT_FAILURE);
  }
  //read input data. ESC or 'q' for quitting
  while( (char)keyboard != 'q' && (char)keyboard != 27 )
	{
    //cout << "click " << click[0] << click[1]<< endl;
    /*if(click[0].x == 0 && loop > 2){
      waitKey(0);
    }
    loop++;
		if(click[0].x != 0 && click[0].y != 0 && click[1].x != 0 && click[1].y != 0){
			boundarylow = click[0];
			boundaryhigh = click[1];
		}*/
		totalPeople = 0;
		totalArea = 0;
		//read the current frame
		if(!capture.read(frame)) {
		cerr << "Unable to read next frame." << endl;
		cerr << "Exiting..." << endl;
		exit(EXIT_FAILURE);
	}
	//update the background model
	pMOG2->setDetectShadows(true);
	pMOG2->setShadowValue(0);
	pMOG2->setShadowThreshold(0.5);
	pMOG2->apply(frame, fgMaskMOG2);
	//get the frame number and write it on the current frame
	stringstream ss;
	rectangle(frame, cv::Point(10, 2), cv::Point(100,20), cv::Scalar(255,255,255), -1); // bg rectangle for frame number
  ss << capture.get(CAP_PROP_POS_FRAMES);
  string frameNumberString = ss.str();
  putText(frame, frameNumberString.c_str(), cv::Point(15, 15), FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
	rectangle(frame, cv::Point(493, 4), cv::Point(630,29), cv::Scalar(255,255,255), -1); // bg rectangle for frame number
  //putText(frame, "ALARM", cv::Point(500, 25), FONT_HERSHEY_SIMPLEX, 0.9 , cv::Scalar(0,0,255), 2);
  //putText(frame, "Test", cv::Point(500, 25), FONT_HERSHEY_SIMPLEX, 0.9 , cv::Scalar(0,0,255), 2);
  //show the current frame and the fg masks
  //imshow("Frame", frame);
  //imshow("FG Mask MOG 2", fgMaskMOG2);
	cv::erode(fgMaskMOG2,fore,cv::Mat());
	cv::dilate(fore,fore,cv::Mat());
	//cv::dilate(fgMaskMOG2,fore,cv::Mat());
	//cv::imshow("Fore", fore);
	cv::findContours( fore, // binary input image 
                               contours, // vector of vectors of points
                               CV_RETR_EXTERNAL, // retrieve only external contours
                               CV_CHAIN_APPROX_NONE); // detect all pixels of each contour
	cv::drawContours( frame, // draw contours here
                                  contours, // draw these contours
                                  -1, // draw all contours
                                  cv::Scalar(0,0,255), // set color
                                  2); // set thickness
	//cv::imshow("Frame",frame);
	//Approximate contours to polygons + get bounding rects and circles
  vector<vector<Point> > contours_poly( contours.size() );
	vector<Rect> boundRect( contours.size() );
	vector<Rect> Rect1( contours.size() );
	vector<Rect> Rect2( contours.size() );
	vector<Rect> filteredRect( contours.size() );
	vector<Point2f>center( contours.size() );
  vector<float>radius( contours.size() );
	vector<Rect> found, found_filtered;
	int areaperPeople[10000] = {0};
	
	//data << "# of contour size: " << contours.size() << endl ;
	for( int i = 0; i < contours.size(); i++ )
  {
		areaObject.push_back(i);
  	// Filtering Blob that is detected to delete false positif 
		if(contourArea(contours[i]) >= 1)
		{
			approxPolyDP( Mat(contours[i]), contours_poly[i], 4, true );
      boundRect[i] = boundingRect( Mat(contours_poly[i]) );
      minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] ); 
			Moments m1 = moments(Mat(contours_poly[i]), false);
			//Point x(x1,y1);
    	boundRect[i] = boundingRect( Mat(contours_poly[i]) );
			totalArea = totalArea + boundRect[i].area();
      float x1 = m1.m10 / m1.m00;
      float y1 = m1.m01 / m1.m00;
      //float x3 = ((boundRect[i].tl().x + boundRect[i].br().x)/2);
     //float y3 = ((boundRect[i].tl().y + boundRect[i].br().y)/2);
			for( int j = 0; j < contours.size(); j++ )
  		{
      	if ( j!=i ) { 
					Moments m2 = moments(Mat(contours_poly[j]), false);
      		float x2 = m2.m10 / m2.m00;
      		float y2 = m2.m01 / m2.m00;
					float distance = sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
					if ( distance < 50 && boundRect[i].area() > 400){
						Rect1[i] = boundingRect( Mat(contours_poly[i]) );
						Rect2[i] = boundingRect( Mat(contours_poly[j]) );
						//boundRect[i] = Rect1[i] | Rect2[i];
						//filteredRect[i] = Rect1[i] | Rect2[i];
						}
				}	
			}
			int areaPeople = 1000;
			if( boundRect[i].area() > 1 && y1 > boundarylow.y && y1 < boundaryhigh.y && x1 > boundarylow.x 
					&& x1 < boundaryhigh.x){
				filteredRect[i] = boundRect[i];
				if (/*boundRect[i].area() <= areaPeople && */boundRect[i].area() >= areaPeople/2){
					totalPeople++;
					areaperPeople[i] = 1;
					//cout << "People found 1" << endl;
					rectangle(frame, cv::Point(493, 4), cv::Point(630,29), cv::Scalar(255,255,255), -1); // bg rectangle for frame number
  				putText(frame, "CRITICAL", cv::Point(500, 25), FONT_HERSHEY_SIMPLEX, 0.9 , cv::Scalar(0,0,255), 2);
				}
				int loop = 2;
				//for(loop=2;loop<=2;loop++){
				/*	if (boundRect[i].area() <= areaPeople*loop && boundRect[i].area() > (areaPeople-(loop-1))){
						totalPeople = totalPeople + loop;
						areaperPeople[i] = loop;
						//Mat classifier((boundRect[i].size()*8),CV_8UC3);
						Mat classifier(Size(2000,1000),CV_8UC3);
						//Mat classifier(Size(320,240),CV_8UC3);
						//cout << "People found " << loop << endl;
					}*/
				/*loop = 3;
					if (boundRect[i].area() <= areaPeople*loop && boundRect[i].area() > (areaPeople-(loop-1))){
						totalPeople = totalPeople + loop;
						areaperPeople[i] = loop;
						//cout << "People found " << loop << endl;
					}*/
				//}
				/*if (boundRect[i].area() < areaPeople*3 && boundRect[i].area() > areaPeople*2){
					totalPeople = totalPeople + 3;
					areaperPeople[i] = 3;
						Mat classifier(Size(2000,1000),CV_8UC3);
						//Mat classifier(Size(320,240),CV_8UC3);
						//cout << "People found 3" << endl;
				}*/
				/*if (boundRect[i].area() < areaPeople*4 && boundRect[i].area() > areaPeople*3){
					totalPeople = totalPeople + 4;
					areaperPeople[i] = 4;
						Mat classifier(Size(2000,1000),CV_8UC3);
						//Mat classifier(Size(320,240),CV_8UC3);
						//cout << "People found 4" << endl;
				}*/
				
				//cout << filteredRect[i].area() << endl;
			}
			areaPeople = 4000;
			if( boundRect[i].area() > 1 && y1 > boundarylow1.y && y1 < boundaryhigh1.y && x1 > boundarylow1.x 
					&& x1 < boundaryhigh1.x){
				filteredRect[i] = boundRect[i];
				if (boundRect[i].area() <= areaPeople && boundRect[i].area() >= areaPeople/4){
					totalPeople++;
					areaperPeople[i] = 1;
					//cout << "People found 1" << endl;
					rectangle(frame, cv::Point(493, 4), cv::Point(630,29), cv::Scalar(255,255,255), -1); // bg rectangle for frame number
  				putText(frame, "WARNING", cv::Point(500, 25), FONT_HERSHEY_SIMPLEX, 0.9 , cv::Scalar(0,0,255), 2);
				}
				int loop = 2;
				//for(loop=2;loop<=2;loop++){
			/*		if (boundRect[i].area() <= areaPeople*loop && boundRect[i].area() > (areaPeople-(loop-1))){
						totalPeople = totalPeople + loop;
						areaperPeople[i] = loop;
						//Mat classifier((boundRect[i].size()*8),CV_8UC3);
						Mat classifier(Size(2000,1000),CV_8UC3);
						//Mat classifier(Size(320,240),CV_8UC3);
						//cout << "People found " << loop << endl;
					}

					totalPeople = totalPeople + 3;
					areaperPeople[i] = 3;
						Mat classifier(Size(2000,1000),CV_8UC3);
						//Mat classifier(Size(320,240),CV_8UC3);
						//cout << "People found 3" << endl;
				}
				if (boundRect[i].area() < areaPeople*4 && boundRect[i].area() > areaPeople*3){
					totalPeople = totalPeople + 4;
					areaperPeople[i] = 4;
						Mat classifier(Size(2000,1000),CV_8UC3);
						//Mat classifier(Size(320,240),CV_8UC3);
						//cout << "People found 4" << endl;
				}*/
				
				//cout << filteredRect[i].area() << endl;
			}
    	minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );		
  	}
  }
	cout << "Total People " << totalPeople << " area " << totalArea << endl;
	int garisx = 0;
	int lengthx = 1500;
	int garisy = 100;
	int a;
	int kolomx = 100;
	int panjangkolom = 1000;
	int kolomy = 100;
	/*for(a=0;a<20;a++){
		line( frame, Point(kolomx ,kolomy), Point(kolomx,kolomy+panjangkolom), Scalar( 0, 0, 0 ), 2, 8 );
	nux desktop app	kolomx = kolomx + 80;
		line( frame, Point(kolomx ,kolomy), Point(kolomx,kolomy+panjangkolom), Scalar( 0, 0, 0 ), 2, 8 );
	}
	line( frame, Point(boundarylow.x ,boundarylow.y), Point(boundarylow.x,1100), Scalar( 0, 0, 255 ), 2, 8 );
	line( frame, Point(boundaryhigh.x ,100), Point(boundaryhigh.x,1100), Scalar( 0, 0, 255 ), 2, 8 );
	for(a=0;a<10;a++){
		line( frame, Point(garisx ,garisy), Point(garisx+lengthx,garisy), Scalar( 0, 0, 0 ), 2, 8 );
		garisy=garisy+150;
	}
	line( frame, Point(0 ,boundarylow.y), Point(1500,boundarylow.y), Scalar( 0, 0, 255 ), 2, 8 );
	line( frame, Point(0 ,boundaryhigh.y), Point(1500,boundaryhigh.y), Scalar( 0, 0, 255 ), 2, 8 );*/
    totalMovingObject = 0;
	stringstream oss;
  	Mat drawing = Mat::zeros( frame.size(), CV_8UC3 );
  	for( int i = 0; i< contours.size(); i++ )
    	{
					Scalar color_1 = Scalar(255,255,0);
					Scalar color_2 = Scalar(255,255,255);
					Scalar color_3 = Scalar(154,250,0);
					Scalar color_4 = Scalar(0,0,255);
       		Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
       		drawContours( frame, contours_poly, i, color_1, 1, 8, vector<Vec4i>(), 0, Point() );
					if(areaperPeople[i]<2){
					std::string label = "N:" + to_string(areaperPeople[i]) + " A:" + to_string(filteredRect[i].area());
					//putText(frame, label, filteredRect[i].tl(), FONT_HERSHEY_PLAIN, 1.0, color_4, 2.0);
       		rectangle( frame, filteredRect[i].tl(), filteredRect[i].br(), color_4, 2, 8, 0 );
					}
					if(areaperPeople[i]==2){
						std::string label = "N:" + to_string(areaperPeople[i]) + " A:" + to_string(filteredRect[i].area());
						//putText(frame, label, filteredRect[i].tl(), FONT_HERSHEY_PLAIN, 1.0, color_1, 2.0);
       		rectangle( frame, filteredRect[i].tl(), filteredRect[i].br(), color_1, 2, 8, 0 );
					}
					if(areaperPeople[i]==3){
						std::string label = "N:" + to_string(areaperPeople[i]) + " A:" + to_string(filteredRect[i].area());
						//putText(frame, label, filteredRect[i].tl(), FONT_HERSHEY_PLAIN, 1.0, color_2, 2.0);
       		rectangle( frame, filteredRect[i].tl(), filteredRect[i].br(), color_2, 2, 8, 0 );
					}
					if(areaperPeople[i]==4){
						std::string label = "N:" + to_string(areaperPeople[i]) + " A:" + to_string(filteredRect[i].area());
						//putText(frame, label, filteredRect[i].tl(), FONT_HERSHEY_PLAIN, 1.0, color_3, 2.0);
       		rectangle( frame, filteredRect[i].tl(), filteredRect[i].br(), color_2, 2, 8, 0 );
					}
       		//circle( frame, center[i], (int)radius[i], color_1, 2, 8, 0 );
		
       		//drawContours( drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
       		//rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );
       		//circle( drawing, center[i], (int)radius[i], color, 2, 8, 0 );
					
     	}
	if(totalPeople > 0){
		char filename[128];
    sprintf(filename, "frame_%06d.jpg", frame_count);
    cv::imwrite(filename, frame);
	}
	//imshow( "Contours", drawing );
	frame_count++;
	imshow( "Frame", frame );
	Mat im;
	//transisition to blob
    	IplImage image =  frame;
	
        //get the input from the keyboard
        keyboard = waitKey( 30 );
    }
    //delete capture object
    capture.release();
}
void processImages(char* fistFrameFilename) {
    //read the first file of the sequence
    frame = imread(fistFrameFilename);
    if(frame.empty()){
        //error in opening the first image
        cerr << "Unable to open first image frame: " << fistFrameFilename << endl;
        exit(EXIT_FAILURE);
    }
    //current image filename
    string fn(fistFrameFilename);
    //read input data. ESC or 'q' for quitting
    while( (char)keyboard != 'q' && (char)keyboard != 27 ){
        //update the background model
        pMOG2->apply(frame, fgMaskMOG2);
        //get the frame number and write it on the current frame
        size_t index = fn.find_last_of("/");
        if(index == string::npos) {
            index = fn.find_last_of("\\");
        }
        size_t index2 = fn.find_last_of(".");
        string prefix = fn.substr(0,index+1);
        string suffix = fn.substr(index2);
        string frameNumberString = fn.substr(index+1, index2-index-1);
        istringstream iss(frameNumberString);
        int frameNumber = 0;
        iss >> frameNumber;
        rectangle(frame, cv::Point(10, 2), cv::Point(100,20),
                  cv::Scalar(255,255,255), -1);
        putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
                FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
        //show the current frame and the fg masks
        //imshow("Frame", frame);
        //imshow("FG Mask MOG 2", fgMaskMOG2);
	      erode(fore,fgMaskMOG2,cv::Mat());
	      dilate(fore,fore,cv::Mat());
	      //imshow("Fore", fore);
        //get the input from the keyboard
        keyboard = waitKey( 30 );
        //search for the next image in the sequence
        ostringstream oss;
        oss << (frameNumber + 1);
        string nextFrameNumberString = oss.str();
        string nextFrameFilename = prefix + nextFrameNumberString + suffix;
        //read the next frame
        frame = imread(nextFrameFilename);
        if(frame.empty()){
            //error in opening the next image in the sequence
            cerr << "Unable to open image frame: " << nextFrameFilename << endl;
            exit(EXIT_FAILURE);
        }
        //update the path of the current frame
        fn.assign(nextFrameFilename);
    }
}
