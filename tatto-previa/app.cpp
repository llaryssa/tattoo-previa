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
		std::cout << "ERROR: There is no image" << std::endl;
		ERROR("There is no image");
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

	// Update UI
	updateUI();
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

	const double distLeftHandButBigger = cv::norm(leftHand - buttonBiggerLocation);
	const double distLeftHandButSmaller = cv::norm(leftHand - buttonSmallerLocation);
	const double distRightHandButBigger = cv::norm(rightHand - buttonBiggerLocation);
	const double distRightHandButSmaller = cv::norm(rightHand - buttonSmallerLocation);
	if (distLeftHandButBigger < buttonRadius || distRightHandButBigger < buttonRadius) {
		zoomFactor = zoomFactor * 1.02;
	}
	if (distLeftHandButSmaller < buttonRadius || distRightHandButSmaller < buttonRadius) {
		zoomFactor = zoomFactor * 0.98;
	}



	// centro da imagem
	//cv::Point2f center = cv::Point2f(round(tattooSrcMat.cols / 2), round(tattooSrcMat.rows / 2));
	cv::Point2f center = cv::Point2f(round(tattooMat.cols / 2), round(tattooMat.rows / 2));

	// vetor normalizado entre os pontos desejados
	cv::Point2f vector = cv::Point2f(rightHand.x - rightElbow.x, rightHand.y - rightElbow.y);
	float norm = sqrt((vector.x*vector.x) + (vector.y*vector.y));
	vector.x /= norm;
	vector.y /= norm;

	cv::Point2f axisVector(0, 1);
	float factor = 1;

	// angulo do vetor entre os pontos desejados
	double cossine = (axisVector.x * vector.x) + (axisVector.y * vector.y);

	// z value of the cross product
	if ((vector.x*axisVector.y - vector.y*axisVector.x) < 0)
		factor = -1;
	
	double angleInRadians = acos(cossine);
	double angle = factor * angleInRadians*(57.2958);    // in degrees / counter-clockwise
	double scale = norm / (tattooSrcMat.rows*2);
	scale *= zoomFactor;

	// transform tattoo
	cv::Mat R = cv::getRotationMatrix2D(center, angle, scale);
	cv::warpAffine(tattooSrcMat, tattooMat, R, tattooSrcMat.size(), cv::INTER_CUBIC);




	///////////////////////////////////////

	//CameraIntrinsics calib;
	//coordinateMapper->GetDepthCameraIntrinsics(&calib);
	//cv::Point3f vector3d = target2 - target1;
	//float norm3d = sqrt((vector3d.x*vector3d.x) + (vector3d.y*vector3d.y) + (vector3d.z*vector3d.z));
	//vector3d.x /= norm3d;
	//vector3d.y /= norm3d;
	//vector3d.z /= norm3d;

	//cv::Point3f axisX(1, 0, 0);
	//cv::Point3f axisY(0, 1, 0);
	//cv::Point3f axisZ(0, 0, 1);

	//double cos_alpha = (axisX.x * vector3d.x) + (axisX.y * vector3d.y) + (axisX.z * vector3d.z);
	//double cos_beta = (axisY.x * vector3d.x) + (axisY.y * vector3d.y) + (axisY.z * vector3d.z);
	//double cos_gamma = (axisZ.x * vector3d.x) + (axisZ.y * vector3d.y) + (axisZ.z * vector3d.z);
	//
	//double alpha = acos(cos_alpha);
	//double beta = acos(cos_beta);
	//double gamma = acos(cos_gamma);

	//rotateImage(tattooSrcMat, tattooMat, alpha, beta, gamma, 0, 0, 1000, calib.FocalLengthX, calib.FocalLengthY);

	//////////////////////////////////////////////////



	// pega o vetor 3d e projeta o ponto medio dele nas coord de tela
	CameraSpacePoint* cameraLocationPoint = new CameraSpacePoint();
	cameraLocationPoint->X = (target1.x + target2.x) / 2;
	cameraLocationPoint->Y = (target1.y + target2.y) / 2;
	cameraLocationPoint->Z = (target1.z + target2.z) / 2;
	ColorSpacePoint colorLocationPoint;
	ERROR_CHECK(coordinateMapper->MapCameraPointToColorSpace(*cameraLocationPoint, &colorLocationPoint));
	// define the tattoo print location
	tattooLocation = cv::Point(colorLocationPoint.X, colorLocationPoint.Y);




	///////////////////////////////////// testes /////////////////////////////////////////////
	//// mostrando o vetor do braco e o angulo (teste)
	cv::line(colorMat, rightHand, rightElbow, cv::Scalar(255, 0, 0), 3);
	//cv::putText(colorMat, std::to_string(cossine), rightElbow, cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 1.5, cv::Scalar(0,255,0), 3);
	//cv::putText(colorMat, std::to_string(angle), rightElbow, cv::FONT_HERSHEY_SCRIPT_SIMPLEX, 1.5, cv::Scalar(0, 255, 0), 3);

}

