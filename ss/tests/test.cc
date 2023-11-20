#include <iostream>
#include <thread>
#include "../ss/log.h"
#include "../ss/util.h"

int main(int argc, char** argv) {
	ss::Logger::ptr logger(new ss::Logger);
	logger->addAppender(ss::LogAppender::ptr(new ss::StdoutLogAppender));

    ss::FileLogAppender::ptr file_appender(new ss::FileLogAppender("./log.txt"));
    ss::LogFormatter::ptr fmt(new ss::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    //file_appender->setLevel(ss::LogLevel::ERROR);
	//ss::LogEvent::ptr event(new ss::LogEvent(__FILE__, __LINE__, 0, ss::GetThreadId(), ss::GetFiberId(), time(0)));
	//event->getSS() << "hello log";

	//logger->log(ss::LogLevel::DEBUG, event);
	std::cout << "hello" << std::endl;

	SS_LOG_INFO(logger) << "test" ;
	SS_LOG_ERROR(logger) << "test error" ;
	SS_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
	return 0;
}


