#include <iostream>
#include <thread>
#include "../zy/log.h"
#include "../zy/util.h"

int main(int argc, char** argv) {
	zy::Logger::ptr logger(new zy::Logger);
	logger->addAppender(zy::LogAppender::ptr(new zy::StdoutLogAppender));

    zy::FileLogAppender::ptr file_appender(new zy::FileLogAppender("./log.txt"));
    zy::LogFormatter::ptr fmt(new zy::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);

	//设置只有ERROR级别的日志
	file_appender->setLevel(zy::LogLevel::ERROR);
	logger->addAppender(file_appender);
    //file_appender->setLevel(zy::LogLevel::ERROR);
	//zy::LogEvent::ptr event(new zy::LogEvent(__FILE__, __LINE__, 0, zy::GetThreadId(), zy::GetFiberId(), time(0)));
	//event->getZY() << "hello log";

	//logger->log(zy::LogLevel::DEBUG, event);
	std::cout << "hello" << std::endl;

	ZY_LOG_INFO(logger) << "test";
	ZY_LOG_ERROR(logger) << "test error";
	ZY_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
	auto l = zy::LoggerMgr::GetInstance()->getLogger("xx");
	ZY_LOG_INFO(l) << "xxx";
	return 0;
}


