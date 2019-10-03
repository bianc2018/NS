/*
	http 传输对象
*/
#ifndef HTTP_OBJECT_HPP_
#define HTTP_OBJECT_HPP_

#include <unordered_map>
#include <regex>

#include <boost/noncopyable.hpp>

#include "../util/util.hpp"
#include "../util/logging.hpp"
#include "../util/ns_error.hpp"
#include "../util/ns_cache.hpp"

#define JSON_MIME "application/json; charset=UTF-8"
#define XML_MIME "application/rss+xml"
#define HTML_MIME "text/html"
#define HTTP_CT "Content-Type"
#define HTTP_LEN "Content-Length"
#define HTTP_CONN "Connection"
#define HTTP_RANGE "Content-Range"


namespace ns
{
	
	namespace http
	{
		//class BuffCache
		enum ParserStatus
		{
			PARSE_START_LINE1 = 1,
            PARSE_START_LINE2 = 2,
            PARSE_START_LINE3 = 3,
			PARSE_HEAD_KEY = 4,
            PARSE_HEAD_VALUE = 5,
			PARSE_BODY = 6,
			PARSE_OVER = 7,
			PARSE_ERROR = -1,
		};

		const std::string CRLF("\r\n");
		const std::string CRLFCRLF("\r\n\r\n");
		const std::string SPACE(" ");
		const std::string HEAD_SPLIT(": ");

		//http version
		const std::string HTTP_VERSION10("HTTP/1.0");
		const std::string HTTP_VERSION11("HTTP/1.1");

		//请求方法
		const std::string Method_GET("GET");
		const std::string Method_POST("POST");

        struct UrlParam
        {
            std::string protocol;
            std::string host;
            int port=80;
            std::string path;
            std::string params;
        };

        class HttpObject :boost::noncopyable
        {

        public:
            HttpObject() :\
                parser_status_(PARSE_START_LINE1), content_len_cache_(0)
            {

            }
            ~HttpObject()
            {
                
            }

            Pos get_body_len()
            {
                return body_.get_size();
            }

            util::BufferCache& get_body()
            {
                return body_;
            }

            //设置回复报文的报文头
            void set_head_value(const std::string& key, const std::string& value)
            {
                //覆盖
                header_[key] = value;
            }
            std::string get_head_value(const std::string& key, const std::string& notfond = "")
            {
                auto p = header_.find(key);
                if (header_.end() == p)
                {
                    return notfond;
                }
                return p->second;
            }

            //获取根据http协议构建的报文头
            std::string get_head()
            {
                //增加字段
                if (!header_.count(HTTP_CT))
                {
                    //HTML_MIME
                    set_head_value(HTTP_CT, HTML_MIME);
                }
                if (!header_.count(HTTP_LEN))
                {
                    set_head_value(HTTP_LEN, std::to_string(body_.get_size()));
                }
                if (!header_.count(HTTP_CONN))
                {
                    //HTML_MIME 默认关闭连接
                    set_head_value(HTTP_CONN, "close");
                }

                std::string head = beg_line_[0] + SPACE + beg_line_[1] + SPACE + beg_line_[2] + CRLF;
                for (auto h : header_)
                {
                    head += h.first + HEAD_SPLIT + h.second + CRLF;
                }
                head += CRLF;
                return head;
            }

            virtual ParserStatus parse(const char* buff, Pos len) = 0;
            
