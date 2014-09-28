#ifndef EVENTLOOPER_H
#define EVENTLOOPER_H
#include <sys/epoll.h>
#include "channel.h"
#include <iostream>
#include <stdlib.h>
class eventLooper {
    int _epfd;
    int _sizeHint;
    bool _exitNow;
    static const int MAXEVENTS=1024;
    struct epoll_event* _events;
public:
    eventLooper(int sizeHint):
        _sizeHint(sizeHint),
        _exitNow(false) {
        _epfd = epoll_create(_sizeHint);
        _events = (struct epoll_event*)
            calloc(MAXEVENTS, sizeof(struct epoll_event));
    }
    bool watch(channel* chn) {
        if (!chn || !chn->isActive()) {
            return false;
        }
        struct epoll_event ev;
        ev.data.ptr=(void*) chn;
        ev.events=EPOLLIN;
        int rc= epoll_ctl(_epfd, EPOLL_CTL_ADD, chn->getFD(), &ev);
        if (rc) 
        {
            return false;
        }

        return true;
    }
    bool unwatch(channel* chn) {
        if (!chn) {
            return false;
        }
        int rc=epoll_ctl(_epfd, EPOLL_CTL_DEL, 
            chn->getFD(), NULL);
        if (rc) return false;
        return true;
    }
    bool run(void) {
        if (_epfd<=0 || _events==NULL) {
            return false;
        }
        while (!_exitNow) {
            int count=epoll_wait(_epfd, _events, MAXEVENTS, -1);
            for (int i=0;i<count;i++) {
                channel* chn=(channel*)_events[i].data.ptr;
                chn->channelInput();
                if (!chn->isActive()) {
                    unwatch(chn);
                }
            }
        }
        return true;
    }
    void stop(void) {_exitNow=true;}
};
#endif

