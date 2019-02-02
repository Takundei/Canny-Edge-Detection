#include "cannyedge.h"
#include "ui_cannyedge.h"

using namespace std;
using namespace cv;

CannyEdge::CannyEdge(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CannyEdge)
{
    ui->setupUi(this);
    HIGH = 60;
    LOW = 20;
    path = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    abort = true;
    //initialise QGraphicsview
    preview = true;
    ui->previewOriginal->setScene(new QGraphicsScene(this));
    ui->previewOriginal->scene()->addItem(&originalVideo);
    ui->previewOriginal->fitInView(&originalVideo, Qt::KeepAspectRatio);
    ui->previewEdited->fitInView(&editedVideo, Qt::KeepAspectRatio);
    ui->previewEdited->setScene(new QGraphicsScene(this));
    ui->previewEdited->scene()->addItem(&editedVideo);
    ui->previewOriginal->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->previewOriginal->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //set threshhold
    ui->ThreshLow->setValue(LOW);
    ui->ThreshLow->setMaximum(HIGH);
    ui->ThreshHigh->setValue(HIGH);
    ui->progressBar->setMinimum(0);
    //hide features
  ui->progressBar->setEnabled(false);
   ui->Abort->setEnabled(false);
}

CannyEdge::~CannyEdge()
{
    delete ui;
}

void CannyEdge::closeEvent(QCloseEvent *event){
    if(!abort){
        QMessageBox::warning(this,"Warning","Please Click \"Abort\" before closing");
        event->ignore();
    }else{
        event->accept();
    }
}

void CannyEdge::on_Save_clicked()
{
    OutputVideo = QFileDialog::getSaveFileName(this, "Save As",path);
    if (!OutputVideo.endsWith(".avi"))
        OutputVideo += ".avi";
    ui->outputVid->setText(OutputVideo);

}

void CannyEdge::on_Open_clicked()
{
    InputVideo = QFileDialog::getOpenFileName(this, "Save As",path,tr("Videos (*.MOV *.avi *.mp4);; All Files(*.*)"));
    ui->inputVid->setText(InputVideo);
    path = QFileInfo(InputVideo).path();
    OutputVideo =path+"/"+ QFileInfo(InputVideo).baseName() + ".avi";
    ui->outputVid->setText(OutputVideo);
    ui->progressBar->setValue(0);
}

void CannyEdge::on_Convert_clicked()
{
    ui->Open->setEnabled(false);
    ui->inputVid->setEnabled(false);
    ui->outputVid->setEnabled(false);
    ui->Save->setEnabled(false);
    ui->ThreshLow->setEnabled(false);
    ui->ThreshHigh->setEnabled(false);
    ui->progressBar->setEnabled(true);
    ui->Convert->setEnabled(false);
    ui->Abort->setEnabled(true);
    ui->About->setVisible(false);
       int ret = 0;
    abort = false;
    waitKey(30);
    ret = CannyEdgeDetection();
    if(ret == 1){
        QMessageBox::warning(this,"Warning","Error opening video");
    }
    abort = true;
    ui->Convert->setEnabled(true);
    ui->Abort->setEnabled(false);
    ui->Open->setEnabled(true);
    ui->inputVid->setEnabled(true);
    ui->outputVid->setEnabled(true);
    ui->Save->setEnabled(true);
    ui->ThreshLow->setEnabled(true);
    ui->ThreshHigh->setEnabled(true);
    ui->progressBar->setEnabled(false);
    ui->Abort->setEnabled(false);

}

void CannyEdge::on_Abort_clicked()
{
    abort = true;

}

void CannyEdge::on_ThreshLow_valueChanged(int arg1)
{
    LOW = arg1;

}

void CannyEdge::on_ThreshHigh_valueChanged(int arg1)
{
    HIGH = arg1;
    ui->ThreshLow->setMaximum(HIGH);
}

enum Mode {non_maximum_suppression , edge_tracking}; //determines the neigbors that are chosen :along gradient ; perpendicular

struct AdjacentPixels{
    char x1,x2,y1,y2;
};

inline void RoundAngle(float &Gy,float Gx, float &angle){

    angle = atan2(Gy,Gx) * 57.29577f; //angle converted to DEG
    while(angle < 0) angle += 180.0f;

    if(angle <= 22.5f || angle > 157.5f){
        angle = 0;
    }else if(angle <= 67.5f){
        angle = 45;
    }else if(angle <= 112.5f){
        angle = 90;
    }else if(angle <= 157.5f){
        angle = 135;
    }

}

inline void Adjacent(float &angle,AdjacentPixels &A,Mode mode = non_maximum_suppression ){

    if(mode == edge_tracking){
        angle = angle < 90 ? angle + 90:angle -90;
    }
    switch(static_cast<int>(angle)){
    case 0: //0 x G = E->W
        A.x1 = -1;
        A.x2 = 1;
        A.y1 = 0;
        A.y2 = 0;
        break;
    case 135://135 G = NE / SW
        A.x1 = 1;
        A.x2 =-1;
        A.y1 = -1;
        A.y2 = 1;
        break;
    case 90: //90  G = N / S
        A.x1 = 0;
        A.x2 = 0;
        A.y1 = -1;
        A.y2 = 1;
        break;
    case 45://45 G = NW / SE
        A.x1 = -1;
        A.x2 = 1;
        A.y1 = -1;
        A.y2 = 1;
        break;
    }


}

