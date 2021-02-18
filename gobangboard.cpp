#include "gobangboard.h"
#include "ui_gobangboard.h"
#include <QPushButton>
#include <QGridLayout>
#include <QSignalMapper>
#include <QCloseEvent>
#define TOTALCOUNT 1500

GobangBoard::GobangBoard(QWidget *parent) :
    QWidget(parent), _turn(true),
    ui(new Ui::GobangBoard)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    setWindowTitle(tr("五子棋客户端"));
    QRegExp regIP = QRegExp("([1-9]|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])(\\.(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])){3}");
    ui->serverIPLE->setValidator(new QRegExpValidator(regIP,this));
    QRegExp regPort = QRegExp("([1-9]\\d{3})|(65[1-4]\\d{2})");
    ui->serverPortLE->setValidator(new QRegExpValidator(regPort,this));
    _socket = NULL;
    _score = 0;
    _totalCount = TOTALCOUNT;
    ui->totalLCD->display(_totalCount);
    _timeCount = 100; //初始化步时为100秒
    for(int i=0; i<15; i++) //初始化五子棋盘按钮们
        for(int j=0; j<15; j++)
        {
            _btn[i][j].setFixedSize(40,40);
            ui->gridLayout->addWidget(&_btn[i][j],i,j);
            _btn[i][j].setStyleSheet("color:red");
            QFont font(nullptr,30);
            _btn[i][j].setFont(font);

            connect(&_btn[i][j],&QPushButton::clicked,
                [this,i,j](){
                if(_socket && _turn == _stepCount%2)
                {
                    QByteArray buff;
                    QDataStream out(&buff,QIODevice::WriteOnly);
                    out<<i*15+j<<_intOfRivalSocket;
                    _socket->write(buff);
                }
            });
            _bitBoard[i][j] = 0; //清空棋盘
        }
}

GobangBoard::~GobangBoard()
{
    delete ui;
}

int GobangBoard::checkEndGame() //检测棋局是否分出胜负了，黑棋赢了返回1，红棋赢了返回-1，不然返回0
{
    int status=0,lineCount=0;
    for(int row=0; row<15; row++)
    {
        status=0;
        for(int col=0; col<15; col++) //先测试水平线上有没有五子连线
        {
            if(_bitBoard[row][col] == status)
            {
                lineCount += _bitBoard[row][col];
                if(lineCount >= 5)
                    return 1;
                else if(lineCount <= -5)
                    return -1;
            }else{
                status = _bitBoard[row][col];
                if(status)
                    lineCount = (1==status ? 1 : -1);
            }
        }
    }

    for(int col=0; col<15; col++) //这里测试垂直线上有没有五子连线
    {
        status = 0;
        for(int row=0; row<15; row++)
        {
            if(_bitBoard[row][col] == status)
            {
                lineCount += _bitBoard[row][col];
                if(lineCount >= 5)
                    return 1;
                else if(lineCount <= -5)
                    return -1;
            }else{
                status = _bitBoard[row][col];
                if(status)
                    lineCount = (1==status ? 1 : -1);
            }
        }
    }

    for(int row=0; row<11; row ++) //左上右下测试有没有五子连线（下半三角区）
    {
        status = 0;
        for(int col=0; col+row<15; col++)
        {
            if(_bitBoard[row+col][col] == status)
            {
                lineCount += _bitBoard[row+col][col];
                if(lineCount >= 5)
                    return 1;
                else if(lineCount <= -5)
                    return -1;
            }else{
                status = _bitBoard[row+col][col];
                if(status)
                    lineCount = (1==status ? 1 : -1);
            }
        }
    }
    for(int col=1; col<11; col++) //左上右下测试有没有五子连线（上半三角区）
    {
        status = 0;
        for(int row=0; row+col<15; row++)
        {
            if(_bitBoard[row][col+row] == status)
            {
                lineCount += _bitBoard[row][col+row];
                if(lineCount >= 5)
                    return 1;
                else if(lineCount <= -5)
                    return -1;
            }else{
                status = _bitBoard[row][col+row];
                if(status)
                    lineCount = (1==status ? 1 : -1);
            }
        }
    }

    for(int row=4; row<15; row++) //右上左下测试有没有五子连线（上三角半区）
    {
        status = 0;
        for(int col=0; row-col>=0; col++)
        {
            if(_bitBoard[row-col][col] == status)
            {
                lineCount += _bitBoard[row-col][col];
                if(lineCount >= 5)
                    return 1;
                else if(lineCount <= -5)
                    return -1;
            }else{
                status = _bitBoard[row-col][col];
                if(status)
                    lineCount = (1==status ? 1 : -1);
            }
        }
    }
    for(int col=1; col<15; col++) //右上左下测试有没有五子连线（下三角半区）
    {
        status = 0;
        for(int row=14; row-col>=0; row--)
        {
            if(_bitBoard[row][14-row+col] == status)
            {
                lineCount += _bitBoard[row][14-row+col];
                if(lineCount >= 5)
                    return 1;
                else if(lineCount <= -5)
                    return -1;
            }else{
                status = _bitBoard[row][14-row+col];
                if(status)
                    lineCount = (1==status ? 1 : -1);
            }
        }
    }

    return 0;
}

