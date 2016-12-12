// tatto-previa.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "iostream"

#include "app.h"

#include "Kinect.h"

cv::Mat convert_img(cv::Mat &image, double radius);

int main(int argc, char* argv[])
{
	
	char* filename = "images/emoticon.png";
	//char* filename = "images/rose.png";


	cv::Mat img = cv::imread(filename, CV_LOAD_IMAGE_COLOR);

	//std::cout << img << std::endl;


	//cv::cvtColor(img, img, CV_BGR2GRAY);
	//cv::namedWindow("window", CV_WINDOW_AUTOSIZE);
	//cv::imshow("window", img);
	//cv::waitKey(0);

	//cv::Point3f p1 = cv::Point3f(0.5520, 0.1465, 1.1279);
	//cv::Point3f p2 = cv::Point3f(0.2633, 0.0129, 1.22325);

	//cv::Mat out;
	//rotateImage(img, out, p1-p2, 0, 0, 500, 500);
	//cv::namedWindow("window2", CV_WINDOW_AUTOSIZE);
	//cv::imshow("window2", out);
	//cv::waitKey(0);



	//cv::Mat output = convert_img(img, .5);
	//cv::namedWindow("window", CV_WINDOW_AUTOSIZE);
	//cv::imshow("window", output);
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

cv::Point2f convert_pt(cv::Point2f point, int w, int h, double r_factor)
{
	//center the point at 0,0
	cv::Point2f pc(point.x - w / 2, point.y - h / 2);

	// these are your free parameters
	// cylinder focal length and radius
	float f = -w;
	float r = w*r_factor;

	float omega = w / 2;
	float z0 = f - sqrt(r*r - omega*omega);

	float zc = (2 * z0 + sqrt(4 * z0*z0 - 4 * (pc.x*pc.x / (f*f) + 1)*(z0*z0 - r*r))) / (2 * (pc.x*pc.x / (f*f) + 1));
	cv::Point2f final_point(pc.x*zc / f, pc.y*zc / f);
	final_point.x += w / 2;
	final_point.y += h / 2;
	return final_point;
}

cv::Mat convert_img(cv::Mat &input, double radius) 
{
	int height = input.rows;
	int width = input.cols;

	bool found = false;
	double xf, yf;


	cv::Mat output = cv::Mat::ones(2 * height, 2 * width, input.type());

	for (int y = 0; y < output.rows; y++)
	{
		for (int x = 0; x < output.cols; x++)
		{
			cv::Point2f current_pos(x-height/2, y-width/2);
			current_pos = convert_pt(current_pos, width, height, radius);

			cv::Point2i top_left((int)current_pos.x, (int)current_pos.y); //top left because of integer rounding

			//make sure the point is actually inside the original image
			if (top_left.x < 0 ||
				top_left.x > width - 2 ||
				top_left.y < 0 ||
				top_left.y > height - 2)
			{
				if (!found) {
					found = true;
					xf = y;
					yf = x;
				}
				continue;
			}

			//bilinear interpolation
			float dx = current_pos.x - top_left.x;
			float dy = current_pos.y - top_left.y;

			float weight_tl = (1.0 - dx) * (1.0 - dy);
			float weight_tr = (dx)* (1.0 - dy);
			float weight_bl = (1.0 - dx) * (dy);
			float weight_br = (dx)* (dy);

			cv::Vec3b v1 = input.at<cv::Vec3b>(top_left);
			cv::Vec3b v2 = input.at<cv::Vec3b>(top_left.y, top_left.x + 1);
			cv::Vec3b v3 = input.at<cv::Vec3b>(top_left.y + 1, top_left.x);
			cv::Vec3b v4 = input.at<cv::Vec3b>(top_left.y + 1, top_left.x + 1);
			cv::Vec3b value(0, 0, 0);

			for (int c = 0; c < input.channels(); ++c) {
				value[c] = weight_tl * v1[c] +
					weight_tr * v2[c] +
					weight_bl * v3[c] +
					weight_br * v4[c];
			}
			output.at<cv::Vec3b>(y, x) = value;
		}
	}



	return output;
}
