#include "xLog.h"
#include <map>
#include <sys/stat.h>
#include <unistd.h>
namespace XLog
{

std::string XLogFormat::out(const XLogEvent* event)
{
    if(this->vecFormatItem.empty())
    {
        return "XLogFormat error";
    }
    std::string out;
    for(auto it=vecFormatItem.begin();it != vecFormatItem.end(); ++it)
    {
        out += (*it)->out(event);
    }
    return out;
}

void XLogFormat::setFormat(std::string fmt)
{
    format = fmt;
    this->splite();
}

XLogFormat::XLogFormatItem* XLogFormat::create(const char fmt)
{
    if(fmt == 't') return new XLogFormatTime();
    else if(fmt == 'l') return new XLogFormatLevel();
    else if(fmt == 'd') return new XLogFormatThreadID();
    else if(fmt == 'f') return new XLogFormatFileName();
    else if(fmt == 'm') return new XLogFormatFuncName();
    else if(fmt == 'n') return new XLogFormatLine();
    else if(fmt == 'c') return new XLogFormatContent();
    return new XLogFormatError();
}

void XLogFormat::splite()
{
    if(format.empty())
    {
        return ;
    }
    for(auto it=vecFormatItem.begin();it != vecFormatItem.end(); ++it)
    {
        XLogFormatItem* item = (*it);
        if(item)
        {
            delete item;
            item = nullptr;
        }
    }
    this->vecFormatItem.clear();

    std::vector<char> vecStr;
    //%t %l %d %f %m %n %c
    int idx = 0;
    for(int i=0;i<format.size();i++)
    {
        if(format[i] != '%')
        {
            continue;
        }
        if(i+1 < format.size())
        {
            vecStr.push_back(format[i+1]);
            ++i;
        }
    }

    for(int i=0;i<vecStr.size();i++)
    {
        this->vecFormatItem.push_back(XLogFormat::create(vecStr[i]));
    }
}

XLogFormat::~XLogFormat()
{
    for(int i=0;i<vecFormatItem.size();i++)
    {
        if(vecFormatItem[i])
        {
            delete vecFormatItem[i];
            vecFormatItem[i] = nullptr;
        }
    }
}

std::string XLogFormat::XLogFormatTime::out(const XLogEvent* event)
{
    return XLogUtil::GetTimestampStr2() + " ";
}

std::string XLogFormat::XLogFormatLevel::out(const XLogEvent* event)
{
    return "[" + XLogFormat::XLogFormatLevel::strLevel(event->level) + "]";
}

std::string XLogFormat::XLogFormatThreadID::out(const XLogEvent* event)
{
    return " ThreadID:[" + std::to_string(event->dwThreadId) + "] ";
}

std::string XLogFormat::XLogFormatFileName::out(const XLogEvent* event)
{
    return std::string(event->strFileName) + "|";
}

std::string XLogFormat::XLogFormatFuncName::out(const XLogEvent* event)
{
    return std::string(event->strFuncName) + "|";
}

std::string XLogFormat::XLogFormatLine::out(const XLogEvent* event)
{
    return "Line:[" + std::to_string(event->dwLine) + "] ";
}

std::string XLogFormat::XLogFormatContent::out(const XLogEvent* event)
{
    return "out:" + event->strContent;
}

std::string XLogFormat::XLogFormatError::out(const XLogEvent* event)
{
    return "Format Error";
}

XLogOutputFile::XLogOutputFile(const char* fileName) : \
strFileName(fileName),\
strLogDir("log/"),
lastUpateLogFileTime(XLogUtil::GetCurTime()),
filePtr(NULL),
isInit(false)
{
    this->init();
}

XLogOutputFile::~XLogOutputFile()
{
    if(filePtr)
    {
        fflush(filePtr);
        fclose(filePtr);
    } 
}

void XLogOutputFile::out(const XLogEvent* event,XLogFormat* format)
{
    if(!isInit) return ;
    this->checkUpateFile();
    std::string str = format->out(event);
    fprintf(filePtr, "%s",str.c_str());
    fprintf(filePtr, "\n" );
}

bool XLogOutputFile::init()
{
    return isInit = this->open();
}

bool XLogOutputFile::open()
{
    if ( -1 == access( strLogDir.c_str(), F_OK ) )
	{
		mkdir( strLogDir.c_str(), 0755 );
		chmod( strLogDir.c_str(), 0755 );
	}
    std::string fullPath = strLogDir + std::string(strFileName);
    filePtr = fopen(fullPath.c_str(), "a+" );
    return filePtr == NULL ? false : true;
}
void XLogOutputFile::checkUpateFile()
{
	time_t cur_t = XLogUtil::GetCurTime();
	if ( !XLogUtil::IsSameDay( lastUpateLogFileTime, cur_t ) )
	{
		changeLogFile();
		lastUpateLogFileTime = cur_t;
	}
}
void XLogOutputFile::changeLogFile()
{
	if ( filePtr )
	{
		fflush( filePtr );
		fclose( filePtr );
	}
	std::string logname = strLogDir + std::string(strFileName);
	struct stat file_stat;
	if ( stat( logname.c_str(), &file_stat ) == 0 )	
	{
		std::string storefile = strLogDir + std::string(strFileName) + "." +  XLogUtil::GetTimestampStr( lastUpateLogFileTime );
        //重命名文件
        rename( logname.c_str(), storefile.c_str() );
	}
	this->open();
}

void XLogOutputConsole::out(const XLogEvent* event,XLogFormat* format)
{
    const std::string out = format->out(event);
    std::cout << out << std::endl;
}

void XLogger::addXLogOutput(XLogOutputLocationBase* output)
{
    this->vecOutLocat.push_back(output);
}

void XLogger::out(const XLogEvent* event)
{
    if(this->format == nullptr)
    {
        return ;
    }
    //log level ctrl out
    if(event->level >= this->level)
    {
        for(auto it=vecOutLocat.begin();it != vecOutLocat.end(); ++it)
        {
            (*it)->out(event,this->format);
        }    
    }
}

}