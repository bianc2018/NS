/*
	http 回复报文
*/
#ifndef HTTP_RESPONSE_H_
#define HTTP_RESPONSE_H_
#include "http_object.hpp"
namespace ns
{
	namespace http
	{
		//HTTP-Version Status-Code Reason-Phrase 
		class HttpResponse :public HttpObject
		{
		public:
			//方法
			std::string get_version()
			{
				return get_beg_line(0);
			}
			void set_version(const std::string m)
			{
				set_beg_line(0, m);
			}
			//请求
			std::string get_status_code()
			{
				return get_beg_line(1);
			}
			void set_status_code(const std::string m)
			{
				set_beg_line(1, m);
			}
			//版本
			std::string get_reason_phrase()
			{
				return get_beg_line(2);
			}
			void set_reason_phrase(const std::string m)
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
	
		typedef ns_shared_ptr<HttpResponse> Response;
	}
	
}
#endif