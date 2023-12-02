#include "log.h"
#include <map>
#include <iostream>
#include <functional>
#include "config.h"
#include <time.h>
#include <string.h>

namespace zy {

const char* LogLevel::Tostring(LogLevel::Level level) {
	switch (level) {
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
	default:
		return "UNKNOW";
	}
	return "UNKNOW";
}//switch的宏定义写法

LogEventWrap::LogEventWrap(LogEvent::ptr e)
	:m_event(e) {

}

LogEventWrap::~LogEventWrap() {
	m_event->getLogger()->log(m_event->getLevel(), m_event);
}

//va_list #include <stdarg.h>注意用法
void LogEvent::format(const char* fmt, ...) {
	va_list al;
	va_start(al, fmt);
	format(fmt, al);
	va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

std::stringstream& LogEventWrap::getSS() {
	return m_event->getSS();
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
	MessageFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getContent();
	}
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
	LevelFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << LogLevel::Tostring(level);
	}
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
	ElapseFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getElapse();
	}
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
	NameFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getLogger()->getName();
	}
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
	ThreadIdFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getThreadId();
	}
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
 	FiberIdFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getFiberId();
	}
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
	DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%S") 
		:m_format(format) {
		if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
		}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
	}
	
private:
	std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
	FilenameFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getFile();
	}
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
	LineFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << event->getLine();
	}
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
	NewLineFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << std::endl;
	}
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
	StringFormatItem(const std::string& str)
		:m_string(str) {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << m_string;
	}
private:
	std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
	TabFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override{
		os << "\t";
	}
private:
	std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(thread_id)
    ,m_fiberId(fiber_id)
    ,m_time(time)
	,m_logger(logger)
	,m_level(level) {
}

Logger::Logger(const std::string& name) 
	:m_name(name)
	,m_level(LogLevel::DEBUG) {
	m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
	if (!appender->getFormatter()) {
		appender->setFormatter(m_formatter);
	}
	m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
	for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
		if (*it == appender) {
			m_appenders.erase(it);
			break;
		} 		
	}
}

void Logger::log(LogLevel::Level level, const LogEvent::ptr event) {
    if(level >= m_level) {
        auto self = shared_from_this();
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders) {
                i->log(self, level, event);
            }
        } else if(m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) {
	log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
	log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event) {
	log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event) {
	log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event) {
	log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
	:m_filename(filename) {
	reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	if (level >= m_level) {
		std::string str = m_formatter->format(logger, level, event);
		m_filestream << str;
		//std::cout << str << std::endl;
	}
}

bool FileLogAppender::reopen() {
	if (m_filestream) {
		m_filestream.close();
	}
	//改变为追加模式
	m_filestream.open(m_filename, std::ios::app);
	return !!m_filestream;//双感叹号转为0/1
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr event) {
	if (level >= m_level) {
		std::cout << m_formatter->format(logger, level, event);
	}
}

LogFormatter::LogFormatter(const std::string& pattern)
	:m_pattern(pattern) {
		init();
}
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	std::stringstream ss;
	for (auto& i : m_items) {
		i->format(ss, logger, level, event);
	}
	return ss.str();
}
std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    for(auto& i : m_items) {
        i->format(ofs, logger, level, event);
    }
    return ofs;
}
//实现日志初始化格式，用户自定义
//合法格式:%xxx %xxx{xxx}  %%
void LogFormatter::init() {
	//str, format, type
	std::vector<std::tuple<std::string, std::string, int>> vec;
	std::string nstr;
	//在选择使用std::size_t还是int时，需要考虑具体的应用场景和要处理的数据类型。如果需要表示对象的大小或索引，通常会使用std::size_t，以确保能够容纳足够大的值。如果只需要一般的整数类型，可以使用int。当涉及到数组的大小或索引时，通常会使用std::size_t。
	for (size_t i = 0; i < m_pattern.size(); i++) {
		if (m_pattern[i] != '%') {
			nstr.append(1, m_pattern[i]);
			continue;
		}

		if ((i + 1) < m_pattern.size()) {
			if (m_pattern[i + 1] == '%') {
				nstr.append(1, '%');
				continue;
			}
		}

		size_t n = i + 1;
		int fmt_status = 0;
		size_t fmt_begin = 0;

		std::string fmt;
		std::string str;
		while (n < m_pattern.size()) {
			if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
				str = m_pattern.substr(i + 1, n - i - 1);
				break;
			}
			if (fmt_status == 0) {
				if (m_pattern[n] == '{') {
					str = m_pattern.substr(i + 1, n - i - 1);
					//std::cout << "*" << str << std::endl;
					fmt_status = 1;//解析格式
					fmt_begin = n;
					++n;
					continue;
				}
			}
			if (fmt_status == 1) {
				if (m_pattern[n] == '}') {
					fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
					//std::cout << "#" << str << std::endl;
					fmt_status = 0;
					++n;
					break;
				}
			}
			++n;
			if (n == m_pattern.size()) {
				if (str.empty()) {
					str = m_pattern.substr(i + 1);
				}
			}
		}
 		if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            //m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

	if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}
		//%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, NameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FilenameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:Tab
        XX(F, FiberIdFormatItem),           //F:协程id
        //XX(N, ThreadNameFormatItem),        //N:线程名称
