/*
	http 客户端
*/
#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include "../tcp/net.hpp"

#include "http_request.hpp"
#include "http_response.hpp"

namespace ns
{
	namespace http
	{
        const Pos NET_BUFF_SIZE(1024 * 5);

		class HttpClient
		{
			//服务端回复函数
			typedef std::function<void(ns_shared_ptr<HttpClient> client, util::error::NS_ERROR_CODE e)> HttpServerResponseFun;

		public:
            HttpClient(const std::string& host):sock_(std::make_shared<net::NetConnect>())
            {
                UrlParam p;
                HttpObject::url_parse(host,p);
                ip_list_ = net::GNET.dns_query(p.host);
                port_ = p.port;
            }
        
            Response request(Request request, util::error::NS_ERROR_CODE* e = nullptr)
            {
                if (request)
                {
                    if (!re_connect())
                    {
                        return err_handler(e, util::error::NS_TCP_CLIENT_CONNECT_ERROR);
                    }
                    
                    //发送请求
                    auto head = request->get_ext_head();
                    auto sendlen = sock_->send(head);
                    if (sendlen < 0)
                    {
                        return err_handler(e, util::error::NS_TCP_SEND_ERROR);
                    }
                    LOG_DBG << "req head:\n" << head;
                    auto& body = request->get_body();
                    std::string buffer;
                    while (true)
                    {
                        buffer = body.read_content(NET_BUFF_SIZE);
                        if (0 == buffer.size())
                            break;
                        sendlen = sock_->send(buffer);
                        if (sendlen < 0)
                        {
                            return err_handler(e, util::error::NS_TCP_SEND_ERROR);
                        }
                        LOG_DBG << "req body:\n" << buffer;
                    }
                    
                    //接收回复
                    auto response = std::make_shared<HttpResponse>();
                    auto recv_buffer = util::make_shared_buff(NET_BUFF_SIZE);
                    while (true)
                    {
                        LOG_DBG << "recving";
                        auto  recvlen = sock_->recv(recv_buffer.get(), NET_BUFF_SIZE);
                        
                        if (recvlen < 0)
                        {
                            return err_handler(e, util::error::NS_TCP_SEND_ERROR);
                        }
                        else if (recvlen >= 0)
                        {
                            LOG_DBG << "recvlen=" << recvlen << "\n" << std::string(recv_buffer.get(), recvlen);
                            auto parser_ret = response->parse(recv_buffer.get(), recvlen);
                            
                            if (PARSE_OVER == parser_ret)
                            {
                                break;
                            }
                            else  if (PARSE_ERROR == parser_ret)
                            {
                                return err_handler(e, util::error::NS_HTTP_OBJECT_PARSER_ERROR);
                            }
                        }
                    }
                    if ("close" == response->get_head_value("Connection", "close"))
                    {
                        sock_->close();
                    }
                    return response;
                }
                else
                {
                    //连接失败
                    return err_handler(e, util::error::NS_TCP_CLIENT_CONNECT_ERROR);
                }
            }
        
