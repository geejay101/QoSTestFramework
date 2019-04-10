#include <iostream> // for standard I/O
#include <string>   // for strings
#include <iomanip>  // for controlling float print precision
#include <sstream>  // string to number conversion
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv2/imgcodecs.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/ml.hpp>
#include <opencv2/dnn.hpp>

using namespace cv;
using namespace cv::ml;
using namespace cv::dnn;
using namespace std;

#include <unistd.h>
#include <stdio.h>

#define ND 4
#define INTERVAL 3000.0/30.0
int tagsize= 60;
int framenumber = 600;
void getMaxClass(const Mat &probBlob, int *classId, double *classProb);
int test_on_single_photo_dl(Mat img);

void help()
{
    cout << endl;
    cout << "/////////////////////////////////////////////////////////////////////////////////" << endl;
    cout << "This program measures jitter, the time gap between two consecutive saved frames" << endl;
    cout << "If you want to run it in terminal instead of basicServer, please cd to mcu-bench_cpp folder and use ./native/xxx" << endl;
    cout << "For example: ./native/flr ./native/Data/localLatency.txt" << endl;
    cout << "This program will read the tag photos from C++ and generate a new file call rec_timestamp.txt " << endl;
    cout << "/////////////////////////////////////////////////////////////////////////////////" << endl << endl;
}

vector<pair<unsigned long long, unsigned int> > datas;

int main(int argc, char *argv[])
{
    string rec_timestamp = "../dataset/Data/rec_timestamp.txt";
    ofstream receive_timestamp(rec_timestamp.c_str());
    ifstream received_video(argv[1]);
    ofstream jitter_out("../dataset/output/jitter.txt");
    receive_timestamp<<",";

    int height = tagsize;
    int width = tagsize*ND;
    int isFirstData = 0;
    int overFlag = 0;
    int v(0);
    unsigned int r, g, b;
    long t(0);//get timestamp from file "mixRawFile"
    unsigned int a1, r1, g1, b1;//get ARGB from file "mixRawFile"
    char c;

    for(int f = 0;;f++)
    {

        if(overFlag == 1)//the whole file is over
        {
            overFlag = 0;
            //cout<<"over!"<<endl;
            break;
        }
        if(overFlag == 2)//one frame over
        {
            overFlag = 0;
        }
        Mat image(height, width, CV_8UC3);

        if (isFirstData == 0)
        {
            isFirstData = 1;
            received_video >> c;//get first ','
        }
        received_video >> t;//get a frame's timestamp
        received_video >> c;

        for(int i = 0;i < height;i++)
        {
            for(int j = 0;j < width;j++)
            {
                received_video >> v;
                b1 = v;
                received_video >> c;
                received_video >> v;
                g1 = v;
                received_video >> c;
                received_video >> v;
                r1 = v;
                received_video >> c;
                received_video >> v;
                a1 = v;
                received_video >> c;
                //cout << a1 << " " << r1 << " " << g1 << " " << b1 << " " << endl;

                image.at<Vec3b>(i, j)[0] = b1;
                image.at<Vec3b>(i, j)[1] = g1;
                image.at<Vec3b>(i, j)[2] = r1;

                if ((c == 'f'))
                {
                    for (int i = 0; i < 4; ++i)
                    {
                        received_video >> c;
                    }
                    received_video >> c;//get next char,such as ',' or 'e'.
                    if (c == ' ')
                    {
                        //cout<<"occ2"<<endl;
                        overFlag = 1;//file over
                        break;
                    }
                    if (received_video.eof())//the situation:the file over at 'frame'
                    {
                        //cout<<"occ3"<<endl;
                        overFlag = 1;//file over
                        break;
                    }
                    else
                    {
                        //cout<<"occ4"<<endl;
                        overFlag = 2;//one frame over
                        break;
                    }
                }
                if (received_video.eof())//the situation:the file over at ' '
                {
                    //cout<<"occ5"<<endl;
                    overFlag = 1;//file over
                    break;
                }
            }
            if (overFlag >= 1)
            {
                //cout<<"occ6"<<endl;
                break;//break to do iq task when a frame over or the whole file over.(maybe a img didn't trans completely,so must make a force break.)
            }
        }

        int framenum2(0);
        Mat img2;
        for(int k = 0;k < ND;k++)
        {
            img2 = image(Rect(k*tagsize, 0, tagsize, tagsize));
            img2 = 255 - img2 ;
            int num = test_on_single_photo_dl(img2);
            framenum2 = framenum2*10 + num;
        }
        //cout << "num is " << framenum2 << endl;
        receive_timestamp<<framenum2<<","<<t<<",";//save timestamp to file
        if(received_video.eof())
        {
            break;
        }
    }

    receive_timestamp.close();
    ifstream received_tag(rec_timestamp.c_str());

    unsigned long long int tt = 0;
    int tag(0);
    int framenum(0);
//////////////////////////////////////////////////////////Load data ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//for c++
    unsigned long long lastt = 0;
    int lastf = -1;
    int roomsize=1;//default
    received_tag>>c;//','
    for(;;)
    {
        received_tag >> tag;
        received_tag >> c;
        received_tag >> tt;
        received_tag >> c;
        framenum = tag;

        lastt = tt;
        if(lastf != framenum)
        {
            datas.push_back(make_pair(tt, framenum));
            lastf = framenum;
        }
        if(received_tag.eof())
            break;
    }

    // for(int i = 1;i < datas.size();i++)
    // {
    //  cout << datas[i].first << " " << datas[i].second << endl;
    // }
    //sleep(100);
    //cout << "---------------------------------------------------------------------------------------" << endl;
    //cout << "---------------------------------------jitter------------------------------------------" << endl;
    //cout << "---------------------------------------------------------------------------------------" << endl;
    //ofstream ofile2("./native/output/jitter2.data");
    for(int i = 2;i < datas.size();i++)
    {
        unsigned long long int temp = datas[i].first - datas[i-1].first;
        int inter = datas[i].second - datas[i-1].second;
        //if (temp < 2000)
    //  if (temp < 10000)
    //  {



       if (inter < 0)
        {
           //cout << "---<0--inter--" << (inter + framenumber) << endl;
           cout << abs((datas[i].first - datas[i-1].first)/(inter + framenumber)) << endl;//what's INTERVAL?
           jitter_out << abs((datas[i].first - datas[i-1].first)/(inter + framenumber));//what's INTERVAL?
        }
        else{
          
          cout << abs((datas[i].first - datas[i-1].first)/(inter)) << endl;//what's INTERVAL?
          jitter_out << abs((datas[i].first - datas[i-1].first)/(inter));//what's INTERVAL?
          }
            
  //          cout << abs(datas[i].first - datas[i-1].first - INTERVAL) << endl;//what's INTERVAL?
    //  }
            //jitter_out << abs(datas[i].first - datas[i-1].first - INTERVAL);
            jitter_out << ",";
    //  }
    }

    return 0;

}

