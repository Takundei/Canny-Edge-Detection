#include <iostream>
#include "opencv2/opencv.hpp"
#include <cstdlib>
#include <cstdio>
#include "unistd.h"
#include "signal.h"

using namespace std;
using namespace cv;

/*determines the neigbors that are chosen :along gradient OR perpendicular*/

enum Mode {non_maximum_suppression, edge_tracking};

/*pixel (i,j) is adjacent to pixels (i + x1 , j + y1) and (i + x2 , j + y2)*/
struct AdjacentPixels {
    char x1,x2,y1,y2;
};

bool Abort = false;

static void at_abort_sigint (int signo) //When user presses ctr-c , save progress and exit
{
    Abort = true;
}

inline void RoundAngle(float &Gy,float Gx, float &angle)
{

    angle = atan2(Gy,Gx) * 57.29577f; //angle converted to DEG
    while(angle < 0) angle += 180.0f;//positive angles

    if(angle <= 22.5f || angle > 157.5f) {
        angle = 0;
    } else if(angle <= 67.5f) {
        angle = 45;
    } else if(angle <= 112.5f) {
        angle = 90;
    } else if(angle <= 157.5f) {
        angle = 135;
    }
}

inline void Adjacent(float &angle,AdjacentPixels &A,Mode mode = non_maximum_suppression )
{
    /*	finds pixels along the same gradient:
                -non-maximum suppression - compare pixels along the gradient
                -Double Thresholding - compare pixels perpendicular to the gradient

    */
    if(mode == edge_tracking) {
        angle = angle < 90 ? angle + 90:angle -90; //rotate by 90 DEG
    }
    switch(static_cast<int>(angle)) {
    case 0: //Gradient = E->W
        A.x1 = -1;
        A.x2 = 1;
        A.y1 = 0;
        A.y2 = 0;
        break;
    case 135://Gradient = NE / SW
        A.x1 = 1;
        A.x2 =-1;
        A.y1 = -1;
        A.y2 = 1;
        break;
    case 90: //Gradient = N / S
        A.x1 = 0;
        A.x2 = 0;
        A.y1 = -1;
        A.y2 = 1;
        break;
    case 45://Gradient = NW / SE
        A.x1 = -1;
        A.x2 = 1;
        A.y1 = -1;
        A.y2 = 1;
        break;
    }

}