void GobangBoard::on_connServerBtn_clicked() //当点击连接服务器的按钮之后
{
    if(_totalCount<=0)
        return;
    ui->connServerBtn->setEnabled(false);
    _socket = new QTcpSocket(this);
    _socket->connectToHost(ui->serverIPLE->text(),ui->serverPortLE->text().toInt());
    connect(_socket,&QTcpSocket::readyRead,  this,&GobangBoard::slotGetServerData);
    connect(_socket,&QTcpSocket::disconnected,[this](){
        QMessageBox::warning(0,"警告","抱歉，您和服务器的链接断开！");
    });
}

void GobangBoard::slotGetServerData() //如果服务器端对我发来了数据...
{
    QDataStream in;
    in.setDevice(_socket);
    while(_socket && _socket->bytesAvailable())
    {
        int type;
        in>>type;

        switch(type)
        {
        case GAMESTART0: //服务器发来了配对消息，接下来初始化您的开局状态
        case GAMESTART1:
        {
            in>>_intOfRivalSocket>>_myNum>>_rivalNum;
            ui->rivalLB->setText(tr("服务器为您匹配的对手是%1号选手").arg(_rivalNum));
            ui->gameStatusLB->setText("对局已经开始了");
            _turn = type%2;

            for(int i=0; i<15*15; i++)
            {
                _btn[i/15][i%15].setText("");
                _btn[i/15][i%15].setEnabled(true);
                _bitBoard[i/15][i%15] = 0; //清空棋盘
            }
            _stepCount=0;
            _timeCount = 100; //重新刷新步时100秒
            _timer.start(1000);
            ui->loseBtn->setEnabled(true);
            connect(&_timer,&QTimer::timeout,  this,&GobangBoard::slotTimeout);
        }break;
        case USERCONN:
        {
            in>>_myNum;
            ui->MYLB->setText(tr("您是%1号选手！").arg(_myNum));
        }break;
        case QUITGAME: //服务器告诉您，对手逃跑了
        {
            ui->gameStatusLB->setText("您的对手逃跑了，系统盼您获胜！");
            ui->rivalLB->setText("您可以重新开局了");
            _intOfRivalSocket = 0;
            ui->loseBtn->setEnabled(false); //对手逃跑了，自己的认输按钮失效
            for(int i=0; i<15*15; i++)
                _btn[i/15][i%15].setEnabled(false);//所有棋块按钮都不能按了
            _timer.stop();  //对手逃跑了，停止计时
        }break;
        case SERVERCLOSE:
        {
            QMessageBox::warning(0,tr("警告"),tr("抱歉，服务器关闭了服务！"));
        }break;
        case SERVERFULL:
        {
            QMessageBox::warning(0,tr("警告"),tr("抱歉，服务器人满了！请稍候再试"));
            _socket->close();
            _socket = nullptr;
        }break;
        case TALK: //服务器转发了对手给你发的话
        {
            QString str;
            in>>str;
            ui->textBrowser->setTextColor(_turn ? Qt::black : Qt::red);
            ui->textBrowser->append(tr("您的对手对您说:%1").arg(str));
        }break;
        case BROADCAST: //服务器转发了某选手的广播信息
        {
            QString str;
            in>>str;
            ui->textBrowser->setTextColor(Qt::blue);
            ui->textBrowser->append(str);
        }break;
        case TIMEOUT: //服务器发来了对手超时的信息
        {
            _timer.stop(); //对手输了，停止自己的计时
            ui->loseBtn->setEnabled(false); //对手输了，自己的认输按钮失效
            ui->gameStatusLB->setText("对手超时或认输了，恭喜您获胜!");
            ui->rivalLB->setText("您可以重新开局了");
            ui->startBtn->setEnabled(true); //可以重开一局了，开始按钮生效
            for(int i=0; i<15*15; i++)    //因为对局结束，所有的棋块按钮失效
                _btn[i/15][i%15].setEnabled(false);
        }break;
        case RANKLIST: //服务器响应了您的查看排行榜请求
        {
            QString str;
            in>>str;
            QMessageBox::information(0,"排行榜",str);
        }break;
        case WIN: //服务器发来了某个选手获胜的信息
        {
            QString str;
            in>>str;
            ui->textBrowser->setTextColor(Qt::green);
            ui->textBrowser->append(str);
        }break;
        default: //更新棋盘，因为自己或对手走棋了
        {
            _timeCount = 100; //重新初始化步时数为100，表示恢复步时100秒计时
            _btn[type/15][type%15].setStyleSheet((_stepCount%2) ? "color:red" : "color:black");
            _btn[type/15][type%15].setText((_stepCount%2) ? "X" : "O");
            _btn[type/15][type%15].setEnabled(false);
            _bitBoard[type/15][type%15] = (_stepCount%2) ? 1 : -1;
            _stepCount++;

            if(_stepCount%2 == _turn)
                ui->gameStatusLB->setText(tr("棋局进行到第%1手，现在轮到您走棋，您是%2")
                                          .arg(_stepCount).arg(!_turn ? "O字黑棋" : "X字红棋"));
            else
                ui->gameStatusLB->setText(tr("棋局进行到第%1手，对手正在思考中").arg(_stepCount));

            //然后检测己方有没有获得胜利，如果获得胜利，向服务器发送己方获胜的信息
            int result = this->checkEndGame();
            if(1 == result) //如果是后手红X赢了
            {
                if(_turn) //如果我是后手方
                {
                    ui->gameStatusLB->setText("经检测，您执红战胜了对手，恭喜！");
                    ui->rivalLB->setText("您可以重新开局了");
                    _score++;
                    QByteArray buf;
                    QDataStream out(&buf,QIODevice::WriteOnly);
                    out<<WIN<<_myNum<<_rivalNum;
                    _socket->write(buf);
                    ui->startBtn->setEnabled(true);
                }else{
                    ui->gameStatusLB->setText("经检测，很遗憾，您战败了。");
                    ui->startBtn->setEnabled(true);
                }
                for(int i=0; i<15*15; i++) //不管怎样，对局已经结局
                    _btn[i/15][i%15].setEnabled(false);//所以棋块按钮先失效
                _timer.stop();         //停止计时
                ui->loseBtn->setEnabled(false); //
                ui->startBtn->setEnabled(true);
            }
            else if(-1 == result) //如果是先手黑O赢了
            {
               if(!_turn) //如果我是先手方
               {
                    ui->gameStatusLB->setText("经检测，您执黑战胜了对手，恭喜！");
                    ui->rivalLB->setText("您可以重新开局了");
                    _score++;
                    QByteArray buf;
                    QDataStream out(&buf,QIODevice::WriteOnly);
                    out<<WIN<<_myNum<<_rivalNum;
                    _socket->write(buf);
                    ui->startBtn->setEnabled(true);
               }else{
                   ui->gameStatusLB->setText("经检测，很遗憾，您战败了。");
                   ui->startBtn->setEnabled(true);
               }
               for(int i=0; i<15*15; i++)
                   _btn[i/15][i%15].setEnabled(false);
               _timer.stop();
               ui->loseBtn->setEnabled(false);
               ui->startBtn->setEnabled(true);
            }
        }break;
        }

    }
}

