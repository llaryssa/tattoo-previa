#ifndef __APP__
#define __APP__

#include "targetver.h"

#include <Windows.h>
#include <Kinect.h>
#include <opencv2/opencv.hpp>

#include <vector>
#include <array>

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
	cv::Mat colorMat, tattooMat;
	cv::Point tattooLocation;

	cv::Point rightElbow;
	cv::Point rightHand;

	// Body Buffer
	std::array<IBody*, BODY_COUNT> bodies;
	std::array<cv::Vec3b, BODY_COUNT> colors;

public:
	// Constructor
	Kinect();

	// Destructor
	~Kinect();

	// Load the tattoo file
	void setTattoo(char* filename);

	// Processing
	void run();

private:
	// Initialize
	void initialize();

	// Initialize Sensor
	inline void initializeSensor();

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

	// Draw Data
	void draw();

	// Draw Color
	inline void drawColor();
	
	// Draw Tattoo
	inline void drawTattoo();

	// Draw Hand State
	inline void overlayTattoo(cv::Mat& src, cv::Mat& overlay, const cv::Point& location, const double opacity);
	
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
};

#endif // __APP__