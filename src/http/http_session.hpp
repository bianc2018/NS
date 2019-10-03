#ifndef HTTP_SESSION_H_
#define HTTP_SESSION_H_

#include <atomic>

#include "..//tcp/tcp_link.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

namespace ns
{
    namespace http
    {
        class HttpSession
        {
            typedef std::function<void(ns_shared_ptr<HttpSession>,Response)> ResponseHandler;
            typedef std::function<void(ns_shared_ptr<HttpSession>,Request)> RequestHandler;
        public:
            ns_shared_ptr<HttpSession> create(const std::string& host);
            
            ns_shared_ptr<HttpSession> create(ns_shared_ptr<tcp::TcpLink> link);

            void request(Request req, ResponseHandler handler)
            {
                re_connenct();
                if (req)
                {
                    auto h = req->get_head();
                    Pos beg = 0;
                    link_->reg_note_handler(error::NS_TCP_SENDED, [req, handler](ns_shared_ptr<tcp::TcpLink> link, void* data)mutable
                        {
                            //auto body = req->get_body()
                        });
                    link_->async_send(h);
                }
            }
             
            void response(Response res, RequestHandler handler);

           
            void close();
        protected:
            ns_shared_ptr<HttpSession> shared_from_this()
            {
                return self_;
            }
        private:
            void re_connenct()
            {
                if (nullptr == link_)
                {
                    link_->connect
                }
                if (link_->is_connect())
                {
                    
                }
            }
            void recv_request(ns_shared_ptr<tcp::TcpLink> link, void* data,\
                Request req, RequestHandler handler, ns_shared_ptr<HttpSession> self);
            void recv_Response(ns_shared_ptr<tcp::TcpLink> link, void* data,\
                Response res, ResponseHandler handler, ns_shared_ptr<HttpSession> self);
        private:
            std::atomic_bool re_flag_;
            boost::asio::ip::tcp::endpoint ep_;
            ns_shared_ptr<tcp::TcpLink> link_;
            ns_shared_ptr<HttpSession> self_;
        };
    }
}
#endif
