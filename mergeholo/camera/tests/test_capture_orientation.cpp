#include "CaptureOrientation.h"
#include "multiviewCameraOrbit.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cstdlib>
#include <cmath>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::exit(1);
    }
}

void testPixelMappings()
{
    cv::Mat source = (cv::Mat_<unsigned char>(2, 3) << 1, 2, 3, 4, 5, 6);
    const cv::Mat clockwise = rotateCaptureImage(source, CaptureRotation::Clockwise90);
    const cv::Mat counterclockwise = rotateCaptureImage(
        source, CaptureRotation::CounterClockwise90);

    expect(clockwise.rows == 3 && clockwise.cols == 2,
        "clockwise rotation must swap rows and columns");
    expect(counterclockwise.rows == 3 && counterclockwise.cols == 2,
        "counterclockwise rotation must swap rows and columns");
    expect(clockwise.type() == source.type() && counterclockwise.type() == source.type(),
        "rotation must preserve matrix type");

    const unsigned char expectedClockwise[3][2] = {
        { 4, 1 },
        { 5, 2 },
        { 6, 3 }
    };
    const unsigned char expectedCounterclockwise[3][2] = {
        { 3, 6 },
        { 2, 5 },
        { 1, 4 }
    };
    for (int row = 0; row < clockwise.rows; ++row) {
        for (int column = 0; column < clockwise.cols; ++column) {
            expect(clockwise.at<unsigned char>(row, column)
                    == expectedClockwise[row][column],
                "clockwise pixel mapping mismatch");
            expect(counterclockwise.at<unsigned char>(row, column)
                    == expectedCounterclockwise[row][column],
                "counterclockwise pixel mapping mismatch");
        }
    }
}

void testSpatialDepthMappingAndAlignment()
{
    cv::Mat rgb(2, 3, CV_8UC3);
    cv::Mat depth(2, 3, CV_32FC3);
    for (int row = 0; row < rgb.rows; ++row) {
        for (int column = 0; column < rgb.cols; ++column) {
            const int id = row * rgb.cols + column + 1;
            rgb.at<cv::Vec3b>(row, column) = cv::Vec3b(
                static_cast<unsigned char>(id),
                static_cast<unsigned char>(id + 10),
                static_cast<unsigned char>(id + 20));
            depth.at<cv::Vec3f>(row, column) = cv::Vec3f(
                static_cast<float>(id + 100),
                static_cast<float>(id + 200),
                static_cast<float>(id));
        }
    }

    cv::Mat rotatedRgb = rotateCaptureImage(rgb, CaptureRotation::CounterClockwise90);
    const cv::Mat rotatedDepth = rotateCaptureSpatialDepth(
        depth, CaptureRotation::CounterClockwise90);

    expect(rotatedRgb.type() == CV_8UC3, "RGB type must be preserved");
    expect(rotatedDepth.type() == CV_32FC3, "three-channel depth type must be preserved");
    expect(rotatedRgb.size() == rotatedDepth.size(),
        "RGB and depth rotations must keep matching dimensions");

    for (int row = 0; row < rotatedRgb.rows; ++row) {
        for (int column = 0; column < rotatedRgb.cols; ++column) {
            const unsigned char rgbId = rotatedRgb.at<cv::Vec3b>(row, column)[0];
            const float depthId = rotatedDepth.at<cv::Vec3f>(row, column)[2];
            expect(static_cast<float>(rgbId) == depthId,
                "RGB and depth pixels lost their one-to-one alignment");
        }
    }

    const cv::Vec3f topLeft = rotatedDepth.at<cv::Vec3f>(0, 0);
    expect(topLeft[0] == 203.0f && topLeft[1] == -103.0f && topLeft[2] == 3.0f,
        "spatial depth must map (X,Y,Z) to (Y,-X,Z) after pixel rotation");
    const cv::Vec3f oldPclPoint(103.0f, -203.0f, -3.0f);
    const cv::Vec3f newPclPoint(topLeft[0], -topLeft[1], -topLeft[2]);
    expect(newPclPoint[0] == -oldPclPoint[1]
            && newPclPoint[1] == oldPclPoint[0]
            && newPclPoint[2] == oldPclPoint[2],
        "depth_io PCL coordinates must rotate counterclockwise about +Z");

    const cv::Vec3f bottomRight = rotatedDepth.at<cv::Vec3f>(2, 1);
    expect(bottomRight[0] == 204.0f && bottomRight[1] == -104.0f
            && bottomRight[2] == 4.0f,
        "spatial depth coordinate mapping mismatch at the opposite corner");

    const unsigned char sourceValue = rgb.at<cv::Vec3b>(0, 2)[0];
    rotatedRgb.at<cv::Vec3b>(0, 0)[0] = 255;
    expect(rgb.at<cv::Vec3b>(0, 2)[0] == sourceValue,
        "rotated output must not alias source camera storage");

    const cv::Mat clockwiseRgb = rotateCaptureImage(rgb, CaptureRotation::Clockwise90);
    const cv::Mat clockwiseDepth = rotateCaptureSpatialDepth(
        depth, CaptureRotation::Clockwise90);
    expect(clockwiseRgb.size() == clockwiseDepth.size(),
        "clockwise RGB and depth rotations must keep matching dimensions");
    for (int row = 0; row < clockwiseRgb.rows; ++row) {
        for (int column = 0; column < clockwiseRgb.cols; ++column) {
            const unsigned char rgbId = clockwiseRgb.at<cv::Vec3b>(row, column)[0];
            const float depthId = clockwiseDepth.at<cv::Vec3f>(row, column)[2];
            expect(static_cast<float>(rgbId) == depthId,
                "clockwise RGB and depth pixels lost their one-to-one alignment");
        }
    }
    const cv::Vec3f clockwiseTopLeft = clockwiseDepth.at<cv::Vec3f>(0, 0);
    expect(clockwiseTopLeft[0] == -204.0f && clockwiseTopLeft[1] == 104.0f
            && clockwiseTopLeft[2] == 4.0f,
        "clockwise spatial depth must map (X,Y,Z) to (-Y,X,Z)");
    const cv::Vec3f clockwiseBottomRight = clockwiseDepth.at<cv::Vec3f>(2, 1);
    expect(clockwiseBottomRight[0] == -203.0f && clockwiseBottomRight[1] == 103.0f
            && clockwiseBottomRight[2] == 3.0f,
        "clockwise spatial depth coordinate mapping mismatch at the opposite corner");
}

