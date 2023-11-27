#include "mytcpsocket.h"
#include"opedb.h"
#include <QDebug>
#include "mytcpserver.h"
#include <QDir>

MyTcpSocket::MyTcpSocket(QObject *parent) : QTcpSocket(parent)
{
    connect(this,SIGNAL(readyRead()),
            this,SLOT(recvMsg()));
    connect(this,SIGNAL(disconnected()),
            this,SLOT(clientOffline()));

}

QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::recvMsg()
{
    //接收该socket中的PDU
    qDebug()<<this->bytesAvailable();
    uint uiPDULen = 0;
    this->read((char*)&uiPDULen,sizeof(uint));
    uint uiMsgLen = uiPDULen-sizeof(PDU);
    PDU*pdu = mkPDU(uiMsgLen);
    this->read((char*)pdu+sizeof(uint),uiPDULen);


//  qDebug()<<pdu->uiMsgType<<(char*)(pdu->caMsg);
    //根据PDU的消息类型进行不同的操作
    switch (pdu->uiMsgType)
    {
    case ENUM_MSG_TYPE_REGISTER_REQUEST:
    {
        char caName[32] ={"/0"};
        char caPwd[32] ={"/0"};
        strncpy(caName,pdu->caData,32);
        strncpy(caPwd,pdu->caData+32,32);
        bool ret = OpeDB::getInstance().handleRegist(caName,caPwd);

        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_REGISTER_RESPONSE;
        qDebug()<<ret;
        if(ret){
            strcpy(respdu->caData, REGIST_OK);
            QDir dir;
            //创建根目录
            bool ret = dir.mkdir(QString("./%1").arg(caName));
            qDebug()<<ret;
            qDebug()<<QString("./%1").arg(caName);
        }else{
            strcpy(respdu->caData, REGIST_FAILED);
        }

        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;

        break;
    }
    case ENUM_MSG_TYPE_LOGIN_REQUEST:
    {
        char caName[32] ={"/0"};
        char caPwd[32] ={"/0"};
        strncpy(caName,pdu->caData,32);
        strncpy(caPwd,pdu->caData+32,32);
        bool ret = OpeDB::getInstance().handleLogin(caName,caPwd);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPONSE;
        qDebug()<<ret;
        if(ret){
            strcpy(respdu->caData, LOGIN_OK);
            m_strName = caName;

        }else{
            strcpy(respdu->caData, LOGIN_FAILED);
        }

        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:
    {
        QStringList ret = OpeDB::getInstance().handleAllOnline();
        uint uiMsgLen = ret.size()*32;
        PDU *respdu = mkPDU(uiMsgLen);
        respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPONSE;
        qDebug()<<ret;
        for(int i=0;i<ret.size();i++){
            memcpy((char*)(respdu->caMsg)+i*32
                   ,ret.at(i).toStdString().c_str()
                   ,ret.at(i).size());

        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;

    }
    case ENUM_MSG_TYPE_ALL_USR_REQUEST:
    {
        QStringList ret = OpeDB::getInstance().handleAllUsr();
        uint uiMsgLen = ret.size()*32;
        PDU *respdu = mkPDU(uiMsgLen);
        respdu->uiMsgType = ENUM_MSG_TYPE_ALL_USR_RESPONSE;
        qDebug()<<ret;
        for(int i=0;i<ret.size();i++){
            memcpy((char*)(respdu->caMsg)+i*32
                   ,ret.at(i).toStdString().c_str()
                   ,ret.at(i).size());

        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:
    {

        int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData);

        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPONSE;
        qDebug()<<ret;
        if(ret == -1){
            strcpy(respdu->caData,SEARCH_USR_NO);
            qDebug()<<(char*)(respdu->caData);
        }else if(ret == 1){
            strcpy(respdu->caData,SEARCH_USR_ONLINE);
            qDebug()<<(char*)(respdu->caData);
        }else if(ret == 0){
            strcpy(respdu->caData,SEARCH_USR_OFFLINE);
            qDebug()<<(char*)(respdu->caData);
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;


        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
    {
       char caName[32] ={"/0"};
       char caPerName[32] ={"/0"};
       strncpy(caPerName,pdu->caData,32);
       strncpy(caName,pdu->caData+32,32);
       int ret = OpeDB::getInstance().handleAddFriend(caPerName,caName);

       PDU* respdu = NULL;

       if(ret==-1){
           respdu = mkPDU(0);
           respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPONSE;
           strcpy(respdu->caData,UNKNOW_ERROR);
           write((char*)respdu,respdu->uiPDULen);
           free(respdu);
           respdu = NULL;


       }else if(ret==0){
           respdu = mkPDU(0);
           respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPONSE;
           strcpy(respdu->caData,EXISTED_FRIEND);
           write((char*)respdu,respdu->uiPDULen);
           free(respdu);
           respdu = NULL;




       }else if(ret==1){


           MyTcpServer::getInstance().resend(caPerName,pdu);



       }else if(ret==2){
           respdu = mkPDU(0);
           respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPONSE;
           strcpy(respdu->caData,ADD_FRIEND_OFFLINE);

           write((char*)respdu,respdu->uiPDULen);
           free(respdu);
           respdu = NULL;


       }else if(ret==3){
           respdu = mkPDU(0);
           respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPONSE;
           strcpy(respdu->caData,ADD_FRIEND_NO_EXSIT);
           write((char*)respdu,respdu->uiPDULen);
           free(respdu);
           respdu = NULL;

       }


       break;



    }
    case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
    {
        char caName[32] ={"/0"};
        char caPerName[32] ={"/0"};
        strncpy(caPerName,pdu->caData,32);
        strncpy(caName,pdu->caData+32,32);
        OpeDB::getInstance().handleAgreeAddFriend(caPerName,caName);




        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPONSE;
        strcpy(respdu->caData,ADD_FRIEND_AGREE);

        MyTcpServer::getInstance().resend(caName,respdu);

        break;
    }
    case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
    {
        char caName[32] ={"/0"};

        strncpy(caName,pdu->caData+32,32);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPONSE;
        strcpy(respdu->caData,ADD_FRIEND_REFUSE);
        MyTcpServer::getInstance().resend(caName,respdu);
        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:
    {
        char caName[32] ={"/0"};
        strncpy(caName,pdu->caData,32);
        qDebug()<<caName;
        QStringList ret = OpeDB::getInstance().handleFulshFriend(caName);
        uint uiMsgLen = ret.size()*32;
        PDU *respdu = mkPDU(uiMsgLen);
        respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPONSE;

        for(int i=0;i<ret.size();i++){
            memcpy((char*)(respdu->caMsg)+i*32
                   ,ret.at(i).toStdString().c_str()
                   ,ret.at(i).size());
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
    {

        char caSelfName[32] = {"\0"};
        char caFriendName[32] = {"\0"};
        strncpy(caSelfName,pdu->caData,32);
        strncpy(caFriendName,pdu->caData+32,32);
        OpeDB::getInstance().handleDelFriend(caSelfName,caFriendName);

        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPONSE;
        strcpy(respdu->caData,DEL_FRIEND_OK);
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;


        MyTcpServer::getInstance().resend(caFriendName,pdu);


        break;


    }
    case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
    {
        char caPerName[32] ={'\0'};
        memcpy(caPerName,pdu->caData+32,32);
        MyTcpServer::getInstance().resend(caPerName,pdu);
        break;
    }
    case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
    {
        char caName[32] ={"/0"};
        strncpy(caName,pdu->caData,32);
        qDebug()<<caName;
        QStringList onlineFriend = OpeDB::getInstance().handleGroupChat();
        QString tmp;
        for (int i=0;i<onlineFriend.size();i++) {
            tmp = onlineFriend.at(i);
            MyTcpServer::getInstance().resend(tmp.toStdString().c_str(),pdu);
        }
        break;
    }
    case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
    {
        QDir dir;
        QString strCurPath = QString("%1").arg((char*)pdu->caMsg);
        qDebug()<<strCurPath;
        bool ret = dir.exists(strCurPath);
        if(ret)//当前目录存在
        {
            char caNewDir[32] = {'\0'};
            memcpy(caNewDir,pdu->caData+32,32);
            QString strNewPath = strCurPath+"/"+caNewDir;
            qDebug()<<strNewPath;
            ret = dir.exists(strNewPath);
            qDebug()<<"-->"<<ret;
            if(ret)//创建的文件夹已经存在
            {
                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPONSE;
                strncpy(respdu->caData,FILE_NAME_EXIST,32);
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            else
            {
                QDir dir;
                //创建根目录
                bool ret = dir.mkdir(QString("./%1").arg(strNewPath));
                qDebug()<<ret;
                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPONSE;
                strncpy(respdu->caData,CREATE_DIR_OK,32);
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
        }
        else //当前目录不存在
        {
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPONSE;
            strncpy(respdu->caData,DIR_NO_EXIST,32);
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }

        break;
    }
    case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:
    {
        char *pCurPath = new char[pdu->uiMsgLen];
        memcpy(pCurPath,pdu->caMsg,pdu->uiMsgLen);
        QDir dir(pCurPath);
        QFileInfoList fileInfoList = dir.entryInfoList();

        int iFileCount = fileInfoList.size();
        PDU *respdu = mkPDU(sizeof(FileInfo)*iFileCount);
        FileInfo *pFileInfo = NULL;
        QString strFileName;
        respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPONSE;

        //根cmd的操作一样
        for (int i = 0;i<iFileCount;i++)
        {


            pFileInfo = (FileInfo*)(respdu->caMsg)+i;
            strFileName = fileInfoList[i].fileName();
            memcpy(pFileInfo->caFileName,strFileName.toStdString().c_str(),strFileName.size());

            if(fileInfoList[i].isDir()){
                pFileInfo->iFileType = 0;
            }else if (fileInfoList[i].isFile()) {
                pFileInfo->iFileType = 1;
            }



          qDebug() << fileInfoList[i].fileName()
                   << fileInfoList[i].size()
                    <<"dirictory:"<< fileInfoList[i].isDir()
                    <<"常规文件：" <<fileInfoList[i].isFile();
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;
        break;
    }
    case ENUM_MSG_TYPE_DEL_DIR_REQUEST:
    {
        char caName[32] = {'\0'};
        strcpy(caName,pdu->caData);
        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath,pdu->caMsg,pdu->uiMsgType);
        QString strPath =  QString("%1/%2").arg(pPath).arg(caName);//如果是二级文件夹，那么拼接是不对的，默认是一级文件夹
        //删除dir和非dir方式不一样
        QFileInfo fileInfo(strPath);
        bool ret=false;
        if(fileInfo.isDir())
        {
            QDir dir;
            dir.setPath(strPath);
            ret = dir.removeRecursively();//dir专用删除方式
        }
        else if (fileInfo.isFile())
        {
            ret = false;

        }
        PDU *respdu = NULL;
        if(ret)
        {
            respdu = mkPDU(strlen(DEL_DIR_OK)+1);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPONSE;
            memcpy(respdu->caData,DEL_DIR_OK,strlen(DEL_DIR_OK));
        }
        else
        {
            respdu = mkPDU(strlen(DEL_DIR_FAILED)+1);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPONSE;
            memcpy(respdu->caData,DEL_DIR_FAILED,strlen(DEL_DIR_FAILED));
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;

        break;
    }
    case ENUM_MSG_TYPE_RENAME_FILE_REQUEST:
    {
        char caOldName[32] = {'\0'};
        char caNewName[32] = {'\0'};
        strncpy(caOldName,pdu->caData,32);
        strncpy(caNewName,pdu->caData+32,32);

        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
        QString strOldPath = QString("%1/%2").arg(pPath).arg(caOldName);
        QString strNewPath = QString("%1/%2").arg(pPath).arg(caNewName);

        QDir dir;
        bool ret = dir.rename(strOldPath,strNewPath);
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPONSE;
        if(ret)
        {
            strcpy(respdu->caData,RENAME_FILE_OK);
        }
        else
        {
            strcpy(respdu->caData,RENAME_FILE_FAILED);
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;

        break;
    }
    case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:
    {
        char caEnterName[32] = {'\0'};

        strncpy(caEnterName,pdu->caData,32);

        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
        QString strPath = QString("%1/%2").arg(pPath).arg(caEnterName);
        QFileInfo fileInfo(strPath);



        if(fileInfo.isDir())
        {
            QDir dir(strPath);
            QFileInfoList fileInfoList = dir.entryInfoList();

            int iFileCount = fileInfoList.size();
            PDU *respdu = mkPDU(sizeof(FileInfo)*iFileCount);
            FileInfo *pFileInfo = NULL;
            QString strFileName;
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPONSE;

            //根cmd的操作一样
            for (int i = 0;i<iFileCount;i++)
            {


                pFileInfo = (FileInfo*)(respdu->caMsg)+i;
                strFileName = fileInfoList[i].fileName();
                memcpy(pFileInfo->caFileName,strFileName.toStdString().c_str(),strFileName.size());

                if(fileInfoList[i].isDir()){
                    pFileInfo->iFileType = 0;
                }else if (fileInfoList[i].isFile()) {
                    pFileInfo->iFileType = 1;
                }



              qDebug() << fileInfoList[i].fileName()
                       << fileInfoList[i].size()
                        <<"dirictory:"<< fileInfoList[i].isDir()
                        <<"常规文件：" <<fileInfoList[i].isFile();
            }
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (fileInfo.isFile())
        {
            PDU *respdu = NULL;
            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPONSE;

            strncpy(respdu->caData,ENTER_DIR_FAILED,strlen(ENTER_DIR_FAILED));
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }

        break;
    }
    case ENUM_MSG_TYPE_DEL_FILE_REQUEST:
    {

        char caName[32] = {'\0'};
        strcpy(caName,pdu->caData);
        char *pPath = new char[pdu->uiMsgLen];
        memcpy(pPath,pdu->caMsg,pdu->uiMsgType);
        QString strPath =  QString("%1/%2").arg(pPath).arg(caName);//如果是二级文件夹，那么拼接是不对的，默认是一级文件夹
        //删除dir和非dir方式不一样
        QFileInfo fileInfo(strPath);
        bool ret=false;
        if(fileInfo.isDir())
        {
            ret = false;
        }
        else if (fileInfo.isFile())
        {
            QDir dir;
            ret = dir.remove(strPath);

        }
        PDU *respdu = NULL;
        if(ret)
        {
            respdu = mkPDU(strlen(DEL_FILE_OK)+1);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPONSE;
            memcpy(respdu->caData,DEL_FILE_OK,strlen(DEL_FILE_OK));
        }
        else
        {
            respdu = mkPDU(strlen(DEL_FILE_FAILED)+1);
            respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPONSE;
            memcpy(respdu->caData,DEL_FILE_FAILED,strlen(DEL_FILE_FAILED));
        }
        write((char*)respdu,respdu->uiPDULen);
        free(respdu);
        respdu = NULL;






        break;
    }
    default:
    {
        break;
    }
    free(pdu);
    pdu = NULL;
    }


}

void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this);
}