            ParserStatus get_parser_status()
            {
                return parser_status_;
            }
        public:
            //公共解析类
            //解析url
            static bool url_parse(const std::string& url, UrlParam& in_param)
            {
                enum Pstatus
                {
                    P_Protocol,
                    P_Host,
                    P_Port,
                    P_Path,
                    P_Params
                };
                UrlParam param;
                int size = url.size();
                Pstatus status = P_Protocol;
                if (size != 0 && '/' == url.at(0))
                {
                    status = P_Path;
                }
                std::string port_str;
                for (int i = 0; i < size; i++)
                {
                    auto ch_p = url.at(i);

                    if (P_Protocol == status)
                    {
                        if (':' == ch_p)
                        {
                            i += 2;
                            status = P_Host;

                        }
                        else if ('.' == ch_p)
                        {
                            param.host += param.protocol;
                            param.host += ch_p;
                            param.protocol = "";
                            status = P_Host;
                        }
                        else if ('/' == ch_p)
                        {
                            param.path += param.protocol;
                            param.path += ch_p;
                            param.protocol = "";
                            status = P_Path;
                        }
                        else
                        {
                            param.protocol += ch_p;
                        }
                    }
                    else  if (P_Host == status)
                    {
                        //有端口
                        if (':' == ch_p)
                        {
                            status = P_Port;
                        }
                        else if ('/' == ch_p)//无端口
                        {
                            status = P_Path;
                            param.path += ch_p;
                        }
                        else
                        {
                            param.host += ch_p;
                        }
                    }
                    else  if (P_Port == status)
                    {
                        if ('/' == ch_p)
                        {
                            status = P_Path;
                            param.path += ch_p;
                            try
                            {
                                if ("" != port_str)
                                    param.port = std::stoi(port_str);
                            }
                            catch (std::exception& e)
                            {
                                param.port = 80;
                            }
                        }
                        else
                        {
                            port_str += ch_p;
                        }
                    }
                    else  if (P_Path == status)
                    {
                        if ('?' == ch_p)
                        {
                            status = P_Params;
                        }
                        else
                        {
                            param.path += ch_p;
                        }
                    }
                    else  if (P_Params == status)
                    {
                        param.params += ch_p;
                    }
                }
                if (P_Protocol == status)
                    return false;

                if (P_Port == status)
                {

                    try
                    {
                        if ("" != port_str)
                            param.port = std::stoi(port_str);
                    }
                    catch (std::exception& e)
                    {
                        param.port = 80;
                    }

                }
                in_param = param;
                return true;
            }

            //解析param 键值对 key=value&key=value
            static bool param_parse(const std::string& param, \
                std::map<std::string, std::string>& kvs)
            {
                enum Pstatus
                {
                    P_Key,
                    P_Value,
                };

                int size = param.size();
                Pstatus status = P_Key;
                std::string key_str, value_str;

                for (int i = 0; i < size; i++)
                {
                    auto ch_p = param.at(i);

                    if (P_Key == status)
                    {
                        if ('=' == ch_p)
                        {
                            status = P_Value;
                        }
                        else
                        {
                            key_str += ch_p;
                        }
                    }
                    else  if (P_Value == status)
                    {
                        //有端口
                        if ('&' == ch_p)
                        {
                            //加入键值
                            status = P_Key;
                            if ("" == key_str)
                                return false;
                            kvs.emplace(key_str, value_str);
                            key_str = "";
                            value_str = "";
                        }
                        else
                        {
                            value_str += ch_p;
                        }
                    }

                }
                if (P_Key == status)
                    return false;
                if ("" == key_str)
                    return false;
                kvs.emplace(key_str, value_str);
                return true;
            }

            //解析param 路径 key=value&key=value
            static bool path_parse(const std::string& path, \
                std::vector<std::string>& paths)
            {
                int size = path.size();
                std::string str;

                for (int i = 0; i < size; i++)
                {
                    auto ch_p = path.at(i);

                    if ('/' == ch_p)
                    {
                        if ("" == str)
                            continue;
                        paths.push_back(str);
                        str = "";
                    }
                    else
                    {
                        str += ch_p;
                    }
                }
                return true;
            }

