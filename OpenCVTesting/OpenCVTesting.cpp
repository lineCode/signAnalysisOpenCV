/**
* Author: Liam Farrelly
* MeanShift Segmentation and floodFill postprocess from: https://sourceforge.net/p/emgucv/opencv/ci/3a873cb051efd9126e7dd05e3b47b046a0998dee/tree/samples/cpp/meanshift_segmentation.cpp#l16
*
*/

#include "stdafx.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"

#include <iostream>

using namespace cv;
using namespace std;

/// Global variables
Mat src, dst;
Mat grayscale, detected_edges;

int morph_elem = 0;
int morph_size = 0;
int morph_operator = 0;
int const max_operator = 4;
int const max_elem = 2;
int const max_kernel_size = 21;
bool kmeansBtn = false;

string msWin = "meanshift";
int spatialRad, colorRad, maxPyrLevel, contrastConst;
Mat img, res, heatMap;

const char* window_name = "Untampered image";
const char* canny_window = "Canny image";
int lowThreshold,kernel_size;

/** Function Headers */
void meanShiftSegmentation(Mat&, Mat&);
void floodFillPostprocess(Mat&, const Scalar&);
void getHistogram(Mat&);
void toGreyscale(Mat&, Mat&);
void cannyThreshold(Mat&, Mat&);
void chunkImage(Mat&, vector<Mat>&);
int countEdges(Mat&);
void overlayImg(int, Rect&);
void boundedContours(Mat&, Mat&);
void contourDraw(Mat&, vector<vector<Point>>, vector<Vec4i>, int);
Mat KMeans_Clustering(Mat&);

//void Morphology_Operations(int, void*);

/*
Kmeans -> greyscale -> contours -> book page 69/70 -> find connected regions -> labeled -> analyse regions to try find text
*/


/**
* @function main
*/
int main(int, char** argv)
{
	//![load]
	img = imread(argv[1], IMREAD_COLOR); // Load an image

	if (img.empty())
	{
		return -1;
	}

	/// Default start
	spatialRad = 10;
	colorRad = 30;
	maxPyrLevel = 1;
	contrastConst = 1.4;
	kernel_size = 3;

	namedWindow(msWin, CV_WINDOW_AUTOSIZE);
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);
	namedWindow(canny_window, CV_WINDOW_AUTOSIZE);

	imshow(window_name, img);

	//KMeans_Clustering(0, 0);

	Mat reducedColour;

	reducedColour = KMeans_Clustering(img);
	imshow(window_name, img);
	//increaseContrast(0, 0);
	imshow(msWin, reducedColour);

	Mat grey;
	toGreyscale(reducedColour, grey);

	Mat canny;
	cannyThreshold(grey, canny);

	//Mat outlinedImg = img;

	//boundedContours(grey, outlinedImg);
	namedWindow("img w/outlines", CV_WINDOW_AUTOSIZE);
	imshow("img w/outlines", canny);

	imwrite("../data/outlinedCharacters.jpg", canny);

	//vector<Mat> imageChunks;

	//chunkImage(dst, imageChunks);

	//namedWindow("heat map", CV_WINDOW_AUTOSIZE);
	//int length = imageChunks;
	/*for (int l = 0; l < imageChunks.size(); l++) {
		cout << "image #" << l << " has " << countEdges(imageChunks[l]) << " edges" << endl;

		imshow("image chunks", imageChunks[l]);
		waitKey(0);
	}*/

	waitKey();
	return 0;
}

void floodFillPostprocess(Mat& target, const Scalar& colorDiff = Scalar::all(1))
{
	CV_Assert(!target.empty());
	//RNG rng = theRNG();
	Mat mask(target.rows + 2, target.cols + 2, CV_8UC1, Scalar::all(0));
	for (int y = 0; y < target.rows; y++)
	{
		for (int x = 0; x < target.cols; x++)
		{
			if (mask.at<uchar>(y + 1, x + 1) == 0)
			{
				//get rgb values for this point
				Point3_<uchar>* p = target.ptr<Point3_<uchar> >(y, x);

				Scalar newVal(p->x, p->y, p->z);
				int four_connectivity = 4;
				floodFill(target, mask, Point(x, y), newVal, 0, colorDiff, colorDiff, four_connectivity);
			}
		}
	}
}

void toGreyscale(Mat& image, Mat& output) {
	cvtColor(image, output, CV_BGR2GRAY);

	imwrite("../data/GreyPostMSCont2.jpg", output);

	namedWindow("greyscale", CV_WINDOW_AUTOSIZE);
	imshow("greyscale", output);
}