void testEmptyInput()
{
    expect(rotateCaptureImage(cv::Mat(), CaptureRotation::Clockwise90).empty(),
        "empty input must produce empty output");
    expect(rotateCaptureSpatialDepth(
            cv::Mat(), CaptureRotation::CounterClockwise90).empty(),
        "empty spatial depth input must produce empty output");
}

void testCameraOrbitOrder()
{
    const MultiviewOrbitAngles first = multiviewOrbitAngles(90, 270, 1.0 / 3.0, 0, 0);
    const MultiviewOrbitAngles center = multiviewOrbitAngles(90, 270, 1.0 / 3.0, 134, 134);
    const MultiviewOrbitAngles last = multiviewOrbitAngles(90, 270, 1.0 / 3.0, 269, 269);

    expect(std::fabs(first.pitchDegrees - 22.4166666667) < 1e-9,
        "first multiview row must start above the subject");
    expect(std::fabs(first.yawDegrees + 44.8333333333) < 1e-9,
        "first multiview column must start at the subject's left");
    expect(std::fabs(center.pitchDegrees - 0.0833333333) < 1e-9
            && std::fabs(center.yawDegrees + 0.1666666667) < 1e-9,
        "middle-left multiview sample must be adjacent to the frontal view");
    expect(std::fabs(last.pitchDegrees + 22.4166666667) < 1e-9
            && std::fabs(last.yawDegrees - 44.8333333333) < 1e-9,
        "last multiview sample must finish below and to the right");
    expect(std::fabs(first.pitchDegrees + last.pitchDegrees) < 1e-9
            && std::fabs(first.yawDegrees + last.yawDegrees) < 1e-9,
        "multiview orbit samples must be symmetric around the frontal view");
}

