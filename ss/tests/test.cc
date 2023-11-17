#include <iostream>
#include <thread>
#include "../ss/log.h"
#include "../ss/util.h"

int main(int argc, char** argv) {
	ss::Logger::ptr logger(new ss::Logger);
	logger->addAppender(ss::LogAppender::ptr(new ss::StdoutLogAppender));

	//ss::LogEvent::ptr event(new ss::LogEvent(__FILE__, __LINE__, 0, ss::GetThreadId(), ss::GetFiberId(), time(0)));
	//event->getSS() << "hello log";

	//logger->log(ss::LogLevel::DEBUG, event);
	std::cout << "hello" << std::endl;

	SS_LOG_INFO(logger) << "test" ;
	SS_LOG_ERROR(logger) << "test error" ;
	return 0;
}


