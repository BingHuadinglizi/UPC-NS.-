#include "QtSGS.h"
#include <QMessageBox>
#include <QDebug>
#include <ctime>




int StartNodeNum = -1, EndNodeNum = -1;//起点终点结点编号


QtSGS::QtSGS(QWidget* parent)
    : QWidget(parent), ui(new Ui::QtSGSClass)
{
    ui->setupUi(this);
    B_Img = QImage("./Resources/UPCMap.jpg");
    BRec = QRect(0, 0,1436, 1000);
    NowArea = true;
    Simulation = false;
    ShowScanRoad = false;
    CurrentRoadNum = 0;
    day = 1;
    hour = 6;
    minute = 0;
    second = 0;
    StuX = -50;
    StuY = -50;
    StuSpeed = 1.2;
    FinalRoadCount = 0;
    ScanRoadCount = 0;
    ui->changeTimeF->setEnabled(true);
    ui->changeTimeS->setEnabled(false);
    ui->StopSearch->setEnabled(false);
    ui->ScanService->setEnabled(true);

    connect(ui->changeTimeF, SIGNAL(clicked()), this, SLOT(TimerFaster()));
    connect(ui->changeTimeS, SIGNAL(clicked()), this, SLOT(TimerSlower()));
    connect(ui->StartSearch, SIGNAL(clicked()), this, SLOT(clickStartSearch()));
    connect(ui->ScanService, SIGNAL(clicked()), this, SLOT(clickScanService()));
    connect(ui->StartBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeStartNodeNum()));
    connect(ui->EndBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeEndNodeNum()));
    connect(ui->StopSearch, SIGNAL(clicked()), this, SLOT(clickStopSearchButton()));
    TimerID = startTimer(100);
}

void QtSGS::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == TimerID)
    {
        second++;
        if (second == 60)
        {
            second = 0;
            minute++;
            if (minute == 60)
            {
                minute = 0;
                hour++;
                if (hour == 24)
                {
                    hour = 0;
                    day++;
                    if (day == 8)
                        day = 1;
                }
            }
        }

        if (Simulation) // 如果开始模拟行进
        {
            if (FinalRoad[CurrentRoadNum].NodeNum < B_NodeNum)
                NowArea = true;
            else
                NowArea = false;

            if (CurrentRoadNum >= FinalRoadCount)
            {
                // 模拟结束
                Simulation = false;
                ui->StartSearch->setEnabled(true);
                ui->StopSearch->setEnabled(false);
                ui->ScanService->setEnabled(true);
                clearFinalRoad();
                StuX = -50;
                StuY = -50;
                CurrentRoadNum = 0;
                FinalRoadCount = 0;
            }
            else
            {
                // 逐帧移动学生位置
                double deltaX = FinalRoad[CurrentRoadNum + 1].LX - StuX;
                double deltaY = FinalRoad[CurrentRoadNum + 1].LY - StuY;
                double distance = sqrt(deltaX * deltaX + deltaY * deltaY);

                if (distance > 0)
                {
                    double Speed = StuSpeed / FinalRoad[CurrentRoadNum].Crowed;
                    double moveX = (deltaX / distance) * Speed;
                    double moveY = (deltaY / distance) * Speed;

                    StuX += moveX;
                    StuY += moveY;

                    // 判断是否到达下一个节点
                    if (fabs(deltaX) <= fabs(moveX) && fabs(deltaY) <= fabs(moveY))
                    {
                        StuX = FinalRoad[CurrentRoadNum + 1].LX;
                        StuY = FinalRoad[CurrentRoadNum + 1].LY;
                        CurrentRoadNum++;
                    }
                }
                else
                {
                    StuX = FinalRoad[CurrentRoadNum + 1].LX;
                    StuY = FinalRoad[CurrentRoadNum + 1].LY;
                    CurrentRoadNum++;
                }
            }
        }
    }
    repaint();
}

