#include <iostream>
#include "../ss/log.h"

int main(int argc, char** argv) {
	ss::Logger::ptr logger(new ss::Logger);
	logger->addAppender(ss::LogAppender::ptr(new ss::StdoutLogAppender));

	ss::LogEvent::ptr event(new ss::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0)));
	event->getSS() << "hello log";

	logger->log(ss::LogLevel::DEBUG, event);
	std::cout << "hello" << std::endl;
	return 0;
}