void GobangBoard::slotTimeout()
{
    ui->lcdNumber->setStyleSheet(_turn == _stepCount%2 ? "color:red" : "color:green");
    ui->lcdNumber->display(_timeCount--);
    if(_stepCount%2 == _turn)  //如果轮到我走棋
    {
        ui->totalLCD->display(_totalCount--);
        if(_totalCount<=0)
        {
            _socket->close();
            this->close();
        }
        if(0 >=_timeCount)     //并且时间耗光了
        {
            _timer.stop();
            QByteArray buf;
            QDataStream out(&buf,QIODevice::WriteOnly);
            out<<WIN<<_rivalNum<<_myNum;
            _socket->write(buf);
            ui->gameStatusLB->setText("很遗憾，这句您步时超了，惜败！");
            ui->startBtn->setEnabled(true);
        }
    }
    if(!_timeCount)
        for(int i=0; i<15*15; i++)
            _btn[i/15][i%15].setEnabled(false);
}

void GobangBoard::on_sendMsg_clicked()//发送给对手的私聊信息
{
    if(_socket && "" != ui->textEdit->document()->toPlainText())
    {
        ui->textBrowser->setTextColor(_turn ? Qt::red : Qt::black);
        ui->textBrowser->append(tr("您说：%1").arg(ui->textEdit->document()->toPlainText()));

        QByteArray buf;
        QDataStream out(&buf,QIODevice::WriteOnly);
        out<<TALK<<_intOfRivalSocket<<ui->textEdit->document()->toPlainText();
        _socket->write(buf);
        ui->textEdit->clear();
    }
}

