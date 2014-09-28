#include "common.h"
#include "eventLooper.h"
#include <list>
#include "stdlib.h"
#include "string.h"
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define TARGET_SCALE 10*1024
bool gExit=false;
eventLooper gEL(TARGET_SCALE);
#define MAX_BUFFER 1024*1024
char gMsg[MAX_BUFFER]={0};
void stopLooper(int sig) {
    DERROR<<"sigint catch! stop looper!"<<std::endl;
    gEL.stop();
    gExit=true;
}
void usage(void) {
    std::cout<<"cctest < -n numConn(max=40960)> <-x speed [1,256] > "
        "<-i remoteIP> <-p remotePort>"<<std::endl;
}
struct stats {
    unsigned long int bytesReceived;
    unsigned long int bytesSent;
    time_t startTime;
    time_t stopTime;
    stats () {
        bytesReceived = 0;
        bytesSent = 0;
    }
    void startCounting(void) {startTime=time(NULL);}
    void stopCounting(void) {stopTime=time(NULL);}
    void report(void) {
        time_t duration = stopTime-startTime;
        DERROR << "recv bytes = "<<bytesReceived<<std::endl;
        DERROR << "send bytes = "<<bytesSent<<std::endl;
        if (duration!=0) {
            DERROR << "upstream speed = "<<(bytesSent*8/duration/1024/1024)
                <<" Mbps"
                <<std::endl;
            DERROR << "downstream speed = "<<(bytesReceived*8/duration/1024/1024)
                <<" Mbps"
                <<std::endl;
        }
    } 
};
stats gStats;
struct config {
    static const int MAX_IPV4_SIZE=16;
    static const int MAX_PORT_SIZE=16;
    static const int MAX_SPEED=256;
    static const int MAX_CONNECTION=40960;
    char remoteIP[MAX_IPV4_SIZE];
    char remotePort[MAX_PORT_SIZE];
    int speed;
    int numConn;
    bool checkDigit(const char* target) {
        for (int i=0;target[i]!=0;i++) {
            if (target[i]>'9' || target[i]<'0') {
                return false;
            }
        }
        return true;
    }
    bool checkIP (const char* target) {
        int dot=0, sec=0;
        for (int i=0;target[i]!=0;i++) {
            if ((target[i]!='.'&&target[i]>'9')
                || (target[i]!='.'&&target[i]<'0')) {
                return false;
            }
            if (target[i]=='.' && (sec>3 || sec==0)) {
                return false;
            } 
            if (target[i]=='.') {
                dot++;
                sec=0;
            } else {
                sec++;
            }
        }
        if (sec>3 || dot!=3) return false;
        return true;
    }
} gConfig;


bool loadConfig(int argc, char* argv[]) {
    if (argc!=9) {
        return false;
    }
    int opt=0;
    while((opt=getopt(argc, argv, "n:x:i:p:"))!=-1) {
        switch(opt) {
        case 'n':
            if (!gConfig.checkDigit(optarg)) return false;
            gConfig.numConn=atoi(optarg);
            if (gConfig.numConn > gConfig.MAX_CONNECTION) {
                return false;
            }
            break;

        case 'x':
            if (!gConfig.checkDigit(optarg)) return false;
            gConfig.speed=atoi(optarg);
            if (gConfig.speed>gConfig.MAX_SPEED 
                || gConfig.speed<1) {
                return false;
            }
            break;
        case 'i':
            if (!gConfig.checkIP(optarg)) return false;
            snprintf(gConfig.remoteIP, gConfig.MAX_IPV4_SIZE,"%s",  optarg);
            break;
        case 'p':
            if (!gConfig.checkDigit(optarg)) return false;
            snprintf(gConfig.remotePort, gConfig.MAX_PORT_SIZE,"%s",  optarg);
            break;
        default:
            return false;
        }
    }
    return true;
}
class cctester: public observer {
    int _count;
    int _maxCount;
    eventLooper* _refEL;
#define FACTOR 1
public:
    cctester(int maxCount):_count(0),_maxCount(maxCount),
        _refEL(&gEL) {;}
    
    virtual void notifyData(channel* ch, 
        const char* buf, const int len) {
            debug->logger(ERROR)<<"cnt: "<<_count++<<" "<<len<<" bytes received!"<<'\r';
            gStats.bytesReceived+=len;
            if (!ch->channelOutput(gMsg, gConfig.speed)) {
                debug->logger(ERROR)<<"send error!"<<endl;
            } else {
                gStats.bytesSent+=gConfig.speed;
            }
            return;
    }
    virtual void notifySetup(channel* ch) {
        //return;//test
        debug->logger(TRACE)<<"connection setup!"<<endl;
        if (!ch->channelOutput(gMsg, gConfig.speed)) {
            debug->logger(ERROR)<<"send error!"<<endl;
        } else {
            gStats.bytesSent+=gConfig.speed;
        }
        return;
    }

};
int main(int argc, char* argv[]) {
    if (!loadConfig(argc, argv)) {
        usage();
        return 1;
    }
    cout<<"remoteIP="<<gConfig.remoteIP<<endl;
    cout<<"remotePort="<<gConfig.remotePort<<endl;
    cout<<"numConn="<<gConfig.numConn<<endl;
    gConfig.speed*=1024;
    cout<<"speed="<<gConfig.speed<<endl;

    for (int i=0;i<gConfig.speed;i++) {
        gMsg[i]='A';
    }
    debug->setLevel(ERROR);
    list<channel*> channelList;
    cctester ob(gConfig.numConn);
    //signal reg
    signal(SIGINT, stopLooper);
    //load channel to event looper
    time_t now = time(NULL);
    for (int i=0;i<gConfig.numConn;i++) {
        channel* ch=new channel;
        ch->attachOB(&ob);
        if (!ch->activate(gConfig.remoteIP, gConfig.remotePort)) {
            debug->logger(ERROR)<<"connect error!"<<endl;
        }
        cerr<<i<<" connect finshed ..."<<'\r';
        gEL.watch(ch);
        channelList.push_back(ch);
    }
    int period = time(NULL)-now;
    if (period>0) {
        debug->logger(ERROR)<<"connect done! ramp up rate="<<(gConfig.numConn/period);
    } else {
        debug->logger(ERROR)<<"connect done! ramp up rate=OO";
    }
    debug->logger(ERROR)<<"\nTransfer begin!"<<endl;
    //eventlooper run
    gStats.startCounting();


    gEL.run();
    gStats.stopCounting();
    gStats.report();
    
    //close the channels
    for (;!channelList.empty();) {
        channel* ch=channelList.front();
        if (ch) delete ch;
        channelList.pop_front();
    }
    
    cout<<"test done"<<endl;
    return 0;
}

