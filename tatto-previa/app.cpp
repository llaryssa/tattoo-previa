#include "stdafx.h"

#include "app.h"
#include "util.h"

#include <thread>
#include <chrono>

#include <math.h>
#include <string>  

#include <omp.h>

// Constructor
Kinect::Kinect()
{
	// Initialize
	initialize();
}

// Destructor
Kinect::~Kinect()
{
	// Finalize
	finalize();
}

void Kinect::setTattoo(char* filename)
{
	// -1 is to guarantee that the transparancy is read
	tattooSrcMat = cv::imread(filename, -1);
	tattooMat = tattooSrcMat.clone();
	if (!tattooMat.data) {
		std::cout << "NAO TEM IMAGEM";
	}
}

// Processing
void Kinect::run()
{
	// Main Loop
	while (true){
		// Update Data
		update();

		// Draw Data
		draw();

		// Show Data
		show();

		// Key Check
		const int key = cv::waitKey(10);
		if (key == VK_ESCAPE){
			break;
		}
	}
}

// Initialize
void Kinect::initialize()
{
	cv::setUseOptimized(true);

	// Initialize Sensor
	initializeSensor();

	// Initialize Tattoo
	initializeTattoo();

	// Initialize Color
	initializeColor();

	// Initialize Body
	initializeBody();

	// Wait a Few Seconds until begins to Retrieve Data from Sensor ( about 2000-[ms] )
	std::this_thread::sleep_for(std::chrono::seconds(2));
}

// Initialize Sensor
inline void Kinect::initializeSensor()
{
	// Open Sensor
	ERROR_CHECK(GetDefaultKinectSensor(&kinect));

	ERROR_CHECK(kinect->Open());

	// Check Open
	BOOLEAN isOpen = FALSE;
	ERROR_CHECK(kinect->get_IsOpen(&isOpen));
	if (!isOpen){
		throw std::runtime_error("failed IKinectSensor::get_IsOpen( &isOpen )");
	}

	// Retrieve Coordinate Mapper
	ERROR_CHECK(kinect->get_CoordinateMapper(&coordinateMapper));
}

// Initialize Tattoo
inline void Kinect::initializeTattoo()
{
	rightElbow = cv::Point(0, 0);
	rightHand = cv::Point(0, 0);

	tattooLocation = cv::Point(0, 0);
}

// Initialize Color
inline void Kinect::initializeColor()
{
	// Open Color Reader
	ComPtr<IColorFrameSource> colorFrameSource;
	ERROR_CHECK(kinect->get_ColorFrameSource(&colorFrameSource));
	ERROR_CHECK(colorFrameSource->OpenReader(&colorFrameReader));

	// Retrieve Color Description
	ComPtr<IFrameDescription> colorFrameDescription;
	ERROR_CHECK(colorFrameSource->CreateFrameDescription(ColorImageFormat::ColorImageFormat_Bgra, &colorFrameDescription));
	ERROR_CHECK(colorFrameDescription->get_Width(&colorWidth)); // 1920
	ERROR_CHECK(colorFrameDescription->get_Height(&colorHeight)); // 1080
	ERROR_CHECK(colorFrameDescription->get_BytesPerPixel(&colorBytesPerPixel)); // 4

	// Allocation Color Buffer
	colorBuffer.resize(colorWidth * colorHeight * colorBytesPerPixel);
}

// Initialize Body
inline void Kinect::initializeBody()
{
	// Open Body Reader
	ComPtr<IBodyFrameSource> bodyFrameSource;
	ERROR_CHECK(kinect->get_BodyFrameSource(&bodyFrameSource));
	ERROR_CHECK(bodyFrameSource->OpenReader(&bodyFrameReader));

	// Initialize Body Buffer
	for (auto& body : bodies){
		body = nullptr;
	}

	// Color Table for Visualization
	colors[0] = cv::Vec3b(255, 0, 0); // Blue
	colors[1] = cv::Vec3b(0, 255, 0); // Green
	colors[2] = cv::Vec3b(0, 0, 255); // Red
	colors[3] = cv::Vec3b(255, 255, 0); // Cyan
	colors[4] = cv::Vec3b(255, 0, 255); // Magenta
	colors[5] = cv::Vec3b(0, 255, 255); // Yellow

	tattooLocation = cv::Point(0, 0);

}

// Finalize
void Kinect::finalize()
{
	cv::destroyAllWindows();

	// Release Body Buffer
	for (auto& body : bodies){
		SafeRelease(body);
	}

	// Close Sensor
	if (kinect != nullptr){
		kinect->Close();
	}
}

// Update Data
void Kinect::update()
{
	// Update Color
	updateColor();

	// Update Body
	updateBody();

	// Update Tattoo
	updateTattoo();
}

