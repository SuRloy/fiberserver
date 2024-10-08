#ifndef __ZY_TCP_SERVER_H__
#define __ZY_TCP_SERVER_H__

#include <memory>
#include "reactor.h"
#include "socket.h"
#include "utils/noncopyable.h"

namespace zy {
    /**
     * @brief TCP 服务器
     */
    class TCPServer : public std::enable_shared_from_this<TCPServer>, NonCopyable {
    public:
        using ptr = std::shared_ptr<TCPServer>;

        /**
         * @brief 构造函数
         * @param name TCP服务器名称
         * @param acceptor acceptor 反应堆
         * @param worker worker 反应堆
         */
        explicit TCPServer(std::string name, Reactor *acceptor = Reactor::GetThis(), Reactor *worker = Reactor::GetThis());

        /**
         * @brief 虚析构函数
         */
        virtual ~TCPServer();

        /**
         * @brief 绑定服务器地址
         * @param address 服务器地址
         * @return 操作是否成功
         */
        bool bind(const Address::ptr &address);

        /**
         * @brief 启动服务器
         */
        void start();

        /**
         * @brief 关闭服务器
         */
        void stop();

        // region # Getter
        static uint64_t getRecvTimeout() { return s_recv_timeout; }

        static uint64_t getSendTimeout() { return s_send_timeout; }

        const std::string &getName() const { return name_; }

        bool isStop() const { return stop_; }

        Reactor* getWorker() { return worker_;}

        bool stopState() { return stop_;}

        Socket::ptr getListenSock() { return sock_;}
        // endregion

        
    protected:
        /**
         * @brief 处理新的客户端连接
         */
        virtual void handleAccept();

        /**
         * @brief 处理客户端连接
         * @param client 需要处理的客户端连接
         */
        virtual void handleClient(const Socket::ptr &client);

    private:
        /// 接收超时时间
        static const uint64_t s_recv_timeout = 1000 * 2 * 60;
        /// 发送超时时间
        static const uint64_t s_send_timeout = 1000 * 1 * 60;
    private:
        /// TCP 服务器名称
        std::string name_;
        /// acceptor，负责处理新的客户端连接
        Reactor *acceptor_;
        /// worker，负责处理客户端连接
        Reactor *worker_;
        /// 监听 socket
        Socket::ptr sock_;
        /// 服务器是否停止
        bool stop_;
    };
}

#endif //ZY_TCP_SERVER_H
