#ifndef _X_LOG_H
#define _X_LOG_H
#include <iostream>
#include <vector>
#include <memory>
#include <sys/syscall.h>
#include <stdarg.h>
#include <string.h>
using namespace std;
//support single thread write log

#define MAX_LOG_LEN 4096

#define XLOG_DEBUG(fmt, args...) \
                     XLOG_SYSTEM_FUNC(XLog::XLogLevel::XLOG_LEVEL_DEBUG,fmt, ##args)

#define XLOG_INFO(fmt, args...) \
                     XLOG_SYSTEM_FUNC(XLog::XLogLevel::XLOG_LEVEL_INFO,fmt, ##args)

#define XLOG_SYSTEM(fmt, args...) \
                     XLOG_SYSTEM_FUNC(XLog::XLogLevel::XLOG_LEVEL_SYSTEM,fmt, ##args)

#define XLOG_WARN(fmt, args...) \
                     XLOG_SYSTEM_FUNC(XLog::XLogLevel::XLOG_LEVEL_WARN,fmt, ##args)

#define XLOG_ERROR(fmt, args...) \
                     XLOG_SYSTEM_FUNC(XLog::XLogLevel::XLOG_LEVEL_ERROR,fmt, ##args)

#define XLOG_SYSTEM_FUNC(level,fmt,args...) \
                     {\
                        XLog::XLogEvent event(__FILE__,__LINE__,__FUNCTION__,\
                        level,\
                        XLog::XLogUtil::GetThreadID(),\
                        XLog::XLogUtil::XLOG_VA_LIST_STR(fmt, ##args));\
                        XLog::XLogger::getInstance()->out(&event);\
                     }
namespace XLog
{

class XLogLevel
{
public:
    XLogLevel():level(XLOG_LEVEL_DEBUG){}
public:
    enum Level
    {
        XLOG_LEVEL_DEBUG = 0,
        XLOG_LEVEL_INFO ,
        XLOG_LEVEL_SYSTEM,
        XLOG_LEVEL_WARN,
        XLOG_LEVEL_ERROR,
    };
    Level level;
};

/*
    const char* 	file,
    int 			line,
    const char* 	func,
    LogLevel 		level,
    uint32			id,
    const char* 	fmt,
    ...
*/
class XLogEvent
{
public:
    XLogEvent(const char* fileNmae,const uint32_t line,const char* funcName,const XLogLevel::Level level,const uint32_t threadId ,std::string content):
    strFileName(fileNmae),
    dwLine(line),
    strFuncName(funcName),
    level(level),
    dwThreadId(threadId),
    strContent(content)
    {}
public:
    const uint32_t dwTimeStamp = 0;
    const uint32_t dwThreadId;
    const uint32_t dwLine;
    const XLogLevel::Level level;
    const char* strFuncName;
    const char* strFileName;
    std::__sso_string strContent;
};
/*
    time level threadID fileName funcName line content
*/
class XLogFormat
{
public:
    // %t %l %d %f %m %n %c
    XLogFormat(std::string fmt) : format(fmt){
        this->splite();
    }
    ~XLogFormat();
public:
    void setFormat(std::string fmt);

    std::string out(const XLogEvent* event);

    class XLogFormatItem 
    {
    public:
        typedef std::shared_ptr<XLogFormatItem> XLogFormatItemPtr;
        virtual ~XLogFormatItem(){}
        virtual std::string out(const XLogEvent* event)=0;
    };

    class XLogFormatTime : public XLogFormatItem
    {
    public:
        std::string out(const XLogEvent* event);
    };

    class XLogFormatLevel : public XLogFormatItem
    {
    public:
        std::string out(const XLogEvent* event);
        static std::string strLevel(XLogLevel::Level level)
        {
            switch(level)
            {
                case XLogLevel::XLOG_LEVEL_DEBUG: return "DEBUG";
                case XLogLevel::XLOG_LEVEL_INFO:  return "INFO";
                case XLogLevel::XLOG_LEVEL_WARN:  return "WARN";
                case XLogLevel::XLOG_LEVEL_ERROR: return "ERROR";
                default:                          return "DEFAULT";
            }
        };
    };

    class XLogFormatThreadID : public XLogFormatItem
    {
    public:
        std::string out(const XLogEvent* event) override;
    };

    class XLogFormatFileName : public XLogFormatItem
    {
    public:
        std::string out(const XLogEvent* event) override;
    };

    class XLogFormatFuncName : public XLogFormatItem
    {
    public:
        std::string out(const XLogEvent* event) override;
    };

    class XLogFormatLine : public XLogFormatItem
    {
    public:
        std::string out(const XLogEvent* event) override;
    };

    class XLogFormatContent : public XLogFormatItem
    {
    public:
        std::string out(const XLogEvent* event) override;
    };

    class XLogFormatError : public XLogFormatItem
    {
    public:
        std::string out(const XLogEvent* event) override;
    };

    static XLogFormatItem* create(const char fmt);
private:
    void splite();
private:
    std::string format;
    typedef std::vector<XLogFormatItem*> XLogFormatItemContainer;
    XLogFormatItemContainer vecFormatItem;
};

class XLogOutputLocationBase
{
public:
    virtual ~XLogOutputLocationBase(){}
    virtual void out(const XLogEvent* event,XLogFormat* format)=0;
};

//use one day update file
class XLogOutputFile : public XLogOutputLocationBase
{
public:
    XLogOutputFile(const char* fileName);
    ~XLogOutputFile();
    void out(const XLogEvent* event,XLogFormat* format) override;
private:
    bool init();
    bool open();
    void checkUpateFile();
    void changeLogFile(); 
private:
    const char* strFileName;
    std::string strLogDir;
    uint32_t lastUpateLogFileTime;
    FILE* filePtr;
    bool isInit;
};

class XLogOutputConsole : public XLogOutputLocationBase
{
public:
    void out(const XLogEvent* event,XLogFormat* format) override;
};

class XLogger
{
public:
    static XLogger* getInstance()
    {
        static XLogger log;
        return &log;
    }
public:
    void addXLogOutput(XLogOutputLocationBase* output);
    void out(const XLogEvent* event);
private:
    XLogger():
    format(nullptr),
    level(XLogLevel::XLOG_LEVEL_DEBUG)
    {  
        //default create file and Console Output
        XLogOutputLocationBase* console = new XLogOutputConsole();
        vecOutLocat.push_back(console);

        XLogOutputLocationBase* file = new XLogOutputFile("xlog.log");
        vecOutLocat.push_back(file);

        //default create format
        if(format == nullptr)
        {
            format = new XLogFormat("%t %l %d %f %m %n %c");
        }
    }
    ~XLogger()
    {
        for(int i=0;i<vecOutLocat.size();i++)
        {
            if(vecOutLocat[i])
            {
                delete vecOutLocat[i];
                vecOutLocat[i] = nullptr;
            }
        }
        if(format)
        {
            delete format;
            format = nullptr;
        }
    }
    typedef std::vector<XLogOutputLocationBase*> XLogOutLocatContainer;
    XLogOutLocatContainer vecOutLocat;
    XLogFormat* format;
    XLogLevel::Level level;
};  

class XLogUtil
{
public:
    static uint32_t GetCurTime()
    {
        return time(NULL);
    }
    static std::string GetTimestampStr2()
    {
        time_t t = GetCurTime();
        tm stTime;
        localtime_r(&t, &stTime);
        tm* aTm = &stTime;
        //       YYYY   year
        //       MM     month (2 digits 01-12)
        //       DD     day (2 digits 01-31)
        //       HH     hour (2 digits 00-23)
        //       MM     minutes (2 digits 00-59)
        //       SS     seconds (2 digits 00-59)
        char buf[20];
        snprintf(buf, 20, "%04d-%02d-%02d %02d:%02d:%02d", aTm->tm_year+1900, aTm->tm_mon+1, aTm->tm_mday, aTm->tm_hour, aTm->tm_min, aTm->tm_sec);
        return std::string(buf);
    }
    static std::string GetTimestampStr(time_t t)
    {
        if ( t == 0 ) t = GetCurTime();
        tm stTime;
        localtime_r(&t, &stTime);
        tm* aTm = &stTime;
        char buf[20];
        snprintf(buf, 20, "%04d-%02d-%02d_%02d-%02d-%02d", aTm->tm_year+1900, aTm->tm_mon+1, aTm->tm_mday, aTm->tm_hour, aTm->tm_min, aTm->tm_sec);
        return std::string(buf);
    }
    static uint32_t GetThreadID()
    {
        return pthread_self();
    }

    static std::string XLOG_VA_LIST_STR(const char* fmt,...)
    {
        char buff[MAX_LOG_LEN] = {0};
        va_list ap;
        va_start(ap, fmt);
        vsnprintf( buff, MAX_LOG_LEN, fmt, ap);
        va_end(ap);
        return buff;
    }

    static bool IsSameDay( uint32_t dwTime1, uint32_t dwTime2 )
    {
        time_t t1 = dwTime1;
        time_t t2 = dwTime2;
        struct tm stcT1;
        struct tm stcT2;
        localtime_r( &t1, &stcT1 );
        localtime_r( &t2, &stcT2 );
        if ( stcT1.tm_year == stcT2.tm_year && stcT1.tm_yday == stcT2.tm_yday )
        {
            return true;
        }
        return false;
    }
};

}

#endif