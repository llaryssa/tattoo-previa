// tatto-previa.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "iostream"

#include "app.h"

#include "Kinect.h"

using namespace std;


int _tmain(int argc, _TCHAR* argv[])
{
	
	//char* filename = "C:/Users/Laryssa/Documents/tattoo-previa/tatto-previa/images/rose.png";
	char* filename = "C:/Users/Laryssa/Documents/tattoo-previa/tatto-previa/images/rose.png";

	//cv::Mat img = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
	//cv::namedWindow("window", CV_WINDOW_AUTOSIZE);
	//cv::imshow("window", img);
	//cv::waitKey(0);


	try {
		Kinect kinect;
		kinect.setTattoo(filename);
		kinect.run();
	}
	catch (std::exception& ex){
		std::cout << ex.what() << std::endl;
	}


	return 0;
}