void meanShiftSegmentation(Mat& target, Mat& output)
{
	//imwrite("../data/closedImg.jpg", img);
	cout << "spatialRad=" << spatialRad << "; "
		<< "colorRad=" << colorRad << "; "
		<< "maxPyrLevel=" << maxPyrLevel << endl;
	pyrMeanShiftFiltering(target, output, spatialRad, colorRad, maxPyrLevel);
	floodFillPostprocess(output, Scalar::all(2));
	//getHistogram(res);
	
	output.convertTo(output, -1, 1.4, 0);

	imwrite("../data/output2.jpg", output);
	
}
//Taken from https://docs.opencv.org/2.4/doc/tutorials/imgproc/histograms/histogram_calculation/histogram_calculation.html
void getHistogram(Mat& src) {

	if (!src.data)
	{
		return;
	}

	/// Separate the image in 3 places ( B, G and R )
	vector<Mat> bgr_planes;
	split(src, bgr_planes);

	/// Establish the number of bins
	int histSize = 256;

	/// Set the ranges ( for B,G,R) )
	float range[] = { 0, 256 };
	const float* histRange = { range };

	bool uniform = true; bool accumulate = false;

	Mat b_hist, g_hist, r_hist;
	Mat grayscale, gray_hist;
	cvtColor(img, grayscale, CV_BGR2GRAY);

	namedWindow("greyscale", CV_WINDOW_AUTOSIZE);
	imshow("greyscale", grayscale);

	/// Compute the histograms:
	calcHist(&bgr_planes[0], 1, 0, Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate);
	calcHist(&bgr_planes[1], 1, 0, Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate);
	calcHist(&bgr_planes[2], 1, 0, Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate);

	calcHist(&grayscale, 1, 0, Mat(), gray_hist, 1, &histSize, &histRange, uniform, accumulate);

	// Draw the histograms for B, G and R
	int hist_w = 512; int hist_h = 400;
	int bin_w = cvRound((double)hist_w / histSize);

	Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

	Mat grayHistImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));

	/// Normalize the result to [ 0, histImage.rows ]
	normalize(b_hist, b_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	normalize(g_hist, g_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	normalize(r_hist, r_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());

	normalize(gray_hist, gray_hist, 0, grayHistImage.rows, NORM_MINMAX, -1, Mat());

	/// Draw for each channel
	for (int i = 1; i < histSize; i++)
	{
		line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(b_hist.at<float>(i - 1))),
			Point(bin_w*(i), hist_h - cvRound(b_hist.at<float>(i))),
			Scalar(255, 0, 0), 2, 8, 0);
		line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(g_hist.at<float>(i - 1))),
			Point(bin_w*(i), hist_h - cvRound(g_hist.at<float>(i))),
			Scalar(0, 255, 0), 2, 8, 0);
		line(histImage, Point(bin_w*(i - 1), hist_h - cvRound(r_hist.at<float>(i - 1))),
			Point(bin_w*(i), hist_h - cvRound(r_hist.at<float>(i))),
			Scalar(0, 0, 255), 2, 8, 0);

		line(grayHistImage, Point(bin_w*(i - 1), hist_h - cvRound(gray_hist.at<float>(i - 1))),
			Point(bin_w*(i), hist_h - cvRound(gray_hist.at<float>(i))),
			Scalar(127, 127, 127), 2, 8, 0);
	}

	/// Display
	namedWindow("calcHist Demo", CV_WINDOW_AUTOSIZE);
	imshow("calcHist Demo", histImage);

	namedWindow("greyscale hist", CV_WINDOW_AUTOSIZE);
	imshow("greyscale hist", grayHistImage);

}


//Canny edge detection, code from OpenCV: https://docs.opencv.org/2.4/doc/tutorials/imgproc/imgtrans/canny_detector/canny_detector.html
void cannyThreshold(Mat& target, Mat& output)
{
	/// Reduce noise with a kernel 3x3
	GaussianBlur(target, output, Size(3, 3),0, 0);

	/// Canny detector
	int ratio = 3, lowThreshold = 20;
	Canny(output, output, lowThreshold, lowThreshold*ratio, kernel_size);

	/// Using Canny's output as a mask, we display our result
	dst = Scalar::all(0);

	//cout << "Edges: " << detected_edges << endl;

	target.copyTo(dst, output);

	imshow(canny_window, dst);

	imwrite("../data/cannyImage.jpg", dst);

	output = dst;
}

void chunkImage(Mat& toChunk, vector<Mat>& chunks) {
	int height = toChunk.rows;
	int width  = toChunk.cols;

	cout << "Image is of size " << height << " x " << width << endl;

	Size chunkDim(height/10, width/10);

	for (int i = 0; i < width; i += chunkDim.width) {
		int iAdjustment = 0;
		if (i + chunkDim.width > width) {
			iAdjustment = width - (i + chunkDim.width);
		}
		for (int j = 0; j < height; j += chunkDim.height){
			int jAdjustment = 0;
			if (j + chunkDim.height > height) {
				jAdjustment = height - (j + chunkDim.height);
			}
			Rect rect = Rect(i, j, chunkDim.width+iAdjustment, chunkDim.height+jAdjustment);
			
			//cout << "Rectangle region : " << j << " to " << (j + chunkDim.height) << " and " << i << " to " << (i + chunkDim.width) << " in image of size "
				//<< height << " x " << width << endl


			Mat thisChunk = Mat(toChunk, rect);   
			chunks.push_back(thisChunk);

			overlayImg(countEdges(thisChunk), rect);

		}
	}
}

