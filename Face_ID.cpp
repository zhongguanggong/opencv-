//人脸识别
//1.采集1张人脸照片
//2.照片处理
//3.读取训练文件
//4.人脸比对
//头文件
//2018.06.04

#include "stdafx.h"
#include<opencv2/opencv.hpp>
#include<opencv2/face/facerec.hpp>
#include<opencv2/core.hpp>
#include<opencv2/face.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

using namespace cv;
using namespace cv::face;
using namespace std;

int retVal;

int main()
{
	CascadeClassifier cascade;
	CascadeClassifier face_cascade;
	CascadeClassifier smile_cascade;
	std::vector<Rect> smile;
	cascade.load("lbpcascade_frontalface.xml");
	face_cascade.load("haarcascade_frontalface_default.xml");
	smile_cascade.load("haarcascade_smile.xml");
	VideoCapture cap;
	//打开摄像头
	cap.open(0);
	Mat frame;
	Mat myFace;
	int pic_num = 1;
    //拍照
	while (pic_num < 2)
	{
		cap >> frame;
		std::vector<Rect> faces;
		Mat frame_gray;
		//对图片进行直方图均衡化
		cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
    //检测人脸
		cascade.detectMultiScale(frame_gray, faces, 1.1, 2, 0, Size(200, 200), Size(800, 800));

		for (size_t i = 0; i < faces.size(); i++)
		{
			//框出人脸
			rectangle(frame, faces[i], Scalar(255, 0, 0), 2, 8, 0);
		}

		if (faces.size() == 1)
		{
			Mat faceROI = frame_gray(faces[0]);
			smile_cascade.detectMultiScale(faceROI, smile, 1.1, 55, CASCADE_SCALE_IMAGE);
            //特征脸处理
			resize(faceROI, myFace, Size(92, 112));
			putText(frame, to_string(pic_num), faces[0].tl(), 3, 1.2, (0, 0, 255), 2, LINE_AA);
			pic_num++;
			string window_name = "my face";
			namedWindow(window_name, CV_WINDOW_AUTOSIZE);
			imshow("请摘下眼镜", frame);
			waitKey(10);
		}
	}
	waitKey(100);
	destroyAllWindows();

	Mat testSample = myFace;

	/*Ptr<FaceRecognizer> model = EigenFaceRecognizer::create();
	model->train(images, labels);
	model->save("MyFacePCAModel.xml");*/

	//读取已经训练好的文件
	Ptr<FaceRecognizer> model1 = FisherFaceRecognizer::create();
	model1->read("MyFaceFisherModel.xml");

	/*Ptr<FaceRecognizer> model2 = LBPHFaceRecognizer::create();
	model2->train(images, labels);
	model2->save("MyFaceLBPHModel.xml");*/

	// 还有一种调用方式，可以获取结果同时得到阈值:
	//confidence为置信度，越小越可信
	int predictedLabel1 = -1;
	double confidence = 0.0;
	model1->predict(testSample, predictedLabel1, confidence);

	//设置置信度边界，高于此值则不能判断为人脸
	if (confidence < 1000)
	{
		cout << "检测到您是" << predictedLabel1 << "号" << endl;
		cout << "门已开，请进！" << endl;
	}
	//else cout << "未检测到您的信息\n置信度为" << confidence <<endl;
	else {
		cout << "对不起，您非本公司职员！" << endl;
	}
	if (smile.size() > 2) cout << "检测到您在微笑" << endl;
	
  return 0;
}

