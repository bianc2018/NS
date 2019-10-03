/*
    “Ï≤Ωioª˘¿‡
*/
#ifndef ASYNC_IO_HPP_
#define ASYNC_IO_HPP_
namespace ns
{
    namespace event
    {
        enum IO_EVENT_TYPE
        {
            IO_READ_EVENT = 1,
            IO_WRITE_EVENT = 2,
            IO_TIMEOUT_EVENT = 3,
            IO_ERROR_EVENT = 3,
        };

        class AsyncIO
        {
        public:
            virtual bool init()=0;
            virtual bool addconn() = 0;
            virtual bool removeconn() = 0;
        };
    }
}
#endif