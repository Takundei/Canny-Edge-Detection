#ifndef CANNYEDGE_H
#define CANNYEDGE_H

#include <QCloseEvent>
#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <iostream>
#include "opencv2/opencv.hpp"
#include <cstdlib>

namespace Ui {
class CannyEdge;
}

class CannyEdge : public QMainWindow
{
    Q_OBJECT

public:
    explicit CannyEdge(QWidget *parent = nullptr);
    ~CannyEdge();

private slots:
    void on_Save_clicked();

    void on_Open_clicked();

    void on_Convert_clicked();

    void on_ThreshLow_valueChanged(int arg1);

    void on_ThreshHigh_valueChanged(int arg1);

    void on_Abort_clicked();

    void on_inputVid_textEdited(const QString &arg1);

    void on_outputVid_textEdited(const QString &arg1);

    void on_Preview_clicked();

protected:
    void closeEvent(QCloseEvent *event);
private:
    Ui::CannyEdge *ui;
    /*Threshhold values*/
    int LOW;
    int HIGH;
    /*Abort botton presed*/
    bool abort;
    /* Show Preview*/
    bool preview;
    /*IO filenames */
    QString InputVideo = " ";
    QString OutputVideo = " ";
    /*Display preview video */
    QGraphicsPixmapItem originalVideo;
    QGraphicsPixmapItem editedVideo;
    /*The Algorithm */
    int CannyEdgeDetection();
    /*Curent working Directory */
    QString path;
};

#endif // CANNYEDGE_H