// Update Color
inline void Kinect::updateColor()
{
	// Retrieve Color Frame
	ComPtr<IColorFrame> colorFrame;
	const HRESULT ret = colorFrameReader->AcquireLatestFrame(&colorFrame);
	if (FAILED(ret)){
		return;
	}

	// Convert Format ( YUY2 -> BGRA )
	ERROR_CHECK(colorFrame->CopyConvertedFrameDataToArray(static_cast<UINT>(colorBuffer.size()), &colorBuffer[0], ColorImageFormat::ColorImageFormat_Bgra));
}

// Update Body
inline void Kinect::updateBody()
{
	// Retrieve Body Frame
	ComPtr<IBodyFrame> bodyFrame;
	const HRESULT ret = bodyFrameReader->AcquireLatestFrame(&bodyFrame);
	if (FAILED(ret)){
		return;
	}

	// Release Previous Bodies
	for (auto& body : bodies){
		SafeRelease(body);
	}

	// Retrieve Body Data
	ERROR_CHECK(bodyFrame->GetAndRefreshBodyData(static_cast<UINT>(bodies.size()), &bodies[0]));
}

// Update Image
inline void Kinect::updateTattoo()
{
	// centro da imagem
	//cv::Point2f center = cv::Point2f(round(tattooSrcMat.cols / 2), round(tattooSrcMat.rows / 2));
	cv::Point2f center = cv::Point2f(round(tattooMat.cols / 2), round(tattooMat.rows / 2));

	// vetor normalizado entre os pontos desejados
	cv::Point2f vector = cv::Point2f(rightHand.x - rightElbow.x, rightHand.y - rightElbow.y);
	float norm = sqrt((vector.x*vector.x) + (vector.y*vector.y));
	vector.x /= norm;
	vector.y /= norm;

	cv::Point2f axisVector = cv::Point2f(0, 1);

	// angulo do vetor entre os pontos desejados
	double cossine = (axisVector.x * vector.x) + (axisVector.y * vector.y);
	double angleInRadians = acos(cossine);
	double angle = angleInRadians*(57.2958);    // in degrees / counter-clockwise
	double scale = norm / (tattooSrcMat.rows*2);

	// transform tattoo
	cv::Mat R = cv::getRotationMatrix2D(center, angle, scale);
	cv::warpAffine(tattooSrcMat, tattooMat, R, tattooSrcMat.size(), cv::INTER_CUBIC);

	// define its location (meio do antebraco)
	tattooLocation = cv::Point((rightElbow.x + rightHand.x) / 2, (rightElbow.y + rightHand.y) / 2);






	/////////////////////////////////// testes /////////////////////////////////////////////
	// mostrando o vetor do braco e o angulo (teste)
	cv::line(colorMat, rightHand, rightElbow, cv::Scalar(255, 0, 0), 3);
	cv::putText(colorMat, std::to_string(angle), rightElbow, cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 1.5, cv::Scalar(0,255,0), 3);

}

// Draw Data
void Kinect::draw()
{
	// Draw Color
	drawColor();

	// Draw Body
	drawBody();

	// Draw Tattoo
	if (tattooLocation.x != 0 && tattooLocation.y != 0)
		drawTattoo();
}

// Draw Color
inline void Kinect::drawColor()
{
	// Create cv::Mat from Color Buffer
	colorMat = cv::Mat(colorHeight, colorWidth, CV_8UC4, &colorBuffer[0]);
}

// Draw Color
inline void Kinect::drawTattoo()
{

	std::cout << tattooLocation << std::endl;
	overlayTattoo(colorMat, tattooMat, tattooLocation, .85);
	cv::circle(colorMat, tattooLocation, 20, cv::Scalar(0, 0, 0));

}

// overlay an image with another given the point in the middle
inline void Kinect::overlayTattoo(cv::Mat& src, cv::Mat& overlay, const cv::Point& point, const double opacitasdy)
{
	cv::Point location = cv::Point(point.x - (overlay.cols / 2), point.y - (overlay.rows / 2));

#pragma omp parallel for
	for (int y = cv::max(location.y, 0); y < src.rows; ++y) {
		int fY = y - location.y;

		if (fY >= overlay.rows)
			break;

#pragma omp parallel for
		for (int x = cv::max(location.x, 0); x < src.cols; ++x) {
			int fX = x - location.x;

			if (fX >= overlay.cols)
				break;

			double opacity =
				((double)overlay.data[fY * overlay.step + fX * overlay.channels() + 3])/255.;

#pragma omp parallel for
			for (int c = 0; opacity > 0 && c < src.channels(); ++c) {
				unsigned char overlayPx = overlay.data[fY * overlay.step + fX * overlay.channels() + c];
				unsigned char srcPx = src.data[y * src.step + x * src.channels() + c];
				//src.data[y * src.step + src.channels() * x + c] = srcPx * (1. - opacity) + overlayPx * opacity;
				src.data[y * src.step + src.channels() * x + c] = overlayPx;
			}
		}
	}
}


