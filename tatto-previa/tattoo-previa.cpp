// tatto-previa.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "iostream"

#include "app.h"

#include "Kinect.h"


void rotateImage(const cv::Mat &input, cv::Mat &output, cv::Point3f rotationVector, double dx, double dy, double dz, double f);

int main(int argc, char* argv[])
{
	
	//char* filename = "images/lena.png";
	char* filename = "images/rose.png";




	//cv::Mat img = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
	////cv::namedWindow("window", CV_WINDOW_AUTOSIZE);
	////cv::imshow("window", img);
	////cv::waitKey(0);

	//cv::Point3f p1 = cv::Point3f(0.5520, 0.1465, 1.1279);
	//cv::Point3f p2 = cv::Point3f(0.2633, 0.0129, 1.22325);

	//cv::Mat out;
	//rotateImage(img, out, p1-p2, 0, 0, 500, 500);
	//cv::namedWindow("window2", CV_WINDOW_AUTOSIZE);
	//cv::imshow("window2", out);
	//cv::waitKey(0);


	cv::Point3f pt(0, 0, 5);




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



void rotateImage(const cv::Mat &input, cv::Mat &output, cv::Point3f rotationVector, double gamma, double dx, double dy, double dz, double fx, double fy)
{
	cv::Mat mat;
	cv::Rodrigues(cv::Mat(rotationVector), mat);


	double zer[3] = { 0.0 };
	double zer1[4] = { 0.0 };
	cv::vconcat(mat, cv::Mat(1, 3, CV_32F, zer), mat);
	cv::hconcat(mat, cv::Mat(4, 1, CV_32F, zer1), mat);
	mat.at<float>(3, 3) = 1.0;


	//alpha = (alpha)*CV_PI / 180.;
	//beta = (beta)*CV_PI / 180.;
	gamma = (gamma)*CV_PI / 180.;

	// get width and height for ease of use in matrices
	double w = (double)input.cols;
	double h = (double)input.rows;

	// Projection 2D -> 3D matrix
	cv::Mat A1 = (cv::Mat_<double>(4, 3) <<
		1, 0, -w / 2,
		0, 1, -h / 2,
		0, 0, 0,
		0, 0, 1);

	//// Rotation matrices around the X, Y, and Z axis
	//Mat RX = (Mat_<double>(4, 4) <<
	//	1, 0, 0, 0,
	//	0, cos(alpha), -sin(alpha), 0,
	//	0, sin(alpha), cos(alpha), 0,
	//	0, 0, 0, 1);
	//Mat RY = (Mat_<double>(4, 4) <<
	//	cos(beta), 0, -sin(beta), 0,
	//	0, 1, 0, 0,
	//	sin(beta), 0, cos(beta), 0,
	//	0, 0, 0, 1);

	cv::Mat RZ = (cv::Mat_<double>(4, 4) <<
		cos(gamma), -sin(gamma), 0, 0,
		sin(gamma), cos(gamma), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);

	// Composed rotation matrix with (RX, RY, RZ)
	//Mat R = RX * RY * RZ;
	mat.convertTo(mat, A1.type());
	cv::Mat R = mat * RZ;

	// Translation matrix
	cv::Mat T = (cv::Mat_<double>(4, 4) <<
		1, 0, 0, dx,
		0, 1, 0, dy,
		0, 0, 1, dz,
		0, 0, 0, 1);

	// 3D -> 2D matrix
	cv::Mat A2 = (cv::Mat_<double>(3, 4) <<
		fx, 0, w / 2, 0,
		0, fy, h / 2, 0,
		0, 0, 1, 0);


	// Final transformation matrix
	cv::Mat trans = A2 * (T * (R * A1));


	// Apply matrix transformation
	cv::warpPerspective(input, output, trans, input.size(), cv::INTER_LANCZOS4);
}