void testSerializedOrientation()
{
    QTemporaryDir temporaryDirectory;
    expect(temporaryDirectory.isValid(), "temporary output directory could not be created");

    cv::Mat rgb(16, 32, CV_8UC3);
    rgb.colRange(0, 16).setTo(cv::Scalar(0, 0, 255));
    rgb.colRange(16, 32).setTo(cv::Scalar(255, 0, 0));
    const cv::Mat rotatedRgb = rotateCaptureImage(
        rgb, CaptureRotation::CounterClockwise90);
    const QString rgbPath = QDir(temporaryDirectory.path()).filePath("rotated.jpg");
    expect(cv::imwrite(rgbPath.toStdString(), rotatedRgb),
        "rotated RGB JPEG could not be written");

    const cv::Mat reloadedRgb = cv::imread(rgbPath.toStdString(), cv::IMREAD_COLOR);
    expect(reloadedRgb.rows == 32 && reloadedRgb.cols == 16,
        "saved RGB dimensions were not rotated");
    const cv::Vec3b top = reloadedRgb.at<cv::Vec3b>(8, 8);
    const cv::Vec3b bottom = reloadedRgb.at<cv::Vec3b>(24, 8);
    expect(top[0] > top[2] + 50, "rotated JPEG top half must be blue");
    expect(bottom[2] > bottom[0] + 50, "rotated JPEG bottom half must be red");

    cv::Mat depth(2, 3, CV_32FC3);
    for (int row = 0; row < depth.rows; ++row) {
        for (int column = 0; column < depth.cols; ++column) {
            const float id = static_cast<float>(row * depth.cols + column + 1);
            depth.at<cv::Vec3f>(row, column) = cv::Vec3f(id, id + 10.0f, id + 20.0f);
        }
    }
    const cv::Mat rotatedDepth = rotateCaptureSpatialDepth(
        depth, CaptureRotation::CounterClockwise90);
    const QString depthPath = QDir(temporaryDirectory.path()).filePath("rotated.tiff");
    const std::vector<int> tiffParameters = { cv::IMWRITE_TIFF_COMPRESSION, 1 };
    expect(cv::imwrite(depthPath.toStdString(), rotatedDepth, tiffParameters),
        "rotated depth TIFF could not be written");

    const cv::Mat reloadedDepth = cv::imread(depthPath.toStdString(), cv::IMREAD_UNCHANGED);
    expect(reloadedDepth.rows == 3 && reloadedDepth.cols == 2,
        "saved depth dimensions were not rotated");
    expect(reloadedDepth.type() == CV_32FC3, "saved depth type changed");
    const cv::Vec3f reloadedTopLeft = reloadedDepth.at<cv::Vec3f>(0, 0);
    const cv::Vec3f reloadedBottomRight = reloadedDepth.at<cv::Vec3f>(2, 1);
    expect(reloadedTopLeft[0] == 13.0f && reloadedTopLeft[1] == -3.0f
            && reloadedTopLeft[2] == 23.0f,
        "saved depth top-left XYZ does not match spatial counterclockwise mapping");
    expect(reloadedBottomRight[0] == 14.0f && reloadedBottomRight[1] == -4.0f
            && reloadedBottomRight[2] == 24.0f,
        "saved depth bottom-right XYZ does not match spatial counterclockwise mapping");
}

QByteArray readProjectFile(const QString& relativePath, const char* message)
{
    QDir projectDirectory(QCoreApplication::applicationDirPath());
    while (!projectDirectory.exists("mergeholo.pro") && projectDirectory.cdUp()) {
    }
    expect(projectDirectory.exists("mergeholo.pro"),
        "could not locate the mergeholo project root");

    QFile file(projectDirectory.absoluteFilePath(relativePath));
    expect(file.open(QIODevice::ReadOnly | QIODevice::Text), message);
    return file.readAll();
}