            //解析原始数据
            ParserStatus parse_row(const char* buff, Pos len)
            {
                for (Pos i = 0; i < len; ++i)
                {
                    auto ch_p = buff[i];
                    
                    if (PARSE_START_LINE1 == parser_status_)
                    {
                        if (' ' == ch_p)
                        {
                            set_beg_line(0, parser_cache1_);
                            parser_cache1_ = "";
                            parser_status_ = PARSE_START_LINE2;
                        }
                        else
                        {
                            parser_cache1_ += ch_p;
                        }
                    }
                    else if (PARSE_START_LINE2 == parser_status_)
                    {
                        if (' ' == ch_p)
                        {
                            set_beg_line(1, parser_cache1_);
                            parser_cache1_ = "";
                            parser_status_ = PARSE_START_LINE3;
                        }
                        else
                        {
                            parser_cache1_ += ch_p;
                        }
                    }
                    else if (PARSE_START_LINE3 == parser_status_)
                    {
                        if ('\r' == ch_p)
                        {
                            continue;
                        }
                        else if ('\n' == ch_p)
                        {
                            set_beg_line(2, parser_cache1_);
                            parser_cache1_ = "";
                            parser_status_ = PARSE_HEAD_KEY;
                        }
                        else
                        {
                            parser_cache1_ += ch_p;
                        }
                    }
                    else if (PARSE_HEAD_KEY == parser_status_)
                    {
                        
                        if ('\r' == ch_p)
                        {
                            continue;
                        }
                        else if ('\n' == ch_p)
                        {
                            if (parser_cache1_ != "")
                            {
                                parser_status_ = PARSE_ERROR;
                                return parser_status_;
                            }

                            if (get_body_len() >= content_len_cache_)
                                parser_status_ = PARSE_OVER;
                            else
                                parser_status_ = PARSE_BODY;
                        }
                        else if (':' == ch_p)
                        {
                            
                            parser_status_ = PARSE_HEAD_VALUE;
                        }
                        else
                        {
                            parser_cache1_ += ch_p;
                        }
                    }
                    else if (PARSE_HEAD_VALUE == parser_status_)
                    {
                        if ('\r' == ch_p)
                        {
                            continue;
                        }
                        else if ('\n' == ch_p)
                        {
                            //键值为空
                            if ("" == parser_cache1_)
                            {
                                parser_status_ = PARSE_ERROR;
                                return parser_status_;
                            }
                            //缓存长度
                            if ((HTTP_LEN == parser_cache1_) && ("" != parser_cache2_))
                            {
                                try
                                {
                                    content_len_cache_ = std::stoll(parser_cache2_);
                                    
                                }
                                catch (std::exception& e)
                                {
                                    parser_status_ = PARSE_ERROR;
                                    return parser_status_;
                                }
                            }
                            LOG_DBG << parser_cache1_<<":"<< parser_cache2_;
                            header_.emplace(parser_cache1_, parser_cache2_);
                            parser_cache1_ = "";
                            parser_cache2_ = "";
                            parser_status_ = PARSE_HEAD_KEY;
                        }
                        else
                        {
                            parser_cache2_ += ch_p;
                        }
                    }
                    else if (PARSE_BODY == parser_status_)
                    {
                        if(!body_.write(buff + i, len - i))
                            parser_status_ = PARSE_ERROR;

                        if (get_body_len() >= content_len_cache_)
                            parser_status_ = PARSE_OVER;
                        LOG_DBG << "body:" << content_len_cache_ << ":" << get_body_len()<<",parser_status_ "<< parser_status_;
                        break;
                    }

                }

                return parser_status_;
            }

        protected:
            //设置http版本行首
            void set_beg_line(int index, const std::string& value)
            {
                beg_line_[index] = value;
            }
            std::string  get_beg_line(int index)
            {
                if (index < 0 || index>3)
                    return "";
                return beg_line_[index];
            }

        protected:
            //解析状态
            ParserStatus parser_status_;
            //解析缓存
            std::string parser_cache1_;
            std::string parser_cache2_;

            Pos content_len_cache_;
        private:
            //报文首部 version_ + SPACE + status_code_ + SPACE + reason_ + CRLF;
            std::string beg_line_[3];

            //结构化的报文头 key:value
            std::unordered_map<std::string, std::string> header_;

            //报文内容
            util::BufferCache body_;
        };
	}
}
#endif
