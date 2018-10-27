#include "tcpapp.h"
#include "ui_tcpapp.h"

TcpApp::TcpApp(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TcpApp),
    onNum(0)
{
    ui->setupUi(this);
    recvSize = 0;
    sendSize = 0;
    //初始化定时器
    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),this,SLOT(auto_time_send()));
}

TcpApp::~TcpApp()
{
    delete ui;
}

//与newconnection信号关联
void TcpApp::accept_connect()
{
    mSocket = mServer->nextPendingConnection(); //返回与客户端连接通信的套接字

    //关联接收数据信号
    connect(mSocket,SIGNAL(readyRead()),this,SLOT(recv_data()));
    //关联掉线信号
    connect(mSocket,SIGNAL(disconnected()),this,SLOT(client_disconnect()));

    //上线用户添加到客户列表容器
    clients.append(mSocket);
    //把用户添加到界面列表中
    QString ip = mSocket->peerAddress().toString().remove("::ffff:");//去除客户端中多余的字符
    ui->onlineUserList->addItem(ip);

    //在线数量添加
    onNum++;
    ui->onlineUserCount->setText(QString::number(onNum));//显示在线数
}

//接收数据
void  TcpApp::recv_data()
{
      QByteArray bytedata;
      QTextCodec *codec = QTextCodec::codecForName("GBK");


      QTcpSocket *obj = (QTcpSocket*)sender();
      //获取发送数据端的IP
      QString ip = obj->peerAddress().toString();
      ip.remove("::ffff:");
      QString msg;// = QString(obj->readAll());
      bytedata = obj->readAll();
      if(ui->checkBox_Hex_Recv->isChecked())
      {
          //bytedata = msg.toLatin1();
          msg = codec->toUnicode(bytedata.toHex());
          //bytedata = QString2Hex(msg);
          ui->receiveList->addItem(ip+":"+ msg);//显示接收到的数据
      }
      else
      {
          ui->receiveList->addItem(ip+":"+ QString(bytedata));//显示接收到的数据
      }
      //ui->receiveList->addItem(ip+":"+msg);//显示接收到的数据
      ui->receiveList->setCurrentRow(ui->receiveList->count()-1);
      recvSize += bytedata.size();//统计接收到的数据的字节数
      ui->receiveNumLabel->setText(QString::number(recvSize));
}

QByteArray TcpApp::QString2Hex(QString str)
{
    QByteArray senddata;
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len; )
    {
        hstr=str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);
    return senddata;
}
char TcpApp::ConvertHexChar(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else return (-1);
}

void TcpApp::client_disconnect()
{
    QTcpSocket *obj = (QTcpSocket*)sender();//获取掉线对象
    if(isServer)
    {
        int row = clients.indexOf(obj);//找到掉线对象的内容所在的行
        QListWidgetItem *item = ui->onlineUserList->takeItem(row);//从界面列表中去除找到的一行内容
        delete item;
        clients.remove(row);//从容器中删除对象

        //掉线时删除在线数量
        onNum--;
        ui->onlineUserCount->setText(QString::number(onNum));
    }
    else
    {
        ui->StartBt->setEnabled(true);//断开连接的时候重新启用开始按钮
    }
}


//客户端连接成功
void TcpApp::connect_suc()
{
    ui->StartBt->setEnabled(false);//连接成功则禁用开始按钮
}
//定时器定时发送数据
void TcpApp::auto_time_send()
{
    QByteArray str = "\r\n";
    QByteArray strData = ui->sendMsgEdit->toPlainText().toLocal8Bit();//m_pWidgets[i]->pEdit->text().toLocal8Bit();
    if(ui->checkBox_Hex_Send->isChecked())
    {
        strData = QByteArray::fromHex(strData);
    }
    else
    {
        strData += str;
    }
    qint64 len = mSocket->write(strData);
    if(len > 0)
    {
        sendSize += len;//统计发送的字节数
        ui->sendNumLabel->setText(QString::number(sendSize));//把发送的字节数显示到sendNumLabel上
    }
}