            Response get(const std::string& url, util::error::NS_ERROR_CODE* e = nullptr)
            {
                UrlParam p;
                HttpObject::url_parse(url, p);
                if (0 == p.path.size())
                    return err_handler(e, util::error::NS_PARAM_ERROR);

                auto req = std::make_shared<HttpRequest>();
                req->set_method(Method_GET);
                req->set_url(p.path+p.params);
               // req->set_head_value("Host", p.host);
                req->set_head_value("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*; q = 0.8");
                req->set_head_value("Accept-Encoding", "gzip,deflate,br");
                req->set_head_value("Accept-Language", "zh-CN,zh; q=0.9");
               // req->set_head_value("Cookie", "BIDUPSID=29B2389DF64DDDE8F48686A96F71F10F;PSTM=1543046345;BD_UPN=12314753;BAIDUID=009C49EBCE04EEFC06A9907D593B167E:FG = 1; BDUSS = TdLa25QSGZhRGZHemRUc1hhempoWHJpSHZiR3d0T2lafi1SODdmUn4zTFA5UkJkSVFBQUFBJCQAAAAAAAAAAAEAAABLlTREZmhncmJjaWRyaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAM9o6VzPaOlcd; MCITY = -% 3A; __cfduid = dde788759db25de05b63777dd5bd0e3301564582480; ispeed_lsm = 2; BDORZ = B490B5EBF6F3CD402E515D22BCDA1598; delPer = 0; BD_CK_SAM = 1; H_PS_PSSID = 1450_21114_29523_29720_29567_29220_26350_22160; rsv_jmp_slow = 1569941363994; B64_BOT = 1; PSINO = 6; H_PS_645EC = cbd18yH % 2Bpc8TGmidsjI6UbYyVl9uAEtG9AAgoMJzE8h7Lwq9jUDgLHvwupjkgDD2xctu; BD_HOME = 1");
                req->set_head_value("Upgrade-Insecure-Requests","1");
                req->set_head_value("Connection", "keep-alive");
                req->set_head_value("User-Agent", "Mozilla/5.0(Windows NT 10.0; WOW64)AppleWebKit/537.36(KHTML,like Gecko)Chrome/63.0.3239.132 Safari/537.36");
                req->set_head_value("Cache-Control", "max-age=0");
                /*
                Accept:
                    Accept - Encoding:gzip, deflate, br
                    Accept - Language : zh - CN, zh; q = 0.9
                    Cache - Control:max - age = 0
                    Connection : keep - alive
                    Cookie : BIDUPSID = 29B2389DF64DDDE8F48686A96F71F10F; PSTM = 1543046345; BD_UPN = 12314753; BAIDUID = 009C49EBCE04EEFC06A9907D593B167E:FG = 1; BDUSS = TdLa25QSGZhRGZHemRUc1hhempoWHJpSHZiR3d0T2lafi1SODdmUn4zTFA5UkJkSVFBQUFBJCQAAAAAAAAAAAEAAABLlTREZmhncmJjaWRyaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAM9o6VzPaOlcd; MCITY = -% 3A; __cfduid = dde788759db25de05b63777dd5bd0e3301564582480; ispeed_lsm = 2; BDORZ = B490B5EBF6F3CD402E515D22BCDA1598; delPer = 0; BD_CK_SAM = 1; H_PS_PSSID = 1450_21114_29523_29720_29567_29220_26350_22160; rsv_jmp_slow = 1569941363994; B64_BOT = 1; PSINO = 6; H_PS_645EC = cbd18yH % 2Bpc8TGmidsjI6UbYyVl9uAEtG9AAgoMJzE8h7Lwq9jUDgLHvwupjkgDD2xctu; BD_HOME = 1
                    Host:www.baidu.com
                    Upgrade - Insecure - Requests : 1
                    User - Agent : Mozilla / 5.0 (Windows NT 10.0; WOW64) AppleWebKit / 537.36 (KHTML, like Gecko) Chrome / 63.0.3239.132 Safari / 537.36
                */
                return request(req, e);
            }
        private:
            //重连
            bool re_connect()
            {
                if (sock_->is_connect())
                    return true;

                for (auto ip : ip_list_)
                {
                    sockaddr_in add;
                    sock_ = net::GNET.connect(ip, port_);
                    if (sock_)
                    {
                        return true;
                    }
                }
                return false;
            }

            Response err_handler(util::error::NS_ERROR_CODE* e, \
                util::error::NS_ERROR_CODE code) 
            {
                //连接失败
                LOG_ERR << util::error::msg(code);
                if (e)
                    * e = code;

                return nullptr;
            }
		private:
            std::vector<std::string> ip_list_;
            int port_;
            ns_shared_ptr<net::NetConnect>  sock_;
		};
	}
}
#endif