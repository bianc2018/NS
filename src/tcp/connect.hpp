/*
    连接,负责连接的创建关闭和数据的读写
*/

#ifndef CONNECT_HPP_
#define CONNECT_HPP_

#include <string>
#include <thread>
#include "../util/event.hpp"
#include "../util/util.hpp"

namespace ns
{
    namespace net
    {
        enum NSSocketType
        {
            NS_SOCK_STREAM = SOCK_STREAM,
            NS_SOCK_DGRAM = SOCK_DGRAM,
            NS_SOCK_RAM = SOCK_RAW,
            NS_SOCK_RDM = SOCK_RDM,
            NS_SOCK_SEQPACKET = SOCK_SEQPACKET,
        };
        enum NSSocketProtocol
        {
            NS_IPPROTO_TCP = IPPROTO_TCP,
            NS_IPPROTO_ICMP = IPPROTO_ICMP,
            NS_IPPROTO_UDP = IPPROTO_UDP,
        };
        //连接，每一个对象实例代表一个连接实例
        class NetConnect
        {
        public:
            explicit NetConnect(NSSocketType type= NS_SOCK_STREAM, NSSocketProtocol protocol= NS_IPPROTO_TCP)
            {
                sock_ = socket(AF_INET, type, protocol);
            }
            explicit NetConnect(SOCKET sock) :sock_(sock)
            {

            }
            size_t send(const std::string& data)
            {
                return ::send(sock_, data.c_str(), data.size(), 0);
            }
            void send(const std::string& data, std::function<void(size_t size)>func)
            {
                NS_EVENT_ASYNC_VOID([func, this](const std::string& data) {
                    auto size = send(data);
                    func(size);
                    }, data);
            }
            int recv(char* buff, int buff_len)
            {
                return ::recv(sock_, buff, buff_len, 0);
            }

            int recv(ns_shared_ptr<char> buff, int buff_len, \
                std::function<void(ns_shared_ptr<char> buff, size_t size)>func)
            {
                NS_EVENT_ASYNC_VOID([func, this](ns_shared_ptr<char> buff, int buff_len) {
                    auto size = recv(buff.get(), buff_len);
                    func(buff, size);
                    }, buff, buff_len);
            }

            ns_shared_ptr<NetConnect> accept()
            {
                sockaddr_in remoteAddr;
                memset(&remoteAddr, 0, sizeof(remoteAddr));
                int len = 0;
                auto client = ::accept(sock_, NULL, NULL);

                if (client == ~0)
                {
                    auto e = WSAGetLastError();
                    printf("error accept code:%d\n", e);
                    return nullptr;
                }

                ns_shared_ptr<NetConnect> pl = ns_make_shared<NetConnect>(client);
                if (pl->is_connect())
                {
                    return pl;
                }
                return nullptr;
            }
            void accept(std::function<void(ns_shared_ptr<NetConnect> cl)> func, int num = 1)
            {
                for (int i = 0; i < num; ++i)
                {
                    NS_EVENT_ASYNC_VOID([this, func]()
                        {
                            printf("thread is run!\n");
                            while (is_connect())
                            {
                                //printf("thread accept!");
                                auto pl = accept();
                                if (pl)
                                {
                                    printf("thread accept %d!\n", pl->get_sock());
                                    func(pl);
                                }
                                else
                                {
                                    break;
                                }
                            }
                        });
                }
            }

            //获取连接对应的套接字
            SOCKET get_sock()
            {
                return sock_;
            }

            //关闭
            bool close()
            {
#if WIN32
                closesocket(sock_);
                sock_ = INVALID_SOCKET;
#else
                close(sock_);
                sock_ = -1;
#endif
                return true;
            }

            bool is_connect()
            {
                //return INVALID_SOCKET <= sock_;
                if (sock_ <= INVALID_SOCKET)
                    return false;

                char buffer[1];
                /*MSG_PEEK表示从输入队列中读数据但并不将数据从输入队列中移除*/
                auto ret = ::recv(sock_, buffer, 1, MSG_PEEK);

                return ret>=0;
            }
            //非阻塞
            int nonblocking()
            {
                unsigned long  nb = 1;

                return ioctlsocket(sock_, FIONBIO, &nb);
            }
            //阻塞
            int blocking()
            {
                unsigned long  nb = 0;

                return ioctlsocket(sock_, FIONBIO, &nb);
            }
        private:
            //套接字
            SOCKET sock_;
            //地址
            //std::string ip;
            //int port;
        };
    }
}
#endif // !TCP_LINK_H
