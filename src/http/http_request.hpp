/*
	http 请求
*/
#ifndef HTTP_REQUEST_H_
#define HTTP_REQUEST_H_

#include <regex>

#include "http_object.hpp"

namespace ns
{
	namespace http
	{
		class HttpRequest :public HttpObject
		{
		public:
            HttpRequest()
            {
                set_version(HTTP_VERSION11);
            }

			//方法
			std::string get_method()
			{
				return get_beg_line(0);
			}
			void set_method(const std::string m)
			{
				set_beg_line(0, m);
			}
			//请求
			std::string get_url()
			{
				return get_beg_line(1);
			}
			void set_url(const std::string m)
			{
				set_beg_line(1, m);
			}
			//版本
			std::string get_version()
			{
				return get_beg_line(2);
			}
			void set_version(const std::string m)
			{
				set_beg_line(2, m);
			}
		
            virtual ParserStatus parse(const char* buff, Pos len) override
			{
                return parse_row(buff, len);
			}
		
			std::string get_ext_head()
			{
				return get_head();
			}

		};
	
		typedef ns_shared_ptr<HttpRequest> Request;
	}
}
#endif