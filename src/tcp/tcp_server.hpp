/*
	tcp ������
*/
#ifndef NS_TCP_SERVER_H_
#define NS_TCP_SERVER_H_
#include "tcp_link.hpp"
namespace ns
{
	namespace tcp
	{
		class TcpServer 
		{
			//���� acceptor
			typedef boost::asio::ip::tcp::acceptor acceptor;
			//���ջص�����
			typedef std::function<void(ns_shared_ptr<TcpLink>, error::NS_ERROR_CODE)> accept_handler;
		public:
			TcpServer(int port) :port_(port),server_(NS_IO)
			{

			}

			error::NS_ERROR_CODE run(accept_handler handler)
			{
				//Э��ջ�첽��������
				if (!handler)
				{
					LOG_ERR << "accept_handler_ is a nullptr";
					return error::NS_TCP_HANDLER_NULL_ERROR;
				}

				boost::asio::ip::tcp::resolver resolver(NS_IO);
				boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve("0.0.0.0", std::to_string(port_)).begin();

				boost::system::error_code ec;
				//������
				server_.open(endpoint.protocol(), ec);
				if (ec)
				{
					LOG_ERR<< "�����Ӵ��� "<<ec.value()<<" "<<ec.message();
					return error::NS_TCP_SERVER_OPEN_ERR;
				}
				//���ò�������ַ������
				server_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
				if (ec)
				{
					LOG_ERR<<"���ò�������ַ�����ô��� "<<ec.value()<<" "<< ec.message();
					//TCP_ERROR_SERVER_SET_ERROR
					return error::NS_TCP_SERVER_SET_ERROR;
				}
				//�󶨵�ַ
				server_.bind(endpoint, ec);
				if (ec)
				{
					LOG_ERR << "�󶨵�ַ���� "<< ec.value()<<" "<< ec.message();
					return error::NS_TCP_SERVER_BIND_ERROR;
				}

				//����
				server_.listen();
				//�첽����
				for(size_t i=0;i< NS_CPU_NUM;++i)
					NS_EVENT_ASYNC(&TcpServer::accept, this, handler);
				return error::NS_SUCCESS;
			}
		private:
			void accept(accept_handler handler)
			{
				auto link = TcpLink::create();
				
                server_.async_accept(*(link->get_sock_ptr()),\
					[this, link, handler](const boost::system::error_code& error)
				{
					if (error)
					{
						LOG_ERR<<"tcp async_accept err "<<error.value()<<error.message();
						NS_EVENT_ASYNC(handler, link, error::NS_TCP_SERVER_ACCEPT_ERROR);
					}
					else
					{
                        
						auto sock = link->get_sock_ptr();
						if (sock->is_open())
						{
                           
							LOG_INFO<< "������ӣ�"<< sock->remote_endpoint().address().to_string()<< ":"<< \
								sock->remote_endpoint().port();
							NS_EVENT_ASYNC(handler, link, error::NS_SUCCESS);
						}
						else
						{
							LOG_ERR<<" ������ӣ��׽��ִ��� ";
							NS_EVENT_ASYNC(handler, link, error::NS_TCP_SERVER_LINK_CLOSED_ERROR);
						}
						//�ظ�����
						NS_EVENT_ASYNC(&TcpServer::accept, this, handler);
					}

				});
			}
		private:
			int port_;
			//��������
			acceptor server_;
			
		};
	}
}
#endif