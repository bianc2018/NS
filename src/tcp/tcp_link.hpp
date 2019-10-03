/*
	tcp ����,�������ӵĴ����رպ����ݵĶ�д
*/

#ifndef TCP_LINK_H
#define TCP_LINK_H

#include <map>
#include <atomic>
#include <memory>
#include <map>

#include <boost/asio.hpp>


#include "../util/ns_error.hpp"
#include "../util/logging.hpp"
#include "../util/util.hpp"
#include "../util/event.hpp"
#include "../util/ns_cache.hpp"

using namespace ns::util;

namespace ns
{
	namespace tcp
	{
        typedef boost::asio::ip::tcp::socket tcp_socket;

		//���ӣ�ÿһ������ʵ������һ������ʵ��
		class TcpLink:boost::noncopyable
		{
			//֪ͨ����
			typedef  ns_func<void(ns_shared_ptr<TcpLink> link,void *data)> NOTE_HANDLER;

			//��ʼ����������յ�������socket
			TcpLink(ns_shared_ptr<boost::asio::ip::tcp::socket> sock) :\
				sock_(sock), time_out_(60), recv_buff_size_(1024*5)
			{
				
			}
		public:
            //��������
			static ns_shared_ptr<TcpLink> create(ns_shared_ptr<boost::asio::ip::tcp::socket> sock = nullptr)
			{
				if (nullptr == sock)
				{
					sock = ns_make_shared<boost::asio::ip::tcp::socket>(NS_IO);
				}
				ns_shared_ptr<TcpLink> ptr(new TcpLink(sock));
				ptr->self_ = ptr;
				return ptr;
			}
			
			//����
			~TcpLink()
			{
                LOG_DBG << "~TcpLink="<<get_remote_ip()<<":"<<get_local_port();
				close();
               
			}

            //����
            error::NS_ERROR_CODE connect(const std::string& ip, const std::string& port)
            {
                boost::asio::ip::tcp::resolver resolver(NS_IO);
                boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(ip, port).begin();
                return connect(endpoint);
            }
            error::NS_ERROR_CODE connect(const boost::asio::ip::tcp::endpoint& ep)
            {
                auto timer = start_timer(time_out_, \
                    std::bind(&TcpLink::note_handler, this, shared_from_this(),\
                        error::NS_TCP_CLIENT_CONNECT_TIME_OUT_ERROR, nullptr));
                auto self = shared_from_this();
                sock_->async_connect(ep, [this, self,timer](const boost::system::error_code& error)
                    {
                        stop_timer(timer);
                        if (error)
                        {
                            LOG_ERR << "tcp async_connect err " << error.value() << error.message();

                            NS_EVENT_ASYNC(&TcpLink::note_handler, this, \
                                self, error::NS_TCP_CLIENT_CONNECT_ERROR,nullptr);
                        }
                        else
                        {

                            if (sock_->is_open())
                            {
                                LOG_INFO << "���ӣ�" << sock_->remote_endpoint().address().to_string() << ":" << \
                                    sock_->remote_endpoint().port();

                                //֪ͨ����
                                NS_EVENT_ASYNC(&TcpLink::note_handler, this, \
                                    self, error::NS_TCP_CONNECTED, nullptr);
                            }
                            else
                            {
                                LOG_ERR << " ���ӣ��׽��ִ��� ";
                                NS_EVENT_ASYNC(&TcpLink::note_handler, this, \
                                    self, error::NS_TCP_CLIENT_CONNECT_CLOSED_ERROR, nullptr);
                            }
                        }
                    });
                return error::NS_SUCCESS;
            }
			
			//��ȡ���Ӷ�Ӧ���׽���
			ns_shared_ptr<boost::asio::ip::tcp::socket> get_sock_ptr()
			{
				return sock_;
			}

            //ע��ص�
            void reg_note_handler(error::NS_ERROR_CODE e, NOTE_HANDLER handler)
            {
                note_handler_map_[e] = handler;
            }

            //�׽��ַ���
			std::string get_remote_ip()
			{
				return sock_->remote_endpoint().address().to_string();
			}
			size_t get_remote_port()
			{
				return sock_->remote_endpoint().port();
			}
			std::string get_local_ip()
			{
				return sock_->local_endpoint().address().to_string();
			}
			size_t get_local_port()
			{
				return sock_->local_endpoint().port();
			}
			bool is_open()
			{
				//sock_->
				return sock_->is_open();
			}
			bool is_connect()
			{
				if (is_open())
				{
                    
                    return sock_->native_handle().have_remote_endpoint();
				}
				return false;
			 }
			
            //�ر�
			bool close()
			{
				try {
					if (is_connect())
					{
						sock_->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                        sock_->close();
					}
					TcpLink::note_handler(shared_from_this(), error::NS_TCP_CLOSEED, nullptr);
                    self_.reset();
					//BOOST_LOG_TRIVIAL(warning) << "close the connection";
				}
				catch (std::exception& e) 
				{
					LOG_DBG << "thread id: " << std::this_thread::get_id() << " " << e.what();
					return false;

					//BOOST_LOG_TRIVIAL(warning) << "thread id: " << this_thread::get_id() << " " << e.what();
				}
				
				return true;
			}

