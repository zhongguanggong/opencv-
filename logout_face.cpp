//注销人脸
//1.从dat文件中读取注销人脸编号
//2.删除文件夹下所有文件
//3.删除相应文件夹
//4.更新CSV文件
//5.重新通过CSV文件读入照片
//6.重新训练
//2018.06.04

#include "stdafx.h"
#include "winsock2.h"
#include <WS2tcpip.h>
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
#include <windows.h>
#include <cstdio>

#pragma comment(lib, "WS2_32") 
#define max_buf 1024



using namespace std;
using namespace cv;
using namespace cv::face;

char readbuf[max_buf];
SOCKET sHost; //服务器套接字


int socket_init()
{
	int retVal;
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
	ofstream f("logout_num.dat", ios::binary);
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
	ifstream f("logout_num.dat", ios::binary);
	if (!f)
	{
		cout << "读取文件失败" << endl;
		return str_read;
	}
	f.read(str_read, sizeof(char));
	f.close();
	return str_read;
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
		send(sHost, readbuf, sizeof(readbuf), 0);//需要看一下是否自动换行
	}
	char end[1] = { '#' };
	send(sHost, end, 1, 0);//以'#'结束
	return 1;
}

//主函数
int main()
{
	int rmdir_num;
	unsigned int password;
	int socket_sta;
	string fn_csv = "at.txt";
	vector<Mat> images;
	vector<int> labels;
	socket_sta = socket_init();
	if (socket_sta < 0) {
		cout << "服务器链接失败！" << endl;
		//return -1;
	}
	else cout << "服务器链接成功！" << endl;
	//num_read = new char[1];
	//num_read = DataRead_CPPMode();
	//rmdir_num = num_read[0];
	cout << "请输入密码：";
	cin >> password;
	if (password != 123456) {
		return 0;
	}
	cout << "请输入要删除员工编号：";
	cin >> rmdir_num;
	//删除文件夹下所有文件
	for (int i = 1; i < 11; i++)
	{
		string delete_file = format("My_faces\\s%d\\pic%d.jpg", rmdir_num,i);
		remove(delete_file.c_str());
	}
	//删除文件夹
	string dirName = format("My_faces\\s%d", rmdir_num);
	_rmdir(dirName.c_str());
	//delete(num_read);
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
	int retVal = send_xml();
	if (retVal < 0) {
		cout << "训练文件发送失败！" << endl;
	}
	else {
		cout << "训练文件发送成功！" << endl;
	}
	closesocket(sHost); //关闭套接字
	WSACleanup(); //释放套接字资源
	cout << "注销成功\n编号" <<  rmdir_num  << "已被移除" <<endl;
	getchar();
	return 0;
}
