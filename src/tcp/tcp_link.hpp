/*
	tcp 连接,负责连接的创建关闭和数据的读写
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

		//连接，每一个对象实例代表一个连接实例
		class TcpLink:boost::noncopyable
		{
			//通知类型
			typedef  ns_func<void(ns_shared_ptr<TcpLink> link,void *data)> NOTE_HANDLER;

			//初始化，传入接收到的连接socket
			TcpLink(ns_shared_ptr<boost::asio::ip::tcp::socket> sock) :\
				sock_(sock), time_out_(60), recv_buff_size_(1024*5)
			{
				
			}
		public:
            //创建连接
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
			
			//解析
			~TcpLink()
			{
                LOG_DBG << "~TcpLink="<<get_remote_ip()<<":"<<get_local_port();
				close();
               
			}

            //链接
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
                                LOG_INFO << "连接：" << sock_->remote_endpoint().address().to_string() << ":" << \
                                    sock_->remote_endpoint().port();

                                //通知数据
                                NS_EVENT_ASYNC(&TcpLink::note_handler, this, \
                                    self, error::NS_TCP_CONNECTED, nullptr);
                            }
                            else
                            {
                                LOG_ERR << " 连接：套接字错误 ";
                                NS_EVENT_ASYNC(&TcpLink::note_handler, this, \
                                    self, error::NS_TCP_CLIENT_CONNECT_CLOSED_ERROR, nullptr);
                            }
                        }
                    });
                return error::NS_SUCCESS;
            }
			
			//获取连接对应的套接字
			ns_shared_ptr<boost::asio::ip::tcp::socket> get_sock_ptr()
			{
				return sock_;
			}

            //注册回调
            void reg_note_handler(error::NS_ERROR_CODE e, NOTE_HANDLER handler)
            {
                note_handler_map_[e] = handler;
            }

            //套接字方法
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
			
            //关闭
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
			//异步接收数据
			void async_recv()
			{
				//申请buff
				ns_shared_ptr<char> buff_ptr;
				try
				{
					buff_ptr = SHARED_BUFF_PTR(recv_buff_size_);
				}
				catch (std::bad_alloc &e)
				{
					LOG_ERR << "申请接收缓存区失败 " << e.what();
					NS_EVENT_ASYNC(&TcpLink::note_handler,this, shared_from_this(), error::NS_NEW_BUFF_ERR, nullptr);
					
					return;
				}

				//开始计时
				auto timer = start_timer(time_out_, boost::bind(&TcpLink::note_handler,this, shared_from_this(), \
					error::NS_TCP_RECV_TIMEOUT, nullptr));

                auto self = shared_from_this();

				//读到数据后的回调函数
				auto read_handler = \
					[this, buff_ptr, self, timer](boost::system::error_code ec, std::size_t s)
				{
					stop_timer(timer);

					if (ec)
					{
						LOG_ERR << "读数据错误 " << ec.value() << ec.message();
						NS_EVENT_ASYNC(&TcpLink::note_handler,this, self, error::NS_TCP_RECV_ERROR, nullptr);
						return;
					}
					//必须重新创建变量，不然数据无法传递给after_async_read 原理未明
					//auto n_buff_ptr_ = buff_ptr;
					//auto n_size_ = s;
					//处理数据
					
                    TcpBuff buff(buff_ptr, s);
                    note_handler(self, error::NS_TCP_RECVED, (void*)&buff);
					return;
				};

				//调用接口，从异步网络读取数据，一有数据就返回
				sock_->async_read_some(boost::asio::buffer(buff_ptr.get(), recv_buff_size_), read_handler);
			}

			//异步发送数据
			void async_send(ns_shared_ptr<char> buff_ptr, size_t len, 
				 size_t begin=0 )
			{
				//开始计时
				auto timer = start_timer(time_out_, boost::bind(&TcpLink::note_handler,this, shared_from_this(), \
					error::NS_TCP_SEND_TIMEOUT,nullptr));
                auto self = shared_from_this();
				//写完数据后的回调函数
				auto write_handler = \
					[this, self, len, begin, buff_ptr, timer]\
					(boost::system::error_code ec, std::size_t s)
				{
					stop_timer(timer);

					if (ec)
					{
						LOG_ERR << "写数据错误" << ec.value() << ec.message();
						NS_EVENT_ASYNC(&TcpLink::note_handler,this, self, error::NS_TCP_SEND_ERROR,nullptr);
						return;
					}
					//写完了
					if (len <= s)
					{
						//通知
						note_handler(self, error::NS_TCP_SENDED,nullptr);
						//返回异步写,取下一个buff
						//async_send(link);
						return;
					}
					//未写完
					//计算剩下的字节
					int now_len = len - s;
					//继续写
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
			//启动一个计时器
			ns_shared_ptr<boost::asio::steady_timer> start_timer(int time_out, boost::function<void()> call)
			{
				//定时器
				auto timer = ns_make_shared<boost::asio::steady_timer>(NS_IO);

				//设置超时
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
			//关闭一个计时器
			int stop_timer(ns_shared_ptr<boost::asio::steady_timer> timer)
			{
				boost::system::error_code ec;
				timer->cancel(ec);
				return ec.value();
			}
		
            //有可能为空
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
            //订阅
            std::map<error::NS_ERROR_CODE, NOTE_HANDLER> note_handler_map_;

			ns_shared_ptr<TcpLink> self_;
			
			//套接字
			ns_shared_ptr<tcp_socket> sock_;
			
			//超时时间 单位秒
			int time_out_;

            size_t recv_buff_size_;

		};
	}
}
#endif // !TCP_LINK_H
