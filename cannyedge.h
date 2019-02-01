#ifndef CANNYEDGE_H
#define CANNYEDGE_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
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

private:
    Ui::CannyEdge *ui;
    int LOW;
    int HIGH;
    bool abort;
    QString InputVideo = " ";
    QString OutputVideo = " ";
    int CannyEdgeDetection();
    QString path;
};

#endif // CANNYEDGE_H