void QtSGS::paintEvent(QPaintEvent* e)
{
    QPainter pt(this);

    // 加载图片
    pt.drawImage(BRec, B_Img);
    QImage startImg("./Resources/Start.ico");
    QImage endImg("./Resources/End.ico");
    QImage BikeImg("./Resources/Bike.ico");
    QImage StuImg("./Resources/Student.ico");
    QImage landmarkImg("./Resources/landmark.ico");

    if (Simulation)
    {
        // 画起点和终点
        pt.drawImage(FinalRoad[0].LX - 10, FinalRoad[0].LY - 10, startImg); // 绘制起点
        pt.drawImage(FinalRoad[FinalRoadCount - 1].LX - 10, FinalRoad[FinalRoadCount - 1].LY - 10, endImg); // 绘制终点

        // 画学生位置
        // 步行
        if (!RideBike)
        {
            pt.drawImage(StuX - 20, StuY - 20, StuImg);
            QPen pen(QColor("#1819ff"), 4, Qt::SolidLine);
            pt.setPen(pen);
        }
        // 骑车   
        else
        {
            pt.drawImage(StuX - 20, StuY - 20, BikeImg);
            QPen pen(QColor("#93ec2e"), 4, Qt::SolidLine);
            pt.setPen(pen);
        }

        // 绘制当前学生位置到下一个结点的路线
        if (CurrentRoadNum < FinalRoadCount - 1)
        {
            pt.drawLine(StuX, StuY, FinalRoad[CurrentRoadNum + 1].LX, FinalRoad[CurrentRoadNum + 1].LY);
        }

        // 绘制从当前节点之后的路线
        for (int i = CurrentRoadNum + 1; i < FinalRoadCount - 1; i++)
        {
            if (NowArea && ((FinalRoad[i].NodeNum >= B_NodeNum || FinalRoad[i].NodeNum == -1) || (FinalRoad[i + 1].NodeNum >= B_NodeNum || FinalRoad[i + 1].NodeNum == -1)))
                break;
            else if (!NowArea && ((FinalRoad[i].NodeNum < B_NodeNum || FinalRoad[i].NodeNum == -1) || (FinalRoad[i + 1].NodeNum < B_NodeNum || FinalRoad[i + 1].NodeNum == -1)))
                break;
            pt.drawLine(FinalRoad[i].LX, FinalRoad[i].LY, FinalRoad[i + 1].LX, FinalRoad[i + 1].LY);
        }
    }

    // 绘制重要地点
    if (ShowScanRoad)
    {
        for (int i = 0; i < B_NodeNum; i++)
        {
            if (ScanRoad[i].NodeNum == -1)
                break;
            else
            {
                pt.drawImage(ScanRoad[i].LX - 15, ScanRoad[i].LY - 15, landmarkImg);
            }
        }
    }
}

void QtSGS::closeEvent(QCloseEvent* e)
{
}

bool QtSGS::SearchJudge()//搜索的前置判断
{
    if (ui->NavigationBox->currentIndex() == 0)
    {
        QMessageBox::information(this, "警告", QString("请选择导航策略"));
        return false;
    }

    else if (ui->NavigationBox->currentIndex() == 1 || ui->NavigationBox->currentIndex() == 2)
    {
        //判断起点终点是否为空或是否相同
        if (ui->StartBox->currentIndex() == 0 || ui->EndBox->currentIndex() == 0)
        {
            QMessageBox::information(this, "警告", QString("请选择当前位置和终点位置"));
            return false;
        }
        if (ui->StartBox->currentIndex() == ui->EndBox->currentIndex())
        {
            QMessageBox::information(this, "警告", QString("当前位置和终点位置不能相同"));
            return false;
        }
    }
    return true;
}

void QtSGS::clearFinalRoad()//清空导航路线
{
    for (int i = 0; i < 2 * NodeCount; i++)
        FinalRoad[i].NodeNum = -1;//根据NodeNum值判断导航是否结束
    FinalRoadCount = 0;
    ScanRoadCount = 0;
    ShowScanRoad = false;
    for (int j = 0; j < B_NodeNum; j++)
        ScanRoad[j].NodeNum = -1;
}



void QtSGS::TimerFaster()//加快虚拟时间
{
    killTimer(TimerID);
    TimerID = startTimer(5);
    ui->changeTimeF->setEnabled(false);
    ui->changeTimeS->setEnabled(true);
}

void QtSGS::TimerSlower()//减慢虚拟时间
{
    killTimer(TimerID);
    TimerID = startTimer(100);
    ui->changeTimeF->setEnabled(true);
    ui->changeTimeS->setEnabled(false);
}

void QtSGS::clickScanService()//扫描附近的服务设施
{
    clearFinalRoad();
    if (ui->StartBox->currentIndex() == 0)
    {
        QMessageBox::information(this, "警告", QString("请选择当前位置"));
        return;
    }
    Navigation* na = new Navigation();
    if (NowArea)
    {
        for (int i = 0; i < B_NodeNum; i++)
            if (na->BestRoad.MapNodes[i].ServiceNode && sqrt(pow(na->BestRoad.MapNodes[StartNodeNum].PX - na->BestRoad.MapNodes[i].PX, 2) +
                pow(na->BestRoad.MapNodes[StartNodeNum].PY - na->BestRoad.MapNodes[i].PY, 2)) <= 400)
            {
                ScanRoad[ScanRoadCount].NodeNum = i;
                ScanRoad[ScanRoadCount].LX =  na->BestRoad.MapNodes[i].PX;
                ScanRoad[ScanRoadCount].LY = 1000 - na->BestRoad.MapNodes[i].PY;
                ScanRoadCount++;
            }
    }
    delete na;
    ShowScanRoad = true;
}