void testCaptureConsumersUseSharedOrientation()
{
    const QByteArray captureWindow = readProjectFile(
        "widgets/CaptureWindow.cpp", "CaptureWindow.cpp could not be read");
    const QByteArray captureSession = readProjectFile(
        "camera/CaptureSession.cpp", "CaptureSession.cpp could not be read");
    const QByteArray depthIo = readProjectFile(
        "vendor/point_cloud/src/depth_io.cpp", "depth_io.cpp could not be read");
    const QByteArray atlasRenderer = readProjectFile(
        "vendor/multiview/multiviewAtlasRenderer.cpp",
        "multiviewAtlasRenderer.cpp could not be read");
    const QByteArray batchRenderer = readProjectFile(
        "vendor/multiview/multiviewBatchRenderer.cpp",
        "multiviewBatchRenderer.cpp could not be read");
    const QByteArray modelMoveHandler = readProjectFile(
        "vendor/multiview/modelMoveHandler.cpp",
        "modelMoveHandler.cpp could not be read");
    const QByteArray uiPipelineTemplate = readProjectFile(
        "config/ui_pipeline_template.ini",
        "ui_pipeline_template.ini could not be read");
    const QByteArray cameraConfig = readProjectFile(
        "vendor/multiview/ModelMoveCameraConfig.h",
        "ModelMoveCameraConfig.h could not be read");

    expect(captureWindow.contains("rotateCaptureCounterClockwise90(frame.img2d)"),
        "main-window RGB capture is not rotated");
    expect(captureWindow.contains("rotateCaptureDepthCounterClockwise90(frame.img3d)"),
        "main-window pipeline depth is not spatially rotated");
    expect(captureWindow.contains("frame.depthMap.empty() ? frame.img3d : frame.depthMap"),
        "main-window display depth does not select the SDK depth map");
    expect(captureWindow.contains("rotateCaptureCounterClockwise90(depthDisplaySource)"),
        "main-window display depth is not rotated");
    expect(captureSession.contains("rotateCaptureCounterClockwise90(data.img2d)"),
        "standalone RGB capture is not rotated");
    expect(captureSession.contains("rotateCaptureDepthCounterClockwise90(data.img3d)"),
        "standalone depth capture is not spatially rotated");
    expect(depthIo.contains("p.x = X;")
            && depthIo.contains("p.y = -Y;")
            && depthIo.contains("p.z = -Z;"),
        "point-cloud loader coordinate convention changed; review the spatial rotation");
    expect(atlasRenderer.contains("rotateX(matrix, -renderPlan_.stepDegrees())")
            == false
            && atlasRenderer.contains("multiviewOrbitAngles")
            && atlasRenderer.contains("modelTransform_->getBound().center()")
            && atlasRenderer.contains("eye - orbitCenter")
            && atlasRenderer.contains("originalPostDrawCallback")
            && atlasRenderer.contains("restorePackState"),
        "atlas multiview must use absolute camera orbit poses");
    expect(batchRenderer.contains("advanceRow()") == false
            && batchRenderer.contains("multiviewOrbitAngles")
            && batchRenderer.contains("modelTransform_->getBound().center()")
            && batchRenderer.contains("eye - orbitCenter")
            && batchRenderer.contains("originalPostDrawCallback")
            && batchRenderer.contains("restoreViewerState"),
        "batch multiview must use absolute camera orbit poses");
    expect(modelMoveHandler.contains(
            "hasInitialRotateXDeg ? static_cast<float>") == false
            && modelMoveHandler.contains(
                "hasInitialRotateZDeg ? static_cast<float>") == false
            && modelMoveHandler.contains(
                "rotateZ(static_cast<float>(m_cameraConfig.initialRotateZDeg))"),
        "default model pose must remain fixed while the camera orbits");
    expect(uiPipelineTemplate.contains("multiview_camera_eye_dir_y=0.0")
            && uiPipelineTemplate.contains("multiview_camera_eye_dir_z=1.0")
            && uiPipelineTemplate.contains("multiview_camera_up_y=1.0")
            && uiPipelineTemplate.contains("multiview_camera_up_z=0.0"),
        "UI pipeline camera defaults must face the rotated capture coordinate system");
    expect(cameraConfig.contains(
            "eyeDirection = osg::Vec3d(0.0, 0.0, 1.0)")
            && cameraConfig.contains(
                "upDirection = osg::Vec3d(0.0, 1.0, 0.0)"),
        "multiview camera fallback must match the rotated capture coordinate system");
    expect(captureWindow.contains("rotateCaptureDepthCounterClockwise90(frame.img3d)"),
        "spatial capture rotation must remain enabled for downstream geometry");

    expect(captureWindow.count("rotateCaptureCounterClockwise90(frame.img2d)") == 1,
        "main-window RGB capture must be rotated exactly once");
    expect(captureWindow.count("rotateCaptureDepthCounterClockwise90(frame.img3d)") == 1,
        "main-window pipeline depth must be spatially rotated exactly once");
    expect(captureSession.count("rotateCaptureCounterClockwise90(data.img2d)") == 1,
        "standalone RGB capture must be rotated exactly once");
    expect(captureSession.count("rotateCaptureDepthCounterClockwise90(data.img3d)") == 1,
        "standalone depth capture must be spatially rotated exactly once");

    expect(captureWindow.indexOf("rotateCaptureCounterClockwise90(frame.img2d)")
            < captureWindow.indexOf("latestRgb_ ="),
        "main-window RGB must be rotated before publishing the frame");
    expect(captureWindow.indexOf("rotateCaptureDepthCounterClockwise90(frame.img3d)")
            < captureWindow.indexOf("latestDepthForPipeline_ ="),
        "main-window depth must be spatially rotated before publishing the frame");
    expect(captureSession.indexOf("rotateCaptureCounterClockwise90(data.img2d)")
            < captureSession.indexOf("if (options.showPreview)"),
        "standalone RGB must be rotated before preview and saving");
    expect(captureSession.indexOf("rotateCaptureDepthCounterClockwise90(data.img3d)")
            < captureSession.indexOf("if (options.showPreview)"),
        "standalone depth must be spatially rotated before preview and saving");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    testPixelMappings();
    testSpatialDepthMappingAndAlignment();
    testEmptyInput();
    testCameraOrbitOrder();
    testSerializedOrientation();
    testCaptureConsumersUseSharedOrientation();
    std::cout << "capture orientation tests passed" << std::endl;
    return 0;
}