		public:
			//�첽��������
			void async_recv()
			{
				//����buff
				ns_shared_ptr<char> buff_ptr;
				try
				{
					buff_ptr = SHARED_BUFF_PTR(recv_buff_size_);
				}
				catch (std::bad_alloc &e)
				{
					LOG_ERR << "������ջ�����ʧ�� " << e.what();
					NS_EVENT_ASYNC(&TcpLink::note_handler,this, shared_from_this(), error::NS_NEW_BUFF_ERR, nullptr);
					
					return;
				}

				//��ʼ��ʱ
				auto timer = start_timer(time_out_, boost::bind(&TcpLink::note_handler,this, shared_from_this(), \
					error::NS_TCP_RECV_TIMEOUT, nullptr));

                auto self = shared_from_this();

				//�������ݺ�Ļص�����
				auto read_handler = \
					[this, buff_ptr, self, timer](boost::system::error_code ec, std::size_t s)
				{
					stop_timer(timer);

					if (ec)
					{
						LOG_ERR << "�����ݴ��� " << ec.value() << ec.message();
						NS_EVENT_ASYNC(&TcpLink::note_handler,this, self, error::NS_TCP_RECV_ERROR, nullptr);
						return;
					}
					//�������´�����������Ȼ�����޷����ݸ�after_async_read ԭ��δ��
					//auto n_buff_ptr_ = buff_ptr;
					//auto n_size_ = s;
					//��������
					
                    TcpBuff buff(buff_ptr, s);
                    note_handler(self, error::NS_TCP_RECVED, (void*)&buff);
					return;
				};

				//���ýӿڣ����첽�����ȡ���ݣ�һ�����ݾͷ���
				sock_->async_read_some(boost::asio::buffer(buff_ptr.get(), recv_buff_size_), read_handler);
			}

			//�첽��������
			void async_send(ns_shared_ptr<char> buff_ptr, size_t len, 
				 size_t begin=0 )
			{
				//��ʼ��ʱ
				auto timer = start_timer(time_out_, boost::bind(&TcpLink::note_handler,this, shared_from_this(), \
					error::NS_TCP_SEND_TIMEOUT,nullptr));
                auto self = shared_from_this();
				//д�����ݺ�Ļص�����
				auto write_handler = \
					[this, self, len, begin, buff_ptr, timer]\
					(boost::system::error_code ec, std::size_t s)
				{
					stop_timer(timer);

					if (ec)
					{
						LOG_ERR << "д���ݴ���" << ec.value() << ec.message();
						NS_EVENT_ASYNC(&TcpLink::note_handler,this, self, error::NS_TCP_SEND_ERROR,nullptr);
						return;
					}
					//д����
					if (len <= s)
					{
						//֪ͨ
						note_handler(self, error::NS_TCP_SENDED,nullptr);
						//�����첽д,ȡ��һ��buff
						//async_send(link);
						return;
					}
					//δд��
					//����ʣ�µ��ֽ�
					int now_len = len - s;
					//����д
					async_send(buff_ptr, now_len, begin + s);
					return;
				};

				sock_->async_write_some(boost::asio::buffer(buff_ptr.get() + begin, len), write_handler);
			}
            void async_send(const std::string& buff)
            {
                auto buff_ptr = util::make_shared_from_str(buff);
                async_send(buff_ptr, buff.size());
            }
			//����һ����ʱ��
			ns_shared_ptr<boost::asio::steady_timer> start_timer(int time_out, boost::function<void()> call)
			{
				//��ʱ��
				auto timer = ns_make_shared<boost::asio::steady_timer>(NS_IO);

				//���ó�ʱ
				auto time_out_handler = \
					[call](boost::system::error_code ec)
				{
					if (!ec)
					{
						call();
					}
				};

				timer->expires_from_now(std::chrono::seconds(time_out));
				timer->async_wait(time_out_handler);
				return timer;

			}
			//�ر�һ����ʱ��
			int stop_timer(ns_shared_ptr<boost::asio::steady_timer> timer)
			{
				boost::system::error_code ec;
				timer->cancel(ec);
				return ec.value();
			}
		
            //�п���Ϊ��
			ns_shared_ptr<TcpLink> shared_from_this()
			{
               /// auto l = self_.lock();
                //if(nullptr == l)
                //    LOG_DBG << "shared this is nullptr";
				return self_;
			}
        private:
            void note_handler(ns_shared_ptr<TcpLink> link, error::NS_ERROR_CODE err_code,void* data)
            {
                auto handler = note_handler_map_.find(err_code);
                if (note_handler_map_.end() != handler)
                {
                    handler->second(link, data);
                }

                if (nullptr!= link&&link->is_open())
                    LOG_DBG << "link " << link->get_remote_ip() << ":" << link->get_remote_port()\
                    << ",note=" << error::msg(err_code);
                else
                    LOG_DBG << "link null,note=" << error::msg(err_code);
                return;
            }
		private:
            //����
            std::map<error::NS_ERROR_CODE, NOTE_HANDLER> note_handler_map_;

			ns_shared_ptr<TcpLink> self_;
			
			//�׽���
			ns_shared_ptr<tcp_socket> sock_;
			
			//��ʱʱ�� ��λ��
			int time_out_;

            size_t recv_buff_size_;

		};
	}
}
#endif // !TCP_LINK_H
