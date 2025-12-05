#include "test_detectmultiscale.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <stdexcept>

using namespace cv;
using namespace std;

PalmDetector::PalmDetector(int deviceId, const std::string& cascadePath)
{

    cap.open(deviceId);
    if (!cap.isOpened()) {
        throw runtime_error("Error: Unable to open camera device");
    }

    if (!palmCascade.load(cascadePath)) {
        throw runtime_error("Error: Failed to load palm cascade: " + cascadePath);
    }


    claheCr = createCLAHE(2.0, Size(8,8));
    claheCb = createCLAHE(2.0, Size(8,8));
}

bool PalmDetector::getAnnotatedFrame(cv::Mat& frame, std::vector<cv::Point>& centers)
{

    cap >> frame;
    if (frame.empty()) return false;


    Mat ycrcb;
    cvtColor(frame, ycrcb, COLOR_BGR2YCrCb);
    vector<Mat> ch;
    split(ycrcb, ch);
    claheCr->apply(ch[1], ch[1]);
    claheCb->apply(ch[2], ch[2]);
    merge(ch, ycrcb);

    Mat skinMask;
    inRange(ycrcb, Scalar(0, 125, 70), Scalar(255, 180, 140), skinMask);


    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5,5));
    morphologyEx(skinMask, skinMask, MORPH_OPEN, kernel);
    morphologyEx(skinMask, skinMask, MORPH_CLOSE, kernel);


    vector<vector<Point>> contours;
    findContours(skinMask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    bool foundPalm = false;
    for (auto& cnt : contours) {
        double area = contourArea(cnt);
        if (area < 5000) continue;


        vector<Point> approx;
        approxPolyDP(cnt, approx, arcLength(cnt, true)*0.02, true);
        if (approx.size() == 4 && isContourConvex(approx)) {

            Rect br = boundingRect(approx);
            double ratio = double(br.width) / br.height;
            if (ratio < 0.5 || ratio > 2.0) continue;


            Mat roiMask = Mat::zeros(skinMask.size(), CV_8U);
            drawContours(roiMask, vector<vector<Point>>{approx}, 0, Scalar(255), FILLED);


            Mat dist;
            distanceTransform(roiMask, dist, DIST_L2, 5);
            double maxVal;
            Point maxLoc;
            minMaxLoc(dist, nullptr, &maxVal, nullptr, &maxLoc);
            if (maxVal < 15) continue;

            centers.push_back(maxLoc);

            rectangle(frame, br, Scalar(255,0,0), 2);
            circle(frame, maxLoc, int(maxVal*0.5), Scalar(0,255,0), 2);
            foundPalm = true;
            break;
        }
    }


    if (!foundPalm) {
        vector<Rect> palms;
        palmCascade.detectMultiScale(frame, palms, 1.1, 5, 0, Size(80,80));
        if (!palms.empty()) {

            Rect best = *max_element(palms.begin(), palms.end(), [](auto&a,auto&b){return a.area()<b.area();});
            Point centerPt(best.x + best.width/2, best.y + best.height/2);
            centers.push_back(centerPt);

            rectangle(frame, best, Scalar(0,0,255), 2);
            circle(frame, centerPt, 10, Scalar(0,255,255), 2);
            foundPalm = true;
        }
    }


    if (!foundPalm) {
        Mat dist;
        distanceTransform(skinMask, dist, DIST_L2, 5);
        double maxVal;
        Point maxLoc;
        minMaxLoc(dist, nullptr, &maxVal, nullptr, &maxLoc);
        if (maxVal >= 10) {
            centers.push_back(maxLoc);
            circle(frame, maxLoc, int(maxVal*0.5), Scalar(255,255,0), 2);
        }
    }

    return true;
}