// Draw Body
inline void Kinect::drawBody()
{
	// Draw Body Data to Color Data
#pragma omp parallel for
	for (int index = 0; index < BODY_COUNT; index++){
		ComPtr<IBody> body = bodies[index];
		if (body == nullptr){
			continue;
		}

		// Check Body Tracked
		BOOLEAN tracked = FALSE;
		ERROR_CHECK(body->get_IsTracked(&tracked));
		if (!tracked){
			continue;
		}

		// Retrieve Joints
		std::array<Joint, JointType::JointType_Count> joints;
		ERROR_CHECK(body->GetJoints(static_cast<UINT>(joints.size()), &joints[0]));

#pragma omp parallel for
		for (int type = 0; type < JointType::JointType_Count; type++){
			// Check Joint Tracked
			const Joint joint = joints[type];
			if (joint.TrackingState == TrackingState::TrackingState_NotTracked){
				continue;
			}

			// Draw Joint Position
			drawEllipse(colorMat, joint, 5, colors[index]);

			// Draw Left Hand State
			if (joint.JointType == JointType::JointType_HandLeft){
				HandState handState;
				TrackingConfidence handConfidence;
				ERROR_CHECK(body->get_HandLeftState(&handState));
				ERROR_CHECK(body->get_HandLeftConfidence(&handConfidence));

				drawHandState(colorMat, joint, handState, handConfidence);
			}

			// Draw Right Hand State
			if (joint.JointType == JointType::JointType_HandRight){
				HandState handState;
				TrackingConfidence handConfidence;
				ERROR_CHECK(body->get_HandRightState(&handState));
				ERROR_CHECK(body->get_HandRightConfidence(&handConfidence));

				drawHandState(colorMat, joint, handState, handConfidence);
			}



			// TESTE
			if (joint.JointType == JointType::JointType_HandRight) {
				ColorSpacePoint colorSpacePoint;
				ERROR_CHECK(coordinateMapper->MapCameraPointToColorSpace(joint.Position, &colorSpacePoint));
				const int x = static_cast<int>(colorSpacePoint.X + 0.5f);
				const int y = static_cast<int>(colorSpacePoint.Y + 0.5f);
				rightHand = cv::Point(x, y);
			}

			if (joint.JointType == JointType::JointType_ElbowRight) {
				ColorSpacePoint colorSpacePoint;
				ERROR_CHECK(coordinateMapper->MapCameraPointToColorSpace(joint.Position, &colorSpacePoint));
				const int x = static_cast<int>(colorSpacePoint.X + 0.5f);
				const int y = static_cast<int>(colorSpacePoint.Y + 0.5f);
				rightElbow = cv::Point(x, y);
			}
	
		}

		/*
		// Retrieve Joint Orientations
		std::array<JointOrientation, JointType::JointType_Count> orientations;
		ERROR_CHECK( body->GetJointOrientations( JointType::JointType_Count, &orientations[0] ) );
		*/

		/*
		// Retrieve Amount of Body Lean
		PointF amount;
		ERROR_CHECK( body->get_Lean( &amount ) );
		*/
	}
}

// Draw Ellipse
inline void Kinect::drawEllipse(cv::Mat& image, const Joint& joint, const int radius, const cv::Vec3b& color, const int thickness)
{
	if (image.empty()){
		return;
	}

	// Convert Coordinate System and Draw Joint
	ColorSpacePoint colorSpacePoint;
	ERROR_CHECK(coordinateMapper->MapCameraPointToColorSpace(joint.Position, &colorSpacePoint));
	const int x = static_cast<int>(colorSpacePoint.X + 0.5f);
	const int y = static_cast<int>(colorSpacePoint.Y + 0.5f);
	if ((0 <= x) && (x < image.cols) && (0 <= y) && (y < image.rows)){
		cv::circle(image, cv::Point(x, y), radius, static_cast<cv::Scalar>(color), thickness, cv::LINE_AA);
	}
}

// Draw Hand State
inline void Kinect::drawHandState(cv::Mat& image, const Joint& joint, HandState handState, TrackingConfidence handConfidence)
{
	if (image.empty()){
		return;
	}

	// Check Tracking Confidence
	if (handConfidence != TrackingConfidence::TrackingConfidence_High){
		return;
	}

	// Draw Hand State 
	const int radius = 75;
	const cv::Vec3b blue = cv::Vec3b(128, 0, 0), green = cv::Vec3b(0, 128, 0), red = cv::Vec3b(0, 0, 128);
	switch (handState){
		// Open
	case HandState::HandState_Open:
		drawEllipse(image, joint, radius, green, 5);
		break;
		// Close
	case HandState::HandState_Closed:
		drawEllipse(image, joint, radius, red, 5);
		break;
		// Lasso
	case HandState::HandState_Lasso:
		drawEllipse(image, joint, radius, blue, 5);
		break;
	default:
		break;
	}
}

// Show Data
void Kinect::show()
{
	// Show Body
	showBody();
}

// Show Body
inline void Kinect::showBody()
{
	if (colorMat.empty()){
		return;
	}

	// Resize Image
	cv::Mat resizeMat;
	const double scale = 0.5;
	cv::resize(colorMat, resizeMat, cv::Size(), scale, scale);

	// Show Image
	cv::imshow("Body", resizeMat);
}