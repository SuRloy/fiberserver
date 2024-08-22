#include "tcp_server.h"

#include <cstring>
#include "log.h"
#include "utils/macro.h"

namespace zy {
    TCPServer::TCPServer(std::string name, Reactor *acceptor, Reactor *worker)
        : name_(std::move(name)), acceptor_(acceptor), worker_(worker), stop_(false) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "create a new tcp server, name = " << getName();
    }

    TCPServer::~TCPServer() {
        if (!stop_) {
            stop();
        }
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "tcp server stop, name = " << getName();
    }

    bool TCPServer::bind(const Address::ptr &address) {
        sock_ = Socket::CreateTCP(address->getFamily());
        if (!sock_->bind(address)) {
            ZY_LOG_ERROR(ZY_LOG_ROOT()) << "bind filed errno=" << errno
                                            << " errstr=" << strerror(errno)
                                            << "addr=[" << address->toString() << "";
            return false;
        }
        if (!sock_->listen()) {
            ZY_LOG_ERROR(ZY_LOG_ROOT()) << "listen filed errno=" << errno
                                            << " errstr=" << strerror(errno)
                                            << "addr=[" << address->toString() << "";
            return false;
        }
        return true;
    }

    void TCPServer::start() {
        ZY_ASSERT(!stop_);
        acceptor_->addTask(std::bind(&TCPServer::handleAccept, shared_from_this()));
    }

    void TCPServer::stop() {
        stop_ = true;
        acceptor_->addTask([this](){
            sock_->cancelRead();
            sock_->close();
        });
    }

    void TCPServer::handleAccept() {
        while (!stop_) {
            Socket::ptr client = sock_->accept();
            if (client) {
                client->setRecvTimeout(s_recv_timeout);
                client->setSendTimeout(s_send_timeout);
                worker_->addTask(std::bind(&TCPServer::handleClient, shared_from_this(), client));
            } else {
                ZY_LOG_ERROR(ZY_LOG_ROOT()) << "accept errno = " << errno
                                                 << " errstr = " << strerror(errno);
            }
        }
    }

    void TCPServer::handleClient(const Socket::ptr &client) {
        ZY_LOG_INFO(ZY_LOG_ROOT()) << "handleClient" << client->toString();
    }
}