void overlayImg(int edges, Rect& region) {
	namedWindow("debug", CV_WINDOW_AUTOSIZE);
	double alpha = double(edges) / 447.0;
	cout << edges << "->" << alpha << endl;
	Mat temp;
	img.copyTo(temp);

	rectangle(temp, region, CV_RGB(255, 0, 0), -1);
	addWeighted(temp, alpha, img, (1 - alpha), 0, heatMap);

	imshow("debug", heatMap);
	waitKey(0);
}

int countEdges(Mat& image) {
	
	int edgeCount = 0;

	Mat_<uchar>::iterator myIterator;
	Mat_<uchar>::iterator end       = image.end<uchar>();
	for (myIterator = image.begin<uchar>(); myIterator != end; myIterator++) {
		if (*myIterator) {
			edgeCount++;
		}
	}
	return edgeCount;
}

void boundedContours(Mat& contourImg, Mat& output) {

	//adaptiveThreshold(contourImg, contourImg, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 0);
	Mat blurred;
	GaussianBlur(contourImg, blurred, Size(3, 3), 0, 0);
	threshold(blurred, blurred, 0, 255, THRESH_BINARY + THRESH_OTSU);

	int kernelDim = 2;
	Mat kernel = getStructuringElement(0, Size(kernelDim,kernelDim), Point(kernelDim/2,kernelDim/2));
	morphologyEx(blurred, blurred, MORPH_OPEN, kernel);
	morphologyEx(blurred, blurred, MORPH_CLOSE, kernel);

	namedWindow("binary", CV_WINDOW_AUTOSIZE);
	imshow("binary", blurred);
	imwrite("../data/binary.jpg", blurred);
	
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(blurred, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE);

	//vector<Rect> boundaries(contours.size());
	contourDraw(output, contours, hierarchy, 0);
}

void contourDraw(Mat& img, vector<vector<Point>> contours, vector<Vec4i> hierarchy, int startPoint) {

	const int HIERARCHY_NEXT = 0;
	const int HIERARCHY_PREV = 1;
	const int HIERARCHY_CHILD = 2;
	const int HIERARCHY_PARENT = 3;

	for (int i = startPoint; i >= 0; i = hierarchy[i][HIERARCHY_NEXT]) {
		int r = rand() % 255;
		int g = rand() % 255;
		int b = rand() % 255;
		//drawContours(img, contours, i, CV_RGB(r, g, b));
		Rect bounder = boundingRect(contours[i]);
		rectangle(img, bounder.tl(), bounder.br(), CV_RGB(r, g, b), 2, 8, 0);
		for (int j = hierarchy[i][HIERARCHY_CHILD]; j >= 0; j = hierarchy[j][HIERARCHY_NEXT]) {
			//boundaries[j] = boundingRect(contours[j]);
			//rectangle(output, boundaries[j].tl(), boundaries[j].br(), CV_RGB(r, g, b), 2, 8, 0);
			contourDraw(img, contours, hierarchy, hierarchy[j][HIERARCHY_CHILD]);
		}
	}

}

/*
try this with if rect1.br - (rect2.tl+5) > 0 then merge

if((rect1 & rect2) == rect1) ... // rect1 is completely inside rect2; do nothing.
else if((rect1 & rect2).area() > 0) // they intersect; merge them.
newrect = rect1 | rect2;
... // remove rect1 and rect2 from list and insert newrect.
*/



//![morphology_operations]
/**
* @function Morphology_Operations
*/
/*void Morphology_Operations(int, void*)
{
// Since MORPH_X : 2,3,4,5 and 6
//![operation]
int operation = morph_operator + 2;
//![operation]

Mat element = getStructuringElement(morph_elem, Size(2 * morph_size + 1, 2 * morph_size + 1), Point(morph_size, morph_size));

/// Apply the specified morphology operation
morphologyEx(src, dst, operation, element);
imshow(window_name, dst);
}*/
//![morphology_operations]


Mat KMeans_Clustering(Mat& target)
{


		//Store the image pixels as an array of samples
		Mat samples(target.rows*target.cols, 3, CV_32F);
		float* sample = samples.ptr<float>(0);
		for (int row = 0; row < target.rows; row++) {
			for (int column = 0; column < target.cols; column++) {
				for (int channel = 0; channel < 3; channel++) {
				samples.at<float>(row*target.cols + column, channel) = (uchar)target.at<Vec3b>(row, column)[channel];
				}
			}
		}
		//Apply kmeans clustering, determining the cluster centres and a label for each pixel
		Mat labels, centres;
		kmeans(samples, 5, labels, TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 100, 0.0001), 10, KMEANS_PP_CENTERS, centres);
		//use centers and labels to populate destination image
		Mat dest = Mat(target.size(), target.type());
		for (int row = 0; row < target.rows; row++) {
			for (int col = 0; col < target.cols; col++) {
				for (int channel = 0; channel < 3;channel++) {
					dest.at<Vec3b>(row, col)[channel] = (uchar)centres.at<float>(*(labels.ptr<int>(row*target.cols + col)), channel);
				}
			}
		}
}