void getMaxClass(const Mat &probBlob, int *classId, double *classProb)
{
    Mat probMat = probBlob.reshape(1, 1); //reshape the blob to 1x1000 matrix
    Point classNumber;
    minMaxLoc(probMat, NULL, classProb, NULL, &classNumber);
    *classId = classNumber.x;
}

int test_on_single_photo_dl(Mat img)
{
    cv::dnn::initModule();  //Required if OpenCV is built as static libs

    String modelTxt = "./ml/deploy.prototxt";
    String modelBin = "./ml/lenet_iter_10000.caffemodel";
    Net net = dnn::readNetFromCaffe(modelTxt, modelBin);
    if (net.empty())
    {
        std::cerr << "Can't load network by using the following files: " << std::endl;
        std::cerr << "prototxt:   " << modelTxt << std::endl;
        std::cerr << "caffemodel: " << modelBin << std::endl;
        exit(-1);
    }
    if (img.empty())
    {
        std::cerr << "Can't read the image " << std::endl;
        exit(-1);
    }
    resize(img, img, Size(28, 28));        //GoogLeNet accepts only 224x224 RGB-images
    //Convert Mat to batch of images
    Mat inputBlob = blobFromImage(img);
    net.setBlob(".data", inputBlob);        //set the network input
    net.forward();                          //compute output
    Mat prob = net.getBlob("prob");   //gather output of "prob" layer
    int classId;
    double classProb;
    getMaxClass(prob, &classId, &classProb);//find the best class
    //std::vector<String> classNames = readClassNames();
   // std::cout << "result by dl: #" << classId << std::endl;
  //  std::cout << "Probability: " << classProb * 100 << "%" << std::endl;
    return classId;
}