void Kinect::updateUI()
{
	buttonRadius = 110;
	const float padding = 30;

	buttonBiggerLocation = cv::Point2d(250 + buttonRadius + padding, colorMat.rows - buttonRadius - padding);
	buttonSmallerLocation = cv::Point2d(250 + buttonRadius * 3 + 2 * padding, colorMat.rows - buttonRadius - padding);

	const int fontFace = cv::FONT_HERSHEY_DUPLEX;
	const float fontScale = 5;
	const float fontThickness = 10;
	const cv::Scalar fontColor = cv::Scalar::all(255);
	const cv::Scalar buttonColor = cv::Scalar::all(60);

	cv::Size textSizePlus = cv::getTextSize("+", cv::FONT_HERSHEY_DUPLEX, fontScale, fontThickness, 0);
	cv::Size textSizeMinus = cv::getTextSize("-", cv::FONT_HERSHEY_DUPLEX, fontScale, fontThickness, 0);

	cv::circle(colorMat, buttonBiggerLocation, buttonRadius, buttonColor, -1);
	cv::putText(colorMat, "+", buttonBiggerLocation + cv::Point(-62, 50), fontFace, fontScale, fontColor, fontThickness);

	cv::circle(colorMat, buttonSmallerLocation, buttonRadius, buttonColor, -1);
	cv::putText(colorMat, "-", buttonSmallerLocation + cv::Point(-65, 50), fontFace, fontScale, fontColor, fontThickness);
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
	overlayTattoo(colorMat, tattooMat, tattooLocation, .85);
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


inline void Kinect::rotateImage(const cv::Mat &input, cv::Mat &output, cv::Point3f rotationVector, double gamma, double dx, double dy, double dz, double fx, double fy)
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

inline void Kinect::rotateImage(const cv::Mat &input, cv::Mat &output, double alpha, double beta, double gamma, double dx, double dy, double dz, double fx, double fy)
{

	//alpha = (alpha)*CV_PI / 180.;
	//beta = (beta)*CV_PI / 180.;
	//gamma = (gamma)*CV_PI / 180.;

	// get width and height for ease of use in matrices
	double w = (double)input.cols;
	double h = (double)input.rows;

	// Projection 2D -> 3D matrix
	cv::Mat A1 = (cv::Mat_<double>(4, 3) <<
		1, 0, -w / 2,
		0, 1, -h / 2,
		0, 0, 0,
		0, 0, 1);

	// Rotation matrices around the X, Y, and Z axis
	cv::Mat RX = (cv::Mat_<double>(4, 4) <<
		1, 0, 0, 0,
		0, cos(alpha), -sin(alpha), 0,
		0, sin(alpha), cos(alpha), 0,
		0, 0, 0, 1);
	cv::Mat RY = (cv::Mat_<double>(4, 4) <<
		cos(beta), 0, -sin(beta), 0,
		0, 1, 0, 0,
		sin(beta), 0, cos(beta), 0,
		0, 0, 0, 1);
	cv::Mat RZ = (cv::Mat_<double>(4, 4) <<
		cos(gamma), -sin(gamma), 0, 0,
		sin(gamma), cos(gamma), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);

	// Composed rotation matrix with (RX, RY, RZ)
	cv::Mat R = RX * RY * RZ;


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
			if (joint.JointType == JointType::JointType_HandLeft) {
				ColorSpacePoint colorSpacePoint;
				ERROR_CHECK(coordinateMapper->MapCameraPointToColorSpace(joint.Position, &colorSpacePoint));
				const int x = static_cast<int>(colorSpacePoint.X + 0.5f);
				const int y = static_cast<int>(colorSpacePoint.Y + 0.5f);
				leftHand = cv::Point(x, y);
			}

			if (joint.JointType == JointType::JointType_HandRight) {
				ColorSpacePoint colorSpacePoint;
				ERROR_CHECK(coordinateMapper->MapCameraPointToColorSpace(joint.Position, &colorSpacePoint));
				const int x = static_cast<int>(colorSpacePoint.X + 0.5f);
				const int y = static_cast<int>(colorSpacePoint.Y + 0.5f);
				rightHand = cv::Point(x, y);

				///////////////////////////////
				CameraSpacePoint cameraPoint = joint.Position;
				target1 = cv::Point3d(cameraPoint.X, cameraPoint.Y, cameraPoint.Z);
			}

			if (joint.JointType == JointType::JointType_ElbowRight) {
				ColorSpacePoint colorSpacePoint;
				ERROR_CHECK(coordinateMapper->MapCameraPointToColorSpace(joint.Position, &colorSpacePoint));
				const int x = static_cast<int>(colorSpacePoint.X + 0.5f);
				const int y = static_cast<int>(colorSpacePoint.Y + 0.5f);
				rightElbow = cv::Point(x, y);

				///////////////////////////////
				CameraSpacePoint cameraPoint = joint.Position;
				target2 = cv::Point3d(cameraPoint.X, cameraPoint.Y, cameraPoint.Z);
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
