#include "../src/http/http_client.hpp"
#include <map>
#include <string>
using namespace ns;
int main()
{
    AUTO_LOGGER(".");					// 在当前目录创建以日期命名的日志文件.
    http::HttpClient cl_("127.0.0.1:5160");
    auto res = cl_.get("/index.html");
    system("pause");
    return 0;
}

