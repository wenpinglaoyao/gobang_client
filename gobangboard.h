#ifndef GOBANGBOARD_H
#define GOBANGBOARD_H

#include <QWidget>
#include <QTcpSocket>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
enum DataType
{
    GAMESTART0 = 256,
    GAMESTART1,
    USERCONN,
    QUITGAME,
    SERVERCLOSE,
    SERVERFULL,
    TALK,
    BROADCAST,
    TIMEOUT,
    RANKLIST,
    WIN
};
namespace Ui {
class GobangBoard;
}

class GobangBoard : public QWidget
{
    Q_OBJECT
public:
    explicit GobangBoard(QWidget *parent = 0);
    ~GobangBoard();
private slots:
    void slotTimeout();
    void slotGetServerData();

    void on_connServerBtn_clicked();

    void on_sendMsg_clicked();

    void on_broadcastBtn_clicked();

    void on_startBtn_clicked();

    void on_loseBtn_clicked();


    void on_ranklistBtn_clicked();

protected:
    int checkEndGame(); //判断游戏是否结束并返回游戏状态（0没结束，1先手胜，-1后手胜）
private:
    Ui::GobangBoard *ui;
    QPushButton _btn[15][15];
    bool _turn; //这个代表当前用户的使用棋子阵营，0是先走的黑棋，1是后走的红棋
    int _bitBoard[15][15]; //这个是棋盘状态
    int _myNum;  //这个是自己的编号
    int _rivalNum; //这个是对手的编号；
    int _stepCount; //这个用来统计棋局进行了多少步
    int _intOfRivalSocket;   //这个是对手的套接字，下棋或者私聊时，要把它发给服务器，然后服务器用它私聊对手
    QTcpSocket* _socket;
    QTimer _timer;
    int _timeCount; //与上面的计时器配合，用来看看自己或对手有没有超时
    int _totalCount; //这个是总局时
    int _score; //这个用来记录用户净胜多少盘棋（赢一盘加一分，输一盘减一分）
};

#endif // GOBANGBOARD_H
