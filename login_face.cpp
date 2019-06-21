//人脸注册
//1.新建保存脸的文件夹sx：x为顺序数
//2.拍摄10张照片到文件夹中
//3.更新at.txt(csv文件)
//4.模型训练
//使用前清空文件夹My_faces里的文件并更新binary.dat
//2018.06.4

#include "stdafx.h"
#include "winsock2.h"
#include <WS2tcpip.h>
#include <windows.h>
#include <opencv2\opencv.hpp>
#include <opencv2\face\facerec.hpp>
#include <opencv2\core.hpp>
#include <opencv2\face.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\imgproc.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <string.h>
#include <direct.h>
#include <Python.h>
#pragma comment(lib, "WS2_32") 

#define max_buf 1024


using namespace std;
using namespace cv;
using namespace cv::face;

int retVal;
char readbuf[max_buf];
SOCKET sHost; //服务器套接字

int socket_init()
{
	WSADATA wsd; //WSADATA变量
	SOCKADDR_IN servAddr; //服务器地址
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		cout << "WSAStartup failed!" << endl;
		return -1;
	}
	//创建套接字
	sHost = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sHost)
	{
		cout << "socket failed!" << endl;
		WSACleanup();//释放套接字资源
		return  -1;
	}
	//设置服务器地址
	servAddr.sin_family = AF_INET;
	//servAddr.sin_addr.s_addr = inet_pton(AF_INET,"192.168.1.1",);
	//PVOID addr;
	//inet_pton(AF_INET, "192.168.1.1", addr);
	//servAddr.sin_addr.S_un.S_addr = (ULONG)addr;
	servAddr.sin_addr.s_addr = inet_addr("192.168.1.1");
	servAddr.sin_port = htons((short)4999);
	//连接服务器
	retVal = connect(sHost, (LPSOCKADDR)&servAddr, sizeof(servAddr));
	if (SOCKET_ERROR == retVal)
	{
		cout << "connect failed!" << endl;
		closesocket(sHost); //关闭套接字
		WSACleanup(); //释放套接字资源
		return -1;
	}
	return 0;
}


//创建新的文件夹用来保存新的人脸
//此函数用来保存下一个人的编号
void DataWrite_CPPMode(char *str_write)
{
	//写出数据
	ofstream f("binary.dat", ios::binary);
	if (!f)
	{
		cout << "创建文件失败" << endl;
		return;
	}
	f.write(str_write, sizeof(char));      //fwrite以char *的方式进行写出，做一个转化
	f.close();
}

char * DataRead_CPPMode()
{
	char *str_read;
	str_read = new char[1];
	ifstream f("binary.dat", ios::binary);
	if (!f)
	{
		cout << "读取文件失败" << endl;
		return str_read;
	}
	f.read(str_read, sizeof(char));
	f.close();
	return str_read;
}

int send_xml()
{
	string read_filename = "MyFaceFisherModel.xml";
	ifstream fd_read(read_filename.c_str());
	if (!fd_read) {
		cout << "打开文件失败！";
		return -1;
	}
	while (fd_read) {
		fd_read.getline(readbuf, max_buf);
		send(sHost, readbuf,sizeof(readbuf), 0);//需要看一下是否自动换行
	}
	char end[1] = {'#'};
	send(sHost, end, 1, 0);//以'#'结束
	return 1;
}

//使用CSV文件去读图像和标签，主要使用stringstream和getline方法
static void read_csv(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';') {
	std::ifstream file(filename.c_str(), ifstream::in);
	if (!file) {
		string error_message = "No valid input file was given, please check the given filename.";
		CV_Error(CV_StsBadArg, error_message);
	}
	string line, path, classlabel;
	while (getline(file, line)) {
		stringstream liness(line);
		getline(liness, path, separator);
		getline(liness, classlabel);
		if (!path.empty() && !classlabel.empty()) {
			images.push_back(imread(path, 0));
			labels.push_back(atoi(classlabel.c_str()));
		}
	}
}