//选择作为服务器
void TcpApp::on_severRB_clicked()
{
    this->isCheckServer = true;
    this->isServer = true;
    //获取本地ip显示在IpEdit中
   ui->IpEdit->setText(QHostAddress(QHostAddress::LocalHost).toString());
   ui->IpEdit->setEnabled(false);//关闭ip输入编辑器
   this->isCheckClient = false;

}

//选择作为客户端
void TcpApp::on_clientRB_clicked()
{
    this->isCheckClient = true;
    this->isServer = false;
    ui->IpEdit->setEnabled(true);//打开ip输入编辑器
    this->isCheckServer = false;

}

//启动服务器或者链接服务器
void TcpApp::on_StartBt_clicked()
{
    if(isServer) //服务器
    {
        mServer = new QTcpServer();
        //关联新客户端链接信号
        connect(mServer,SIGNAL(newConnection()),this,SLOT(accept_connect()));
        mServer->listen(QHostAddress::Any,ui->PortEdit->text().toInt());//启动服务器监听
        ui->StartBt->setEnabled(false);//开始按钮禁用
    }
    if(isServer == false) //客户端
    {
        mSocket = new QTcpSocket();
        //检测链接成功信号
        connect(mSocket,SIGNAL(connected()),this,SLOT(connect_suc()));
        //设置服务器的 ip和端口号
        mSocket->connectToHost(ui->IpEdit->text(),ui->PortEdit->text().toInt());


        //关联接收数据信号
        connect(mSocket,SIGNAL(readyRead()),this,SLOT(recv_data()));
        //关联掉线信号
        connect(mSocket,SIGNAL(disconnected()),this,SLOT(client_disconnect()));
    }

    if(isCheckServer == false && isCheckClient == false)//如果两个都没选择
    {
        QMessageBox::warning(this,"提示","请选择服务器或者客户端");
        ui->StartBt->setEnabled(true);
        return;
    }

    if(isCheckServer)//选择了服务器
    {
       if(ui->PortEdit->text().isEmpty())
       {
           QMessageBox::warning(this,"提示","请输入端口号");
           ui->StartBt->setEnabled(true);
           return;
       }
    }

    if(isCheckClient)//选择了客户端
    {
        if(ui->IpEdit->text().isEmpty())
        {
            QMessageBox::warning(this,"提示","请输入ip和端口号");
            ui->StartBt->setEnabled(true);
            return;
        }
    }

}

//关闭服务器或者断开
void TcpApp::on_closeBt_clicked()
{
    if(isServer)//服务器
    {
        for(int i=0;i<clients.count();i++)
        {
             clients.at(i)->close();//关闭所有客户端
        }

        //关闭所有服务器之后开始按钮才能启用
        mServer->close();
        ui->StartBt->setEnabled(true);
    }
    else //客户端
    {
        mSocket->close();//关闭客户端
        ui->StartBt->setEnabled(true);//启用开始按钮
    }

}

//双击选择要发送的客户端
void TcpApp::on_onlineUserList_doubleClicked(const QModelIndex &index)
{
    mSocket = clients.at(index.row());

}

//自动发送数据
void TcpApp::on_autoCB_clicked(bool checked)
{
    if(checked)

    {
        if(ui->autoTimeEdit->text().toInt() <= 0)
        {
            QMessageBox::warning(this,"提示","请输入时间值ms");
            ui->autoCB->setChecked(false);//把按钮重新置于没选中的状态
            return;
        }
        mTimer->start(ui->autoTimeEdit->text().toInt());//启动定时器
    }
    else
    {
        mTimer->stop();//停止定时器
    }

}

//手动发送数据
void TcpApp::on_sendMsgBt_clicked()
{
    auto_time_send();

}

//清空接收区
void TcpApp::on_clearRcvBt_clicked()
{
    ui->receiveList->clear();
    this->recvSize = 0;
    ui->receiveNumLabel->setText(tr("0"));
}

//清空发送区
void TcpApp::on_clearSendBt_clicked()
{
    ui->sendMsgEdit->clear();
    this->sendSize = 0;
    ui->sendNumLabel->setText(tr("0"));
}
