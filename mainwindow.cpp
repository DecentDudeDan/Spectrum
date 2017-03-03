#include "mainwindow.h"
#include "ui_mainwindow.h"

ConcurrentQueue* points = new ConcurrentQueue();

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    CF(2.5),
    AB(60),
    numPoints(512),
    inSetup(true)
{

    dataTimer = new QTimer();
    ui->setupUi(this);

    setupGraph();

    // setup a timer that repeatedly calls MainWindow::realtimeDataSlot when the timer times out:
    connect(dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
    connect(ui->StopButton, SIGNAL(clicked()), dataTimer, SLOT(stop()));
    //setup user inputs dropdown values
    ui->FFT1->addItem("256", QVariant(256));
    ui->FFT1->addItem("512", QVariant(512));
    ui->FFT1->addItem("1024", QVariant(1024));
    ui->FFT1->addItem("2048", QVariant(2048));
    ui->FFT1->addItem("4096", QVariant(4096));
    ui->FFT1->addItem("8192", QVariant(8192));
    ui->FFT1->addItem("16384", QVariant(16384));
    ui->FFT1->addItem("32768", QVariant(32768));
    ui->FFT1->addItem("65536", QVariant(65536));
    ui->WSize->addItem("Rectangular");
    ui->WSize->addItem("Blackman's");
    ui->WSize->addItem("Blacktop");
    ui->WSize->addItem("Hanning");
    ui->WSize->addItem("Hamming");
    
    inSetup = false;


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupGraph()
{

    ui->customPlot1->setBackground(Qt::lightGray);
    ui->customPlot1->axisRect()->setBackground(Qt::black);

    ui->FFT->setStyleSheet("background-color: rgba( 255, 255, 255, 0);");
    ui->CF->setStyleSheet("background-color: rgba( 255, 255, 255, 0);");
    ui->AB->setStyleSheet("background-color: rgba( 255, 255, 255, 0);");

    // add a graph to the plot and set it's color to blue:
    ui->customPlot1->addGraph();
    ui->customPlot1->graph(0)->setPen(QPen(QColor(224, 195, 30)));
    ui->customPlot1->graph(0)->setLineStyle((QCPGraph::LineStyle)2);

    // set x axis to be a time ticker and y axis to be from -1.5 to 1.5:
    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    ui->customPlot1->xAxis->setRange(70, 6000);
    ui->customPlot1->axisRect()->setupFullAxesBox();
    ui->customPlot1->yAxis->setRange(1, 3000000);
    ui->customPlot1->yAxis->setScaleType(QCPAxis::stLogarithmic);
    ui->customPlot1->yAxis->setTicker(logTicker);
}

void MainWindow::stopStuff()
{
    dataTimer->stop();
    newThread->setStop(true);
    newThread->exit();
    clearPoints();
    ui->customPlot1->clearGraphs();
}

void MainWindow::startStuff()
{
    setupGraph();
    resetXValues();
    newThread = new libThread(numPoints, AB, CF);
    newThread->start();
    dataTimer->start();
}

void MainWindow::clearPoints()
{
    delete points;
    points = new ConcurrentQueue();
}

void MainWindow::resetXValues()
{
    if(xValue.size() > 0) {
        xValue.clear();
    }
    for(int i = 0; i < numPoints; i++) {
        xValue.push_back(i);
    }
}

void MainWindow::realtimeDataSlot()
{
    static QTime time(QTime::currentTime());
    QVector<double> fftPoints;
    double key;
    static double lastPointKey;

    if(points->size() > numPoints)  {
        fftPoints = createDataPoints(false);
    }

    key = time.elapsed()/1000.0; // set key to the time that has elasped from the start in seconds

    if (key-lastPointKey > 0.006)
    {

        // add data to lines:
        if (xValue.size() == fftPoints.size() && fftPoints.size() == numPoints) {
            ui->customPlot1->graph(0)->setData(xValue, fftPoints);
        }

        // rescale value (vertical) axis to fit the current data:
        //ui->customPlot1->graph(0)->rescaleValueAxis();

        ui->customPlot1->replot();
        lastPointKey = key;
    }

    // calculate frames per second and add it to the ui window at bottom of the screen:
    static double lastFpsKey;
    static int frameCount;
    ++frameCount;
    if (key-lastFpsKey > 2) // average fps over 2 seconds
    {
        ui->statusBar->showMessage(
                    QString("%1 FPS, Total Data points: %2")
                    .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
                    .arg(ui->customPlot1->graph(0)->data()->size())
                    , 0);
        lastFpsKey = key;
        frameCount = 0;
    }

}

QVector<double> MainWindow::createDataPoints(bool isLinear)
{
    int i;
    QVector<double> fftPoints;
    QVector<double> ffttemp1;
    fftw_complex in[numPoints], out[numPoints];
    fftw_plan p;

    p = fftw_plan_dft_1d(numPoints, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    if(points->size() > numPoints) {
        for (i = 0; i < numPoints; i++)
        {
            std::complex<double> current = points->dequeue();
            in[i][0] = current.real();
            in[i][1] = current.imag();
        }
    }

    fftw_execute(p);

    for (i = 0; i < numPoints; i++)
    {
        if (i < numPoints/2)
        {
            double Ppp = (out[i][0]*out[i][0] + out[i][1]*out[i][1])/numPoints;
            double dBFS = 20*log(Ppp);
            isLinear ? ffttemp1.push_back(Ppp) : ffttemp1.push_back(dBFS);
        } else
        {
            double Ppp = (out[i][0]*out[i][0] + out[i][1]*out[i][1])/numPoints;
            double dBFS = 20*log(Ppp);
            isLinear ? fftPoints.push_back(Ppp) : fftPoints.push_back(dBFS);
        }
    }

    fftPoints.append(ffttemp1);

    fftw_destroy_plan(p);

    return fftPoints;

}

void MainWindow::on_FFT1_currentIndexChanged(int index)
{
    if (!inSetup && newThread->isRunning()) {
        stopStuff();
        numPoints = ui->FFT1->itemData(index).toInt();
        startStuff();
    }
}

void MainWindow::on_startButton_clicked()
{
    startStuff();
}

void MainWindow::on_StopButton_clicked()
{
    if (newThread->isRunning()) {
        stopStuff();
    }
}

void MainWindow::on_CF1_editingFinished()
{
    if (newThread->isRunning()) {
        stopStuff();
    }
    CF = ui->CF1->text().toDouble();
    startStuff();
}

void MainWindow::on_AB1_editingFinished()
{
    if (newThread->isRunning()) {
        stopStuff();
    }
    AB = ui->AB1->text().toInt();
    startStuff();
}