//主函数
int main()
{
	int mkdir_num;
	int socket_sta;
	char *num_read;
	char *num_write;
	string fn_csv = "at.txt";
	string filename = "MyFaceFisherModel.xml";
	vector<Mat> images;
	vector<int> labels;
	CascadeClassifier cascade;
	VideoCapture cap;
	Mat frame;
	int pic_num = 1;

	socket_sta = socket_init();
	if (socket_sta < 0)
	{
		cout << "服务器链接失败！请检查网络连接!" << endl;
		return 0;
	}
	else cout << "服务器链接成功！" << endl;
	cout << "请对准摄像头，摘下眼镜，录入您的10张照片！" << "\n";
	cascade.load("lbpcascade_frontalface.xml");//人脸检测训练模型
	num_read = new char[1];
	num_write = new char[1];
	num_read = DataRead_CPPMode();
	mkdir_num = num_read[0];
	string dirName = format("My_faces\\s%d", mkdir_num);
	//创建新的文件夹用于保存照片
	//mkdir_num为新注册用户的编号
	_mkdir(dirName.c_str());

	cap.open(0);
	//拍摄10张照片
	while (pic_num<11)
	{
		cap >> frame;

		std::vector<Rect> faces;
		Mat frame_gray;
		cvtColor(frame, frame_gray, COLOR_BGR2GRAY);

        //检测人脸范围
		cascade.detectMultiScale(frame_gray, faces, 1.1, 2, 0, Size(200, 200), Size(800, 800));

		//画出人脸范围
		for (size_t i = 0; i < faces.size(); i++)
		{
			rectangle(frame, faces[i], Scalar(255, 0, 0), 2, 8, 0);
		}

		if (faces.size() == 1)
		{
			Mat faceROI = frame_gray(faces[0]);
			Mat myFace;
			//限定照片大小
			resize(faceROI, myFace, Size(92, 112));
			putText(frame, to_string(pic_num), faces[0].tl(), 3, 1.2, (0, 0, 255), 2, LINE_AA);

			string filename = format("My_faces\\s%d\\pic%d.jpg", mkdir_num,pic_num);
			//保存照片
			imwrite(filename, myFace);
			imshow(filename, myFace);
			waitKey(500);
			destroyWindow(filename);
			pic_num++;
		}
		imshow("请摘下眼镜", frame);
		waitKey(10);
	}
	destroyAllWindows();
	mkdir_num++;
	num_write[0] = mkdir_num;
	//更新编号，保存到二进制文件中
	DataWrite_CPPMode(num_write);
	//释放空间
	delete(num_read);
	delete(num_write);
	//运行python脚本，更新csv文件
	system("python add_label2.py");
	try
	{
		read_csv(fn_csv, images, labels);
	}
	catch (cv::Exception& e)
	{
		cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg << endl;
		// 文件有问题，我们啥也做不了了，退出了
		exit(1);
	}
	// 如果没有读取到足够图片，也退出.
	if (images.size() <= 1) {
		string error_message = "This demo needs at least 2 images to work. Please add more images to your data set!";
		CV_Error(CV_StsError, error_message);
	}

	Ptr<FaceRecognizer> model1 = FisherFaceRecognizer::create();
	//样本训练
	model1->train(images, labels);
	model1->save("MyFaceFisherModel.xml");
	string senddata = format("l%d%d#",job_num, mkdir_num - 1);
	retVal = send(sHost, senddata.c_str(), strlen(senddata.c_str()), 0);
	int retVal2 = send_xml();
	if (SOCKET_ERROR == retVal | (retVal2<0))
	{
		cout << "发送失败！" << endl;
		//closesocket(sHost); //关闭套接字
		//WSACleanup(); //释放套接字资源
	}
	else cout << "发送成功！" << endl;
	closesocket(sHost); //关闭套接字
	WSACleanup(); //释放套接字资源
	cout << "注册成功\n您的编号是" << mkdir_num-1 << endl;
	getchar();
	return 0;
}