void GobangBoard::on_broadcastBtn_clicked() //发送到大厅的广播信息
{
    if(_socket && ""!= ui->textEdit->document()->toPlainText())
    {
        QString str = tr("%1号选手对大家说:").arg(_myNum) + ui->textEdit->document()->toPlainText();
        QByteArray buf;
        QDataStream out(&buf,QIODevice::WriteOnly);
        out<<BROADCAST<<str;
        _socket->write(buf);
    }
}

void GobangBoard::on_startBtn_clicked()
{
    if(_socket)
    {
        ui->gameStatusLB->setText("正在为您匹配对手，请稍候");
        QByteArray buf;
        QDataStream out(&buf,QIODevice::WriteOnly);
        out<<GAMESTART1;
        _socket->write(buf);
        ui->startBtn->setEnabled(false);
    }else{
        ui->gameStatusLB->setText("您还没有进入服务器！");
    }
}

void GobangBoard::on_loseBtn_clicked()
{
    if(_socket)
    {
        ui->loseBtn->setEnabled(false);
        _timer.stop();
        QByteArray buf;
        QDataStream out(&buf,QIODevice::WriteOnly);
        out<<WIN<<_rivalNum<<_myNum;
        _socket->write(buf);
        ui->gameStatusLB->setText("您已经认输了");
        ui->rivalLB->setText("您可以重新开局了");
        ui->startBtn->setEnabled(true);
    }else{
        QMessageBox::warning(0,"警告","您还没有进入游戏大厅！");
    }
}


void GobangBoard::on_ranklistBtn_clicked()
{
    if(_socket)
    {
        QByteArray buf;
        QDataStream out(&buf,QIODevice::WriteOnly);
        out<<RANKLIST;
        _socket->write(buf);
    }
}
