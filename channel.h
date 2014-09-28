#ifndef CHANNEL_H
#define CHANNEL_H

#include <arpa/inet.h>
#include <stdlib.h>
#include <iostream>

class channel;
class observer {
public:
    virtual void notifyData(channel* ch, 
        const char* buf, const int len)=0;
    virtual void notifySetup(channel* ch)=0;
};
class channel {
    int _fd;
    observer* _ob;
    bool _actived;
#define READ_BUF_SIZE 1024*70
    char _readBuf[READ_BUF_SIZE];
public:
    channel():_fd(-1),_ob(NULL),_actived(false){;}
    ~channel() {
        if (_actived) {
            close(_fd);
        }
    }
    void attachOB(observer* ob) {_ob=ob;}
    inline int getFD() {return _fd;}
    bool channelOutput(const char* buf, const int len) {
        for (int count=len;count>0;) {
            int once=write(_fd, buf, len);
            if (once<0) {
                //TODO log error
                return false;
            }
            count-=once;
        }
        return true;
    }
    inline void deactivate(void) {_actived=false;}
    void channelInput(void) {
        int len=read(_fd, (void*)_readBuf, READ_BUF_SIZE);
        if (len==0) {
            _actived=false;
            return;
        }
        if (len<0) {
            //TODO log error
            return;
        }
        _ob->notifyData(this, _readBuf, len);
        return;
    }
    bool activate(const char* remoteIP, 
        const char* remotePort) {
        if (_actived) {
            return true;
        }
        _fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_fd<=0) {
            return false;
        }
        struct sockaddr_in ser_addr;
        ser_addr.sin_family=AF_INET;
        uint16_t numPort=atoi(remotePort);
        ser_addr.sin_port=htons(numPort);
        uint32_t serAddr =0;
        int rc= inet_pton(AF_INET, 
            remoteIP, (void*)&serAddr);
        if (rc<=0) {
            return false;
        }
        ser_addr.sin_addr.s_addr=(serAddr);
        rc=connect(_fd, (struct sockaddr*)&ser_addr, 
            sizeof(struct sockaddr_in));
        if (rc!=0) { 
            return false;
        }
        _actived=true;
        _ob->notifySetup(this);
        return true;
    }
    inline bool isActive(void) {return _actived;}
};


#endif