#undef XX
    };

	for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                //m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    //m_loggers[m_root->m_name] = m_root;

    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type = 0; //1 File, 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }

    bool isValid() const {
        return !name.empty();
    }
};

zy::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
    zy::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        // g_log_defines->addListener([](const std::set<LogDefine>& old_value,
        //             const std::set<LogDefine>& new_value){
        //     ZY_LOG_INFO(ZY_LOG_ROOT()) << "on_logger_conf_changed";
        //     for(auto& i : new_value) {
        //         auto it = old_value.find(i);
        //         zy::Logger::ptr logger;
        //         if(it == old_value.end()) {
        //             //新增logger
        //             logger = ZY_LOG_NAME(i.name);
        //         } else {
        //             if(!(i == *it)) {
        //                 //修改的logger
        //                 logger = ZY_LOG_NAME(i.name);
        //             } else {
        //                 continue;
        //             }
        //         }
        //         logger->setLevel(i.level);
        //         //std::cout << "** " << i.name << " level=" << i.level
        //         //<< "  " << logger << std::endl;
        //         if(!i.formatter.empty()) {
        //             logger->setFormatter(i.formatter);
        //         }

        //         logger->clearAppenders();
        //         for(auto& a : i.appenders) {
        //             zy::LogAppender::ptr ap;
        //             if(a.type == 1) {
        //                 ap.reset(new FileLogAppender(a.file));
        //             } else if(a.type == 2) {
        //                 if(!zy::EnvMgr::GetInstance()->has("d")) {
        //                     ap.reset(new StdoutLogAppender);
        //                 } else {
        //                     continue;
        //                 }
        //             }
        //             ap->setLevel(a.level);
        //             if(!a.formatter.empty()) {
        //                 LogFormatter::ptr fmt(new LogFormatter(a.formatter));
        //                 if(!fmt->isError()) {
        //                     ap->setFormatter(fmt);
        //                 } else {
        //                     std::cout << "log.name=" << i.name << " appender type=" << a.type
        //                               << " formatter=" << a.formatter << " is invalid" << std::endl;
        //                 }
        //             }
        //             logger->addAppender(ap);
        //         }
        //     }

        //     for(auto& i : old_value) {
        //         auto it = new_value.find(i);
        //         if(it == new_value.end()) {
        //             //删除logger
        //             auto logger = ZY_LOG_NAME(i.name);
        //             logger->setLevel((LogLevel::Level)0);
        //             logger->clearAppenders();
        //         }
        //     }
        // });
    }
};


static LogIniter __log_init;

void LoggerManager::init() {
}
}


