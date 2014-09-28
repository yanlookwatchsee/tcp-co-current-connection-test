#ifndef DEBUGGER_H
#define DEBUGGER_H
#include <iostream>
#include <fstream>
using namespace std;

enum debugLevel {
    NONE =0 ,
    TRACE,
    ERROR,
    FATAL
};
class debugger {
    debugLevel _level;
    ofstream _null;
    static debugger* _p;
    debugger():_level(NONE),_null("/dev/_null") {;}
    debugger(debugger& dbg) {;}
    debugger& operator=(debugger& dbg) {;}
public:
    inline void setLevel(debugLevel level) {_level=level;}
    ostream& logger(debugLevel level) {
        if (!level || level < _level) {
            return _null;
        }
        switch (level) {
        case TRACE:
            return cout;
            break;
        case ERROR:
            return cerr;
            break;
        case FATAL:
            return cerr;
            break;
        default:
            break;
        }
        return _null;
    }
    static debugger* createDebugger(void) {
        if (!_p) {
            _p=new debugger;
        }
        return _p;
    }
};
debugger* debugger:: _p=NULL;
#define DSETLEVEL(level) debugger::createDebugger()->setLevel(level)
#define DTRACE debugger::createDebugger()->logger(TRACE)
#define DERROR debugger::createDebugger()->logger(ERROR)
#define DFATAL debugger::createDebugger()->logger(FATAL)
#endif