int CannyEdge::CannyEdgeDetection(){
    VideoCapture video(InputVideo.toUtf8().constData());
    if(!video.isOpened()){
        cerr<<"Error opening video";
        return 1;
    }

    //get width, height and fps
    int width = static_cast<int>(video.get(CV_CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(video.get(CV_CAP_PROP_FRAME_HEIGHT));
    int framecount = static_cast<int>(video.get(CV_CAP_PROP_FRAME_COUNT));
    double FPS = video.get(CV_CAP_PROP_FPS);

    ui->progressBar->setMaximum(framecount);
    framecount = 0;
    VideoWriter output(OutputVideo.toUtf8().constData(),CV_FOURCC('X','2','6','4'),FPS, Size(width,height));

    for(;!abort;){
        QApplication::processEvents();
        Mat frame;
        Mat Gradient,Gx,Gy;
        video.read(frame);

        if(frame.empty()){
            break;
        }

        framecount ++;
        ui->progressBar->setValue(framecount);
        //convert original to Qimage
        QImage QImageOrig;
        if(preview){
            height=ui->previewOriginal->height();
            width=ui->previewOriginal->width();
            QImageOrig = QImage(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
            originalVideo.setPixmap( QPixmap::fromImage(QImageOrig.rgbSwapped()).scaled(width,height,Qt::KeepAspectRatio));
        }

        cvtColor(frame,frame,COLOR_BGR2GRAY);

        GaussianBlur(frame, frame, cv::Size(5, 5), 1.4);
        Sobel(frame,Gx,CV_32F,1,0);
        Sobel(frame,Gy,CV_32F,0,1);

        magnitude(Gx,Gy,Gradient);

        Mat Theta = Mat(frame.rows,frame.cols,CV_32F);
        Mat Final = Mat(Gradient.rows,Gradient.cols,CV_8U,Scalar(0));

        MatIterator_<float> itG = Gradient.begin<float>();
        MatIterator_<float> itend = Gradient.end<float>();
        MatIterator_<float> itGx = Gx.begin<float>();
        MatIterator_<float> itGy = Gy.begin<float>();

        MatIterator_<float>itT = Theta.begin<float>();
        MatIterator_<unsigned char>itF = Final.begin<unsigned char>();

        AdjacentPixels Apx = {0,0,0,0};
        for(;itG != itend; itG++, itF++, itT++, itGy++,itGx++){
            const Point px = itG.pos(); //current px
            if(px.x < 2 || px.y < 2 || px.x > frame.cols -2 || px.y > frame.rows-2){continue;}
            if(*itG < LOW){continue;} //already lower than threshold ->ignore

            RoundAngle(*itGy,*itGx,*itT);
            Adjacent(*itT,Apx);

            if(*itG > Gradient.at<float>(px.y + Apx.y1,px.x + Apx.x1) && *itG > Gradient.at<float>(px.y + Apx.y2,px.x + Apx.x2)){
                if(*itG >= HIGH){
                    *itF = 255; //include in final
                }else{
                    *itF  = 64;
                }
            }

        }

        unsigned char* px,*p1,*p2;
        float *T;
        bool changed = true;
        while(changed){
            changed = false;
            for(int row = 1; row < Final.rows -1; ++row) {
                for(int col = 1; col < Final.cols-1; ++col) {
                    px = Final.ptr<unsigned char>(row);
                    if(px[col] == 64){
                        T = Theta.ptr<float>(row);
                        Adjacent(*T,Apx,edge_tracking);
                        p1 = Final.ptr<unsigned char>(row + Apx.y1);
                        p2 = Final.ptr<unsigned char>(row + Apx.y2);
                        if(p1[col + Apx.x1] == 255 || p2[col + Apx.x2] == 255){
                            px[col] = 255;
                            changed = true;

                        }
                    }
                }
            }
        }
        threshold(Final,Final,200,255,THRESH_BINARY);

        cvtColor(Final,frame,COLOR_GRAY2BGR);
        output.write(frame);

        if(preview){
            height=ui->previewEdited->height();
            width=ui->previewEdited->width();
            QImage QImageEdited(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
            editedVideo.setPixmap( QPixmap::fromImage(QImageEdited.rgbSwapped()).scaled(width,height,Qt::KeepAspectRatio) );

        }
    }

    video.release();
    output.release();
    destroyAllWindows();
    return 0;

}



void CannyEdge::on_inputVid_textEdited(const QString &arg1)
{
    InputVideo = arg1;
}

void CannyEdge::on_outputVid_textEdited(const QString &arg1)
{
    OutputVideo = arg1;
}

void CannyEdge::on_Preview_clicked()
{
    preview = !preview;
    if(preview){
        ui->Preview->setText("ON");
    }else{
        ui->Preview->setText("OFF");
    }
}