int main(int argc, char** argv)
{
    /*IO Files*/
    char * inFile = NULL;
    char * outFile = NULL;
    /*Threshhold values*/
    int LOW = 30;
    int HIGH = 90;
    
    if (signal (SIGINT, at_abort_sigint) == SIG_ERR) {
        cerr<<"Cannot handle SIGINT!"<<endl;
        exit (EXIT_FAILURE);
    }
    int opt;
    while((opt = getopt(argc,argv,"i:o:l:h:")) != -1) {
        switch(opt) {
        case 'i'://input filename
            inFile = optarg;
            break;
        case 'o'://output filename
            outFile = optarg;
            break;
        case 'l'://threshold low
            LOW = atoi(optarg);
            break;
        case 'h'://threshold high
            HIGH = atoi(optarg);
            break;
        default://Invalid option
            cerr<<"Usage: "<<argv[0]<<" -i <input video> -o <output video> [-l <threshold low>] [-h <threshold high>] \n";
            exit(EXIT_FAILURE);

        }
    }
    if(!inFile || !outFile){
        cerr<<"Usage: "<<argv[0]<<" -i <input video> -o <output video> [-l <threshold low>] [-h <threshold high>] \n";
        exit(EXIT_FAILURE);

    }

    VideoCapture video(inFile);
    if(!video.isOpened()) {
        cerr<<"Error opening video";
        exit(EXIT_FAILURE);
    }

    /*get width, height and fps*/
    int width = static_cast<int>(video.get(CV_CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(video.get(CV_CAP_PROP_FRAME_HEIGHT));
    int framecount = static_cast<int>(video.get(CV_CAP_PROP_FRAME_COUNT));
    int framenum = 0;
    double FPS = video.get(CV_CAP_PROP_FPS);

    VideoWriter output(outFile,CV_FOURCC('M','J','P','G'),FPS, Size(width,height));
    /*

    1. Extract frame.
    2. Convert frame to Grayscale.
    3. Apply smoothing to frame.
    4. Get Gradient's Magnitude at each pixel in the frame.
    5. Find all pixels that are local maximum.
    6. Apply thesholding to mark Strong pixels(in final video) and Weak pixels (to be considered for inclusion)
    7. If a weak pixel is adjacent to - along the edge of - a strong pixel ; mark it as strong. Repeat until no more pixels are marked to strong;
    9. Remove all weak pixels

    */
    cout << "Converting ..."<<endl;
    for(;!Abort;) {
        Mat frame;
        Mat Gradient,Gx,Gy;
        video.read(frame);
        framenum++;
        if(frame.empty()) { //end of video
            break;
        }
        cout<<"\rProgress: "<<framenum*100/ framecount << " %";
        cout.flush();
        //2
        cvtColor(frame,frame,COLOR_BGR2GRAY);
        //3
        GaussianBlur(frame, frame, cv::Size(5, 5), 1.4);
        //4
        Sobel(frame,Gx,CV_32F,1,0);
        Sobel(frame,Gy,CV_32F,0,1);

        magnitude(Gx,Gy,Gradient);

        Mat Theta = Mat(frame.rows,frame.cols,CV_32F); //stores the rounded angle (direction of gradient)
        Mat Final = Mat(Gradient.rows,Gradient.cols,CV_8U,Scalar(0)); //final frame

        MatIterator_<float> itG = Gradient.begin<float>();
        MatIterator_<float> itend = Gradient.end<float>();
        MatIterator_<float> itGx = Gx.begin<float>();
        MatIterator_<float> itGy = Gy.begin<float>();

        MatIterator_<float>itT = Theta.begin<float>();
        MatIterator_<unsigned char>itF = Final.begin<unsigned char>();

        AdjacentPixels Apx = {0,0,0,0};

        for(; itG != itend; itG++, itF++, itT++, itGy++,itGx++) {
            const Point px = itG.pos(); //current px
            if(px.x < 2 || px.y < 2 || px.x > frame.cols -2 || px.y > frame.rows-2) {
                continue; //boundary pixels ignored
            }
            if(*itG < LOW) {
                continue;   //already lower than threshold ->ignore
            }

            RoundAngle(*itGy,*itGx,*itT);
            Adjacent(*itT,Apx);
            //5
            if(*itG > Gradient.at<float>(px.y + Apx.y1,px.x + Apx.x1) && *itG > Gradient.at<float>(px.y + Apx.y2,px.x + Apx.x2)) {
                //6
                if(*itG >= HIGH) {
                    *itF = 255; //Marked as a Strong pixel
                } else {
                    *itF  = 64;	//Weak pixel
                }
            }

        }
        //7
        unsigned char* px,*p1,*p2;
        float *T;
        bool changed = true;//indicates that new strong pixels have been added
        while(changed) {
            changed = false;
            for(int row = 1; row < Final.rows -1; ++row) {
                for(int col = 1; col < Final.cols-1; ++col) {

                    if(px[col] == 64) {
                        px = Final.ptr<unsigned char>(row);
                        T = Theta.ptr<float>(row);
                        Adjacent(*T,Apx,edge_tracking);
                        p1 = Final.ptr<unsigned char>(row + Apx.y1);
                        p2 = Final.ptr<unsigned char>(row + Apx.y2);
                        if(p1[col + Apx.x1] == 255 || p2[col + Apx.x2] == 255) {
                            px[col] = 255;
                            changed = true;

                        }
                    }
                }
            }
        }
        //8
        threshold(Final,Final,200,255,THRESH_BINARY);
        //Store frame to final video
        cvtColor(Final,frame,COLOR_GRAY2BGR);
        output.write(frame);

        if (waitKey(30) == 27) break; //press ESC to abort
    }
    cout<<"\nDone\n";
    video.release();
    output.release();
    signal (SIGINT, SIG_DFL);
    destroyAllWindows();
    return EXIT_SUCCESS;

}
