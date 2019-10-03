#ifndef  NS_ERROR_HPP_
#define  NS_ERROR_HPP_
namespace ns
{
	namespace util
	{
		namespace error
		{
			enum NS_ERROR_CODE
			{
				//状态信息
				//连接已经建立
				NS_TCP_CONNECTED =1,
				//连接正常关闭
				NS_TCP_CLOSEED   =2,
				//连接收到一段数据
				NS_TCP_RECVED	 =3,
				//连接发送完一段数据
				NS_TCP_SENDED	 =4,
				//无异常
				NS_SUCCESS = 0,
				//创建缓冲区失败
				NS_NEW_BUFF_ERR = -1,
				//tcp接收数据超时
				NS_TCP_RECV_TIMEOUT = -2,
				//tcp接收数据失败
				NS_TCP_RECV_ERROR = -3,
				//tcp发送数据超时
				NS_TCP_SEND_TIMEOUT = -4,
				//tcp发送数据失败
				NS_TCP_SEND_ERROR = -5,
				//tcp 服务连接打开失败
				NS_TCP_SERVER_OPEN_ERR = -6,
				//tcp 服务器参数设置失败 SERVER_SET_ERROR
				NS_TCP_SERVER_SET_ERROR=-7,
				//tcp 服务器 绑定地址错误
				NS_TCP_SERVER_BIND_ERROR = -8,
				//tcp 服务器 接收建立连接时出现错误
				NS_TCP_SERVER_ACCEPT_ERROR = -9,
				//tcp 服务器  连接已关闭
				NS_TCP_SERVER_LINK_CLOSED_ERROR = -10,
				//tcp 客户端  连接异常
				NS_TCP_CLIENT_CONNECT_ERROR = -11,
				//tcp 客户端  连接超时
				NS_TCP_CLIENT_CONNECT_TIME_OUT_ERROR = -12,
				//tcp 客户端  连接关闭
				NS_TCP_CLIENT_CONNECT_CLOSED_ERROR = -13,
				//tcp 传入的句柄为空
				NS_TCP_HANDLER_NULL_ERROR = -14,
				//目标文件不存在
				NS_FILE_IS_NOT_EXIST = -15,
				//错误的RANGE
				NS_BAD_RANGE = -16,
				//无法打开文件
				NS_NOT_OPEN_FILE = -17,
				//无效的文件
				NS_BAD_FILE = -18,
				//读取文件失败
				NS_READ_FILE_ERROR = -19,
				//数据缓存中数据为空
				NS_DATA_CACHE_NULL = -20,
				//数据缓存中数据已满
				NS_DATA_CACHE_FULL = -21,
				//数据缓存不允许获取空数据
				NS_DATA_CACHE_CANNOT_GET_EMPYTY = -22,
                //空的请求
                NS_HTTP_EMPYTY_REQUEST = -23,
                //http 解析失败
                NS_HTTP_OBJECT_PARSER_ERROR = -24,
                //函数输入参数错误
                NS_PARAM_ERROR = -25,
				//未知错误
				NS_UNKNOW_ERROR = -999,

			};
		
            std::string msg(NS_ERROR_CODE e)
            {
                std::string msg = "未知错误";
                switch (e)
                {
                case ns::util::error::NS_TCP_CONNECTED:
                    msg = "tcp 链接已经建立";
                    break;
                case ns::util::error::NS_TCP_CLOSEED:
                    msg = "tcp 链接已经关闭";
                    break;
                case ns::util::error::NS_TCP_RECVED:
                    msg = "tcp 链接接收到数据";
                    break;
                case ns::util::error::NS_TCP_SENDED:
                    msg = "tcp 链接发送数据完毕";
                    break;
                case ns::util::error::NS_SUCCESS:
                    msg = "操作完成";
                    break;
                case ns::util::error::NS_NEW_BUFF_ERR:
                    msg = "new 缓存失败";
                    break;
                case ns::util::error::NS_TCP_RECV_TIMEOUT:
                    msg = "tcp 接收数据超时";
                    break;
                case ns::util::error::NS_TCP_RECV_ERROR:
                    msg = "tcp 接收数据异常";
                    break;
                case ns::util::error::NS_TCP_SEND_TIMEOUT:
                    msg = "tcp 发送数据超时";
                    break;
                case ns::util::error::NS_TCP_SEND_ERROR:
                    msg = "tcp 发送数据异常";
                    break;
                case ns::util::error::NS_TCP_SERVER_OPEN_ERR:
                    msg = "tcp 服务器启动失败";
                    break;
                case ns::util::error::NS_TCP_SERVER_SET_ERROR:
                    msg = "tcp 服务器参数设置失败";
                    break;
                case ns::util::error::NS_TCP_SERVER_BIND_ERROR:
                    msg = "tcp 服务器地址绑定失败";
                    break;
                case ns::util::error::NS_TCP_SERVER_ACCEPT_ERROR:
                    msg = "tcp 服务器地址接收客户端连接失败";
                    break;
                case ns::util::error::NS_TCP_SERVER_LINK_CLOSED_ERROR:
                    msg = "tcp 服务器地址接收客户端连接错误，得到的链接是无效的";
                    break;
                case ns::util::error::NS_TCP_CLIENT_CONNECT_ERROR:
                    msg = "tcp 客户端连接失败";
                    break;
                case ns::util::error::NS_TCP_CLIENT_CONNECT_TIME_OUT_ERROR:
                    msg = "tcp 客户端连接超时";
                    break;
                case ns::util::error::NS_TCP_CLIENT_CONNECT_CLOSED_ERROR:
                    msg = "tcp 客户端连接异常关闭";
                    break;
                case ns::util::error::NS_TCP_HANDLER_NULL_ERROR:
                    msg = "回调为空";
                    break;
                case ns::util::error::NS_FILE_IS_NOT_EXIST:
                    msg = "目标文件不存在";
                    break;
                case ns::util::error::NS_BAD_RANGE:
                    msg = "错误的请求范围";
                    break;
                case ns::util::error::NS_NOT_OPEN_FILE:
                    msg = "无法打开指定文件";
                    break;
                case ns::util::error::NS_BAD_FILE:
                    msg = "文件异常";
                    break;
                case ns::util::error::NS_READ_FILE_ERROR:
                    msg = "文件读取异常";
                    break;
                case ns::util::error::NS_DATA_CACHE_NULL:
                    msg = "数据缓存队列为空";
                    break;
                case ns::util::error::NS_DATA_CACHE_FULL:
                    msg = "数据缓存队列已满";
                    break;
                case ns::util::error::NS_DATA_CACHE_CANNOT_GET_EMPYTY:
                    msg = "数据缓存队列不允许取size=0大小的数据";
                    break;
                case ns::util::error::NS_HTTP_EMPYTY_REQUEST:
                    msg = "空的http请求";
                    break;
                case ns::util::error::NS_HTTP_OBJECT_PARSER_ERROR:
                    msg = "http 解析错误";
                    break;
                case ns::util::error::NS_PARAM_ERROR:
                    msg = "函数输入的参数错误";
                    break;
                case ns::util::error::NS_UNKNOW_ERROR:
                    break;
                    //NS_HTTP_EMPYTY_REQUEST NS_PARAM_ERROR
                default:
                    break;
                }
            
                return msg;
            }
        }
	}
}
#endif // ! NS_ERROR_HPP_
