#ifndef __ZY_LOG_H__ //防止重定义ss
#define __ZY_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"

#define ZY_LOG_LEVEL(logger, level) \
	if (logger->getLevel() <= level) \
		zy::LogEventWrap(zy::LogEvent::ptr(new zy::LogEvent(logger, level, \
			__FILE__, __LINE__, 0,zy::GetThreadId(), \
			zy::GetFiberId(), time(0)))).getSS()

#define ZY_LOG_DEBUG(logger) ZY_LOG_LEVEL(logger, zy::LogLevel::DEBUG)
#define ZY_LOG_INFO(logger) ZY_LOG_LEVEL(logger, zy::LogLevel::INFO)
#define ZY_LOG_WARN(logger) ZY_LOG_LEVEL(logger, zy::LogLevel::WARN)
#define ZY_LOG_ERROR(logger) ZY_LOG_LEVEL(logger, zy::LogLevel::ERROR)
#define ZY_LOG_FATAL(logger) ZY_LOG_LEVEL(logger, zy::LogLevel::FATAL)

#define ZY_LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level) \
        zy::LogEventWrap(zy::LogEvent::ptr(new zy::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, zy::GetThreadId(),\
                zy::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define ZY_LOG_FMT_DEBUG(logger, fmt, ...) ZY_LOG_FMT_LEVEL(logger, zy::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define ZY_LOG_FMT_INFO(logger, fmt, ...)  ZY_LOG_FMT_LEVEL(logger, zy::LogLevel::INFO, fmt, __VA_ARGS__)
#define ZY_LOG_FMT_WARN(logger, fmt, ...)  ZY_LOG_FMT_LEVEL(logger, zy::LogLevel::WARN, fmt, __VA_ARGS__)
#define ZY_LOG_FMT_ERROR(logger, fmt, ...) ZY_LOG_FMT_LEVEL(logger, zy::LogLevel::ERROR, fmt, __VA_ARGS__)
#define ZY_LOG_FMT_FATAL(logger, fmt, ...) ZY_LOG_FMT_LEVEL(logger, zy::LogLevel::FATAL, fmt, __VA_ARGS__)

#define ZY_LOG_ROOT() zy::LoggerMgr::GetInstance()->getRoot()
#define ZY_LOG_NAME(name) zy::LoggerMgr::GetInstance()->getLogger(name)

namespace zy {


class Logger;
class LoggerManager;
//日志级别
class LogLevel {
public:
	enum Level {
		UNKNOW = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5
	};	

	static const char* ToString(LogLevel::Level level);
	static LogLevel::Level FromString(const std::string& str);
};

//日志事件
class LogEvent {
public:
	typedef std::shared_ptr<LogEvent> ptr;//定义智能指针，支持自动回收，方便内存管理
	LogEvent(std::shared_ptr<Logger> m_logger, LogLevel::Level level, const char* file, int32_t m_line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time);
	
	const char* getFile() const { return m_file;}
	int32_t getLine() const { return m_line;}
	uint32_t getElapse() const { return m_elapse;}
	uint32_t getThreadId() const { return m_threadId;}
	uint32_t getFiberId() const { return m_fiberId;}
	uint64_t getTime() const { return m_time;}
	std::string getContent() const { return m_ss.str();}
	std::shared_ptr<Logger> getLogger() const{ return m_logger;}
	LogLevel::Level getLevel() const { return m_level;}

	std::stringstream& getSS() { return m_ss;}
	void format(const char* fmt, ...);
	void format(const char* fmt, va_list al);

private:
	const char* m_file = nullptr;//文件名
	int32_t m_line = 0;//行号
	uint32_t m_elapse = 0;//程序启动开始到现在的毫秒数
	uint32_t m_threadId = 0;//线程库id
	uint32_t m_fiberId = 0;//协程库id
	uint64_t m_time;//时间戳
	std::stringstream m_ss;

	std::shared_ptr<Logger> m_logger;
	LogLevel::Level m_level;
};

class LogEventWrap {
public:
	LogEventWrap(LogEvent::ptr e);
	~LogEventWrap();
	std::stringstream& getSS();
	LogEvent::ptr getEvent() const { return m_event;}
private:
	LogEvent::ptr m_event;
};



//日志格式器
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
	LogFormatter(const std::string& pattern);

	//%t  %thread_id %message%number
	std::string format (std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
	std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
	class FormatItem {
	public:
		typedef std::shared_ptr<FormatItem> ptr;
		virtual ~FormatItem() {}
		virtual void format (std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
		//直接输出到流中，可以多个组合提高性能
	};
	void init();//做item的解析	

	bool isError() const { return m_error;}

	const std::string getPattern() const { return m_pattern;}
private:
	std::string m_pattern;
	std::vector<FormatItem::ptr> m_items;
	bool m_error = false;
};

//日志输出地
class LogAppender {
friend class Logger;
public:
	typedef std::shared_ptr<LogAppender> ptr;//定义智能指针，支持自动回收，方便内存管理
	typedef Mutex MutexType;
    virtual ~LogAppender() {}

	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) = 0;//纯虚函数必须要有实现的子类	
	virtual std::string toYamlString() = 0;

	void setFormatter(LogFormatter::ptr val);
	LogFormatter::ptr getFormatter();

	LogLevel::Level getLevel() const { return m_level;}
	void setLevel(LogLevel::Level val) { m_level = val;}
protected:
	LogLevel::Level m_level = LogLevel::DEBUG;
	LogFormatter::ptr m_formatter;
	MutexType m_mutex;
    bool m_hasFormatter = false;// 是否有自己的日志格式器
};

//日志器
class Logger : public std::enable_shared_from_this<Logger>{
friend class LoggerManager;
public:
	typedef std::shared_ptr<Logger> ptr;
	typedef Mutex MutexType;
		
	Logger(const std::string& name = "root");
	void log(LogLevel::Level level, const LogEvent::ptr event);
	
	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);
	
	void addAppender(LogAppender::ptr appender);
	void delAppender(LogAppender::ptr appender);
	void clearAppenders();
	LogLevel::Level getLevel() const { return m_level;}
	void setLevel (LogLevel::Level val) { m_level = val;}

	const std::string& getName() const { return m_name;}

	void setFormatter(LogFormatter::ptr val);
	void setFormatter(const std::string& val);
	LogFormatter::ptr getFormatter();

	std::string toYamlString();
private:
	std::string m_name;
	std::list<LogAppender::ptr> m_appenders; //Appender集合，用列表存储(list是双向链表)
	LogLevel::Level m_level;//日志级别
	LogFormatter::ptr m_formatter;
	Logger::ptr m_root;
	MutexType m_mutex;
};



//输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<StdoutLogAppender> ptr;
	void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
	std::string toYamlString() override;
// private:
// 	std::string m_filename;
// 	std::ofstream m_filestream;
};


//定义输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<FileLogAppender> ptr;
	FileLogAppender(const std::string& filename);
	void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
	std::string toYamlString() override;
	bool reopen();//重新打开文件，文件打开成功返回true
private:
	std::string m_filename;
	std::ofstream m_filestream;
};

class LoggerManager {
public:
	typedef Mutex MutexType;
	LoggerManager();
	Logger::ptr getLogger(const std::string& name);
	
	void init();
	Logger::ptr getRoot() const { return m_root;}
	std::string toYamlString();
private:
	MutexType m_mutex;
	std::map<std::string, Logger::ptr> m_loggers;
	Logger::ptr m_root;
};

typedef zy::Singleton<LoggerManager> LoggerMgr;

};



#endif

