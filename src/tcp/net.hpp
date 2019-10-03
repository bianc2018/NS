/*
    ������,�������ӵĴ����رպ����ݵĶ�д
*/

#ifndef NET_HPP_
#define NET_HPP_
#include "connect.hpp"
#include <WS2tcpip.h>
#include "../util/logging.hpp"
namespace ns
{
    namespace net
    {
        class Net
        {
        public:
            Net()
            {
#ifndef __UNIX
                unsigned short ver;
                WSADATA wsaData;
                ver = MAKEWORD(1, 1);
                WSAStartup(ver, &wsaData);
#endif
            }
            ~Net()
            {
#ifndef __UNIX
                WSACleanup();
#endif
            }
        public:
            //���ӷ�����
            ns_shared_ptr<NetConnect> connect(const std::string& ip, int port,int type = NS_SOCK_STREAM)
            {
                auto sock = ::socket(AF_INET,type, NS_IPPROTO_TCP);
                if (sock<=0)
                {
                    return nullptr;
                }
                
                sockaddr_in addr;
                init_addr(ip, port, addr);
                auto ret = ::connect(sock, (const sockaddr*)& addr, sizeof(addr));
                if (ret==0)
                {
                    ns_shared_ptr<NetConnect> p = ns_make_shared<NetConnect>(sock);
                    return p;
                }
                err();
                return nullptr;
            }
            
            //����������
            ns_shared_ptr<NetConnect> open_service(const std::string& ip, int port, int type = SOCK_STREAM)
            {
                ns_shared_ptr<NetConnect> p = ns_make_shared<NetConnect>(type);
                auto sock = p->get_sock();
                if (sock <= 0)
                {
                    return nullptr;
                }

                sockaddr_in sin;
                init_addr(ip, port, sin);
                if (bind(sock, (const sockaddr*)& sin, sizeof(sin)) == SOCKET_ERROR)
                {
                    printf("bind error !");
                    return nullptr;
                }
                //��ʼ����  
                if (listen(sock, 5) == SOCKET_ERROR)
                {
                    printf("listen error !");
                    return nullptr;
                }
                return p;
            }

            void init_addr(const std::string& ip, int port, sockaddr_in& addr)
            {
                memset(&addr, 0, sizeof(addr));

                /*���� sockaddr_in �ṹ������ز���*/
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);//��һ���޷��Ŷ�������ֵת��Ϊ�����ֽ��򣬼����ģʽ(big-endian)��
                //printf("%d\n",INADDR_ANY);
                //INADDR_ANY����ָ����ַΪ0.0.0.0�ĵ�ַ�������ַ��ʵ�ϱ�ʾ��ȷ����ַ�������е�ַ�����������ַ����
                //һ����˵���ڸ���ϵͳ�о������Ϊ0ֵ��
                addr.sin_addr.s_addr = inet_addr(ip.c_str());//���������޷��ų�������ת���������ֽ�˳�򡣡�
            }
        
            std::vector<std::string> dns_query(const std::string& host)
            {
                /*
                    struct hostent
                    {
                        char *h_name;          //����
                        char ** h_aliases;     //����
                        short h_addrtype;        //ip���� ��v4����v6         v4��AF_INET��
                        short h_length;        //��ַ���ȣ�4  �����ҵ����ɶ�Ӷ�û��
                        char ** h_addr_list;   //ע����������һ��˫ָ��Ķ���   unp   p240��ͼ
                    };
                */
                std::vector<std::string> eps;
                if ("" == host)
                    return eps;
                hostent* phst = gethostbyname(host.c_str());
                if (phst)
                {
                    for (int i = 0; phst->h_addr_list[i]; ++i)
                    {
                        char dst[64] = { 0 };
                        auto p = inet_ntop(phst->h_addrtype, phst->h_addr_list[i], dst, 64);
                        if (p)
                        {
                            //std::string ip(p);
                            eps.push_back(p);
                        }
                    }
                }
                return eps;
                //in_addr* iddr = (in_addr*)phst->h_addr;
                //unsigned long IPUL = iddr->s_addr;
                //char* IP = inet_ntoa(*iddr);
              //  cout << IP;
            }

            int err()
            {
                int code = 0;
#if WIN32
               code=WSAGetLastError();
               LOG_ERR << "error is happend code = " << code<<",str="<< gai_strerrorA(code);
#endif
               return code;
            }

        };

        static Net GNET;
    }
}

#endif
