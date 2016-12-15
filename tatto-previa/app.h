#ifndef __APP__
#define __APP__

#include "targetver.h"

#include <Windows.h>
#include <Kinect.h>
#include <opencv2/opencv.hpp>

#include <vector>
#include <array>
#include <string>
using std::string;
const string imagesPath[] = { "emoticon.png", "rose.png", "windows.png", "yy.png", "ancora.png", "cruz.png", "escorpiao.png", "flor.png", "heart.png", "leao.png", "patas.png", "rose2.png", "seta.png", "tat.png", "tat4.png", "tr.png" };

#include <wrl/client.h>
using namespace Microsoft::WRL;

class Kinect
{
private:
	// Sensor
	ComPtr<IKinectSensor> kinect;

	// Coordinate Mapper
	ComPtr<ICoordinateMapper> coordinateMapper;

	// Reader
	ComPtr<IColorFrameReader> colorFrameReader;
	ComPtr<IBodyFrameReader> bodyFrameReader;

	// Color Buffer
	std::vector<BYTE> colorBuffer;
	int colorWidth;
	int colorHeight;
	unsigned int colorBytesPerPixel;
	cv::Mat colorMat, tattooSrcMat, tattooMat;
	cv::Point tattooLocation;


	cv::Point rightElbow;
	cv::Point rightWrist;

	cv::Point rightHand;
	cv::Point leftHand;

	cv::Point3d target1;
	cv::Point3d target2;

	// For UI
	cv::Point buttonBiggerLocation;
	cv::Point buttonSmallerLocation;
	cv::Point buttonImageLocation;
	cv::Point buttonNextLocation;
	float buttonRadius = 80;
	float zoomFactor = 1;

	// Body Buffer
	std::array<IBody*, BODY_COUNT> bodies;
	std::array<cv::Vec3b, BODY_COUNT> colors;

public:
	// Constructor
	Kinect();

	// Destructor
	~Kinect();

	
	// Load the tattoo file
	void setTattoo(const char* filename);

	// Processing
	void run();

private:
	// Initialize
	void initialize();

	// Initialize Sensor
	inline void initializeSensor();

	//// Initialize UI
	//inline void initializeUI();

	// Initialize Tattoo
	inline void initializeTattoo();

	// Initialize Color
	inline void initializeColor();

	// Initialize Body
	inline void initializeBody();

	// Finalize
	void finalize();


	// Update Data
	void update();

	// Update Color
	inline void updateColor();

	// Update Body
	inline void updateBody();

	// Update Tattoo
	inline void updateTattoo();

	// Update UI
	inline void updateUI();

	// Draw Data
	void draw();

	// Draw Color
	inline void drawColor();
	
	// Draw Tattoo
	inline void drawTattoo();

	// Auxiliary for the tattoo
	inline void overlayTattoo(cv::Mat& src, cv::Mat& overlay, const cv::Point& location, const double opacity);
	
	inline void rotateImage(const cv::Mat &input, cv::Mat &output, double alpha, double beta, double gamma, double dx, double dy, double dz, double fx, double fy);
	inline void rotateImage(const cv::Mat &input, cv::Mat &output, cv::Point3f rotationVector, double gamma, double dx, double dy, double dz, double fx, double fy);

	inline cv::Mat cylinderProjection(const cv::Mat &input, double focalLength, double radius);

	inline cv::Point2f convertPointCylinder(const cv::Point2f point, int w, int h, double r_factor);

	// Draw Body
	inline void drawBody();

	// Draw Circle
	inline void drawEllipse(cv::Mat& image, const Joint& joint, const int radius, const cv::Vec3b& color, const int thickness = -1);

	// Draw Hand State
	inline void drawHandState(cv::Mat& image, const Joint& joint, HandState handState, TrackingConfidence handConfidence);

	// Show Data
	void show();

	// Show Color
	inline void showColor();

	// Show Body
	inline void showBody();
	
	// Next Tattoo
	inline void nextTattoo();
	
	// Change Tattoo
	inline void changeTattoo();
	
	//updateNextImageFrame
	void updateNextImageFrame();
};

#endif // __APP__