void QtSGS::clickStartSearch() {
    if (!SearchJudge()) {
        return;
    }
    srand((unsigned)time(NULL));

    clearFinalRoad();

    bool isRideBike = (ui->NavigationBox->currentIndex() == 2); // 2为骑行策略
    double speed = isRideBike ? 2.4 : 1.2; // 速度：骑行2.4，步行1.2
    StartNodeNum = ui->StartBox->currentIndex() - 1;
    EndNodeNum = ui->EndBox->currentIndex() - 1;

    // 校验ID有效性
    if (StartNodeNum < 0 || EndNodeNum < 0 || StartNodeNum >= B_NodeNum || EndNodeNum >= B_NodeNum) {
        QMessageBox::warning(this, "错误", "起点/终点ID无效（超出节点范围或负数）");
        return;
    }

    // 骑行时需要验证起点和终点是否允许骑行
    if (isRideBike) {
        Navigation* na = new Navigation();
        if (!na->BestRoad.MapNodes[StartNodeNum].RideAllow || !na->BestRoad.MapNodes[EndNodeNum].RideAllow) {
            QMessageBox::information(this, "警告", QString("当前位置或终点位置不允许骑行"));
            delete na;
            return;
        }
        delete na;
    }

    Navigation* n = new Navigation();
    // 路径搜索：如果节点数多，这里会阻塞主线程
    n->Search1(StartNodeNum, EndNodeNum, isRideBike, (day - 1) * 86400 + hour * 3600 + minute * 60 + second);

    // 检查是否有有效路径
    if (n->BestRoad.TravelCityCount <= 0) {
        QMessageBox::warning(this, "错误", "路径搜索失败：未找到有效路径");
        delete n;
        return;
    }

    // 遍历最优路径，将每个节点信息存储到 FinalRoad 中
    for (int i = 0; i < n->BestRoad.TravelCityCount; i++) {
        FinalRoad[FinalRoadCount].NodeNum = n->BestRoad.TravelCityNum[i];
        FinalRoad[FinalRoadCount].LX = n->BestRoad.MapNodes[n->BestRoad.TravelCityNum[i]].LX;
        FinalRoad[FinalRoadCount].LY = 1000 - n->BestRoad.MapNodes[n->BestRoad.TravelCityNum[i]].LY;
        FinalRoad[FinalRoadCount].Crowed = n->BestRoad.MapNodes[n->BestRoad.TravelCityNum[i]].Crowed;
        FinalRoad[FinalRoadCount].WaitCount = 0;
        FinalRoadCount++;
    }

    // 设置终点信息
    FinalRoad[FinalRoadCount].NodeNum = FinalRoad[FinalRoadCount - 1].NodeNum;
    FinalRoad[FinalRoadCount].LX = n->BestRoad.MapNodes[FinalRoad[FinalRoadCount].NodeNum].PX;
    FinalRoad[FinalRoadCount].LY = 1000 - n->BestRoad.MapNodes[FinalRoad[FinalRoadCount].NodeNum].PY;
    FinalRoad[FinalRoadCount].Crowed = FinalRoad[FinalRoadCount - 1].Crowed;
    FinalRoad[FinalRoadCount].WaitCount = 0;

    Simulation = true;
    CurrentRoadNum = 0;
    StuX = FinalRoad[0].LX;
    StuY = FinalRoad[0].LY;
    StuSpeed = speed;
    RideBike = isRideBike;

    delete n;
    ui->StartSearch->setEnabled(false);
    ui->StopSearch->setEnabled(true);
    ui->ScanService->setEnabled(false);

    
    repaint();
}

void QtSGS::clickSwitchButton()
{
    if (NowArea)
        NowArea = false;
    else
        NowArea = true;
    repaint();
}

void QtSGS::clickStopSearchButton()
{
    this->Simulation = false;
    ui->StartSearch->setEnabled(true);
    ui->StopSearch->setEnabled(false);
    ui->ScanService->setEnabled(true);
}

void QtSGS::changeStartNodeNum()//如果下拉框有任何操作。则更新对应的信息
{
    StartNodeNum = (ui->StartBox->currentIndex()) - 1;
    // 👇 新增：验证函数是否执行
    QMessageBox::information(this, "调试", QString("起点ID更新为：%1").arg(StartNodeNum));
    qDebug() << "changeStartNodeNum执行，起点ID：" << StartNodeNum;
}

void QtSGS::changeEndNodeNum()
{
    EndNodeNum = (ui->EndBox->currentIndex()) - 1;
    // 👇 新增：验证函数是否执行
    QMessageBox::information(this, "调试", QString("终点ID更新为：%1").arg(EndNodeNum));
    qDebug() << "changeEndNodeNum执行，终点ID：" << EndNodeNum;
}