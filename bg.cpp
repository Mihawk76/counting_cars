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
using namespace cv;
using namespace std;
// Global variables
int totalMovingObject;
Mat fore;
Mat frame; //current frame
Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
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
    //namedWindow("FG Mask MOG 2");
    //create Background Subtractor objects
    pMOG2 = createBackgroundSubtractorMOG2(); //MOG2 approach
    if(strcmp(argv[1], "-vid") == 0) {
        //input data coming from a video
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
Rect combineRect(cv::Rect Rect1, cv::Rect Rect2)
{
	Point tlCombine, brCombine;
	tlCombine = Rect1.tl();
	if (Rect1.tl().x<Rect2.tl().x){
		tlCombine.x = Rect1.tl().x;
	}
	else{
		tlCombine.x = Rect2.tl().x;
	}
	if (Rect1.tl().y>Rect2.tl().y){
		tlCombine.y = Rect1.tl().y;
	}
	else{
		tlCombine.y = Rect2.tl().y;
	}
	if (Rect1.br().x>Rect2.br().x){
		brCombine.x = Rect1.br().x;
	}
	else{
		brCombine.x = Rect2.br().x;
	}
	if (Rect1.tl().y<Rect2.tl().y){
		tlCombine.y = Rect1.tl().y;
	}
	else{
		tlCombine.y = Rect2.tl().y;
	}
	Rect Combine(tlCombine, brCombine);	
	return Combine;	
}
void processVideo(char* videoFilename) 
{
	int totalPeople;
	//create the capture object
	VideoCapture capture(videoFilename);
	if(!capture.isOpened())
	{
		//error in opening the video input
		cerr << "Unable to open video file: " << videoFilename << endl;
		exit(EXIT_FAILURE);
  }
  //read input data. ESC or 'q' for quitting
  while( (char)keyboard != 'q' && (char)keyboard != 27 )
	{
		totalPeople = 0;
		//read the current frame
		if(!capture.read(frame)) {
		cerr << "Unable to read next frame." << endl;
		cerr << "Exiting..." << endl;
		exit(EXIT_FAILURE);
	}
	//update the background model
	pMOG2->apply(frame, fgMaskMOG2);
	//get the frame number and write it on the current frame
	stringstream ss;
	rectangle(frame, cv::Point(10, 2), cv::Point(100,20), cv::Scalar(255,255,255), -1); // bg rectangle for frame number
  ss << capture.get(CAP_PROP_POS_FRAMES);
  string frameNumberString = ss.str();
  putText(frame, frameNumberString.c_str(), cv::Point(15, 15), FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
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
	//vector<Rect> Rect3( contours.size() );
	vector<Rect> filteredRect( contours.size() );
	vector<Point2f>center( contours.size() );
  vector<float>radius( contours.size() );
	
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
						boundRect[i] = Rect1[i] | Rect2[i];
						//filteredRect[i] = Rect1[i] | Rect2[i];
						}
				}	
			}
			int areaPeople = 12000;
			if( boundRect[i].area() > 1000){
				filteredRect[i] = boundRect[i];
				if (boundRect[i].area() < areaPeople && boundRect[i].area() > areaPeople/2){
					totalPeople++;
					cout << "People found 1" << endl;
				}
				if (boundRect[i].area() < areaPeople*2 && boundRect[i].area() > areaPeople){
					totalPeople = totalPeople + 2;
					cout << "People found 2" << endl;
				}
				if (boundRect[i].area() < areaPeople*3 && boundRect[i].area() > areaPeople*2){
					totalPeople = totalPeople + 3;
					cout << "People found 3" << endl;
				}
			}
			//cout << "area " << (filteredRect[i].size().width * filteredRect[i].size().height)  << endl;
			//if( boundRect[i].area() > 50){
			//		filteredRect[i] = boundRect[i];
			//}
    	minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );		
			/*float x = center[i].x;
			float y = center[i].y;
			if (countPusLin <= 2000)
			{
				if ( center[i].x > 10 || center[i].y > 10)
				{
					pusLin[countPusLin] = center[i];
					countPusLin++;
					// Limiting the detection into one box and moving one array to another one.
					// Putting all the moving object to one array 
					if ( center[i].x > boundary1.x && center[i].x < boundary2.x && center[i].y > boundary1.y && center[i].y < boundary2.y)
					{
						movingObject[countMovingObject].positionObject = center[i];
						movingObject[countMovingObject].areaObject = contourArea(contours[i]);
						countMovingObject++;
					}
				}
			}*/
  	}
  }
	cout << "Total People " << totalPeople << endl;
	int garisx = 0;
	int lengthx = 1500;
	int garisy = 100;
	int a;
	int kolomx = 100;
	int panjangkolom = 1000;
	int kolomy = 100;
	//line( frame, Point(garisx+lengthx ,garisy), Point(garisx+lengthx,garisy-150), Scalar( 0, 0, 0 ), 2, 8 );
	//lengthx = lengthx - 80;
	//line( frame, Point(garisx+lengthx ,garisy), Point(garisx+lengthx,garisy-150), Scalar( 0, 0, 0 ), 2, 8 );
	for(a=0;a<20;a++){
		line( frame, Point(kolomx ,kolomy), Point(kolomx,kolomy+panjangkolom), Scalar( 0, 0, 0 ), 2, 8 );
		kolomx = kolomx + 80;
		line( frame, Point(kolomx ,kolomy), Point(kolomx,kolomy+panjangkolom), Scalar( 0, 0, 0 ), 2, 8 );
	}
	for(a=0;a<10;a++){
		line( frame, Point(garisx ,garisy), Point(garisx+lengthx,garisy), Scalar( 0, 0, 0 ), 2, 8 );
		garisy=garisy+150;
	}
	//line( frame, Point(garisx ,garisy), Point(garisx+lengthx,garisy), Scalar( 0, 0, 0 ), 2, 8 );
	//int baseline=0;
	//Size textSize = getTextSize(text, fontFace,fontScale, thickness, &baseline);
	//baseline += thickness;
	//int carIn = 0;
	//int count = 0;
	//Boundary Box for motion tracking
	//line( frame, boundary1 , Point(boundary1.x,boundary2.y), Scalar( 0, 0, 0 ), 2, 8 );
	//line( frame, Point(boundary1.x, boundary2.y) , boundary2, Scalar( 0, 0, 0 ), 2, 8 );
	//line( frame, boundary2 , Point(boundary2.x,boundary1.y), Scalar( 0, 0, 0 ), 2, 8 );
	//line( frame, Point(boundary2.x, boundary1.y) , boundary1, Scalar( 0, 0, 0 ), 2, 8 );
	//Boundary Box for counting cars
	//int yCarMin = 250;
	//int yCarMax = 300;
	//int yBefore = 200;
	//line( frame, Point(1, yCarMin) , Point(boundary2.x,yCarMin), Scalar( 0, 0, 255 ), 2, 8 );
	//line( frame, Point(1, yCarMax) , Point(boundary2.x,yCarMax), Scalar( 0, 0, 255 ), 2, 8 );
	//line( frame, Point(1, yBefore) , Point(boundary2.x,yBefore), Scalar( 0, 255, 255 ), 2, 8 );
	
	//int countObject = 0;
	//totalMovingObject = 0;
	/*while ( totalMovingObject < (countMovingObject-1) )
	{
		int bound = 30; // the variable that we consider the maximal jump of distance an obj travel
		 //Filtering which koordinat is from the same Object, the idea is the same object move in 
		//limited distance in a short time so we assume that if it is under bound than it is from the same object	
		int loop = 0;
		while ( loop <= 10 )
		{
			if ( ((movingObject[totalMovingObject-loop].positionObject).x) 
			< (((movingObject[totalMovingObject].positionObject).x)+bound) 
			&& ((movingObject[totalMovingObject-loop].positionObject).x) 
			> (((movingObject[totalMovingObject].positionObject).x)-bound) 
			&& ((movingObject[totalMovingObject-loop].positionObject).y) 
			< (((movingObject[totalMovingObject].positionObject).y)+bound) 
			&& ((movingObject[totalMovingObject-loop].positionObject).y) 
			> (((movingObject[totalMovingObject].positionObject).y)-bound) ) 
			{
				line( frame, (movingObject[totalMovingObject].positionObject) 
				, (movingObject[totalMovingObject-loop].positionObject), 
				Scalar( 0, 0, 0 ), 2, 8 );
				if // this collect the coordinat that passes the first line
				( ((movingObject[totalMovingObject].positionObject).y) < yCarMax 
		     && ((movingObject[totalMovingObject].positionObject).y) > yCarMin 
		     && ((movingObject[totalMovingObject].positionObject).x) > 1 
		     && ((movingObject[totalMovingObject].positionObject).x) < boundary2.x 
				)
				{
					if //Condition that is allign with the desired movement
					( 
					  ((movingObject[totalMovingObject-loop].positionObject).y) < yCarMin 
					  && ((movingObject[totalMovingObject-loop].positionObject).y) > yBefore 
					  && ((movingObject[totalMovingObject-loop].positionObject).x) > 1 
					  && ((movingObject[totalMovingObject-loop].positionObject).x) < boundary2.x )
					{
						//Adding variable is it is the same with the correct movement
						carIn++;
            if ( movingObject[totalMovingObject].areaObject < 5000 )
            {
                vehicleScreen = "motorcyle";
            }
            else
            {
                vehicleScreen = "car";
            }
						if ( std::find(areaObject.begin(), areaObject.end(), movingObject[totalMovingObject].areaObject) != areaObject.end() )
							{}
						else
							{
								countAreaObject++;
								areaObject.push_back(countAreaObject);
								//int coba = movingObject[totalMovingObject].areaObject;
								areaObject[countAreaObject] = movingObject[totalMovingObject].areaObject;
                cout << " Insert the vehicle name " << endl;
                //getline(cin, vehicleType);
                data << areaObject[countAreaObject] << " is " << vehicleType << endl; 
							}
					}
				}	
			}
		loop++;
		}
		totalMovingObject++;
	}
	*/
    totalMovingObject = 0;
	/*for (int start = 0; start <= countAreaObject; start++)
		{
			area << start << " area is " << areaObject[start] << " jumlah objek adalah " << countAreaObject << endl;
      cout << start << " area is " << areaObject[start] << " jumlah objek adalah " << countAreaObject 
			<< " jenis " << vehicleScreen << endl;
		}*/
	// then put the text itself
	stringstream oss;
	//oss << " amount " << carIn << " type " << vehicleScreen;
	//pusat << "Car that pass " << carIn << endl;
	//text = oss.str();
	//putText(frame, text, Point(10, 350) , fontFace, fontScale, Scalar( 0, 0, 0 ), thickness, 8);
	// Draw polygonal contour + bonding rects + circles
  	Mat drawing = Mat::zeros( frame.size(), CV_8UC3 );
  	for( int i = 0; i< contours.size(); i++ )
    	{
					Scalar color_1 = Scalar(255,0,255);
					Scalar color_2 = Scalar(0,0,0);
       		Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
       		drawContours( frame, contours_poly, i, color_1, 1, 8, vector<Vec4i>(), 0, Point() );
       		//rectangle( frame, boundRect[i].tl(), boundRect[i].br(), color_1, 2, 8, 0 );
       		rectangle( frame, filteredRect[i].tl(), filteredRect[i].br(), color_1, 2, 8, 0 );
       		//circle( frame, center[i], (int)radius[i], color_1, 2, 8, 0 );
		
       		//drawContours( drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
       		//rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );
       		//circle( drawing, center[i], (int)radius[i], color, 2, 8, 0 );
					
     	}
	//imshow( "Contours", drawing );
	imshow( "Final Form", frame );
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
