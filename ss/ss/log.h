#ifndef __SS_LOG_H__ //防止重定义
#define __SS_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

namespace ss {


class Logger;
//日志事件
class LogEvent {
public:
	typedef std::shared_ptr<LogEvent> ptr;//定义智能指针，支持自动回收，方便内存管理
	LogEvent(const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time);

	const char* getFile() const { return m_file;}
	int32_t getLine() const { return m_line;}
	uint32_t getElapse() const { return m_elapse;}
	uint32_t getThreadId() const { return m_threadId;}
	uint32_t getFiberId() const { return m_fiberId;}
	uint64_t getTime() const { return m_time;}
	std::string getContent() const { return m_ss.str();}
	std::stringstream& getSS() { return m_ss;}
private:
	const char* m_file = nullptr;//文件名
	int32_t m_line = 0;//行号
	uint32_t m_elapse = 0;//程序启动开始到现在的毫秒数
	uint32_t m_threadId = 0;//线程库id
	uint32_t m_fiberId = 0;//协程库id
	uint64_t m_time;//时间戳
	std::stringstream m_ss;
};

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

	static const char* Tostring(LogLevel::Level level);//静态成员函数
};

//日志格式器
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
	LogFormatter(const std::string& pattern);

	//%t  %thread_id %message%number
	std::string format (std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
	class FormatItem {
	public:
		typedef std::shared_ptr<FormatItem> ptr;
		virtual ~FormatItem() {}
		virtual void format (std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
		//直接输出到流中，可以多个组合提高性能
	};
	void init();//做item的解析	
private:
	std::string m_pattern;
	std::vector<FormatItem::ptr> m_items;
};


//日志输出地
class LogAppender {
public:
	typedef std::shared_ptr<LogAppender> ptr;//定义智能指针，支持自动回收，方便内存管理
    virtual ~LogAppender() {}

	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) = 0;//纯虚函数必须要有实现的子类	
	
	void setFormatter(LogFormatter::ptr val) { m_formatter = val;}
	LogFormatter::ptr getFormatter() const { return m_formatter;}
protected:
	LogLevel::Level m_level = LogLevel::DEBUG;//针对哪些LEVEL作出记录(?)protected让子类可以使用到
	LogFormatter::ptr m_formatter;
};



//日志器
class Logger : public std::enable_shared_from_this<Logger>{
public:
	typedef std::shared_ptr<Logger> ptr;
		
	Logger(const std::string& name = "root");
	void log(LogLevel::Level level, const LogEvent::ptr event);
	
	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);
	
	void addAppender(LogAppender::ptr appender);
	void delAppender(LogAppender::ptr appender);
	LogLevel::Level getLevel() const { return m_level;}
	void setLevel (LogLevel::Level val) { m_level = val;}

	const std::string& getName() const { return m_name;}
private:
	std::string m_name;
	std::list<LogAppender::ptr> m_appenders; //Appender集合，用列表存储(list是双向链表)
	LogLevel::Level m_level;//日志级别
	LogFormatter::ptr m_formatter;
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<StdoutLogAppender> ptr;
	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
private:
	std::string m_filename;
	std::ofstream m_filestream;
};


//定义输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<StdoutLogAppender> ptr;
	FileLogAppender(const std::string& filename);
	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
	
	bool reopen();//重新打开文件，文件打开成功返回true
private:
	std::string m_filename;
	std::ofstream m_filestream;
};

};



#endif

