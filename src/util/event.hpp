/*
	事件系统

*/
#ifndef NS_EVENT_H_
#define NS_EVENT_H_

#include <thread>
#include <mutex>
#include <list>
#include <future>
#include <chrono>
#include <map>
#include <unordered_map>
#include <condition_variable>
#include <ctime>


//#include <time.h>
namespace ns
{
	namespace event
	{
        typedef long timer_id;
        typedef clock_t timer_ms;
        typedef std::function<void(timer_id id)> timer_out_handler;

        

        //计时器
        struct Timer
        {
            //计时器id
            timer_id id;
            //到时时间
            timer_ms time_out_ms;
            //触发句柄
            timer_out_handler handler;

        };
        typedef std::shared_ptr<Timer> shared_timer;

		class Event
		{
			typedef std::thread run_thread_item;
			typedef  std::function<void()> task_func;
			
		public:
			//单例
			static Event& instance()
			{
				static Event e;
				return e;
				
			}
		public:
			template<typename Func, typename ...Types>
			void async_void(Func func, Types&& ... args)
			{
				post(std::bind(func, std::forward<Types>(args)...));
			}
			
            template<typename Func, typename ...Types>
            void async_void_first(Func func, Types&& ... args)
            {
                post_first(std::bind(func, std::forward<Types>(args)...));
            }

            // 添加新的任务到任务队列
            template<class F, class... Args>
            auto async(F&& f, Args&& ... args)
                -> std::future<typename std::result_of<F(Args...)>::type>
            {
                // 获取函数返回值类型        
                using return_type = typename std::result_of<F(Args...)>::type;

                // 创建一个指向任务的只能指针
                auto task = std::make_shared< std::packaged_task<return_type()> >(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                    );

                std::future<return_type> res = task->get_future();
                //推送
                post([task]() { (*task)(); });
                return res;
            }

            // 添加新的任务到优先
            template<class F, class... Args>
            auto async_first(F&& f, Args&& ... args)
                -> std::future<typename std::result_of<F(Args...)>::type>
            {
                // 获取函数返回值类型        
                using return_type = typename std::result_of<F(Args...)>::type;

                // 创建一个指向任务的只能指针
                auto task = std::make_shared< std::packaged_task<return_type()> >(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                    );

                std::future<return_type> res = task->get_future();
                //推送
                post_first([task]() { (*task)(); });
                return res;
            }

            size_t cpu_num() 
			{
				return std::thread::hardware_concurrency()*2+2;
			}

            //添加计时器
            timer_id add_timer(timer_ms timeout, timer_out_handler handler)
            {
                if (timeout <= 0 || !handler)
                    return -1;
                
                auto t = std::make_shared<Timer>();
                t->time_out_ms = clock()+timeout;
                if (t->time_out_ms < 0)
                {
                    printf("t->time_out_ms < 0\n");
                    return -1;
                }
                t->handler = handler;

                {
                    std::unique_lock<std::mutex> lock(timer_map_lock_);
                    t->id = round_id_;
                    timer_map_id_.emplace(t->id, t);
                    timer_map_ms_.emplace(t->time_out_ms, t);
                    ++round_id_;
                }
                return t->id;
                //Timer
            }
            //移除计时器
            void del_timer(timer_id id)
            {
                std::unique_lock<std::mutex> lock(timer_map_lock_);
                //timer_map_.erase(id);
                auto t = timer_map_id_.find(id);
                if (t == timer_map_id_.end())
                    return;

                auto p = timer_map_ms_.equal_range(t->second->time_out_ms);
                for (auto ptr = p.first; ptr != p.second; ++ptr)
                {
                    if (ptr->second->id = id)
                    {
                        timer_map_ms_.erase(ptr);
                        break;
                    }
                }
                timer_map_id_.erase(t);

                //timer_ma
            }
            
            void set_timer_check(timer_ms ms)
            {
                timer_check_ = ms;
            }
            
            ~Event()
			{
                exit_flag_ = true;
				run_flag_ = false;
                list_condition_.notify_all();
				for (auto& t : run_thread_vector_)
				{
					if (t.joinable())
						t.join();
				}
				run_thread_vector_.clear();

                if (timer_update_thread_.joinable())
                    timer_update_thread_.join();
			}
		private:
			Event():round_id_(0), last_clock_(0), timer_check_(1)
			{
                exit_flag_ = false;
				run_flag_ = true;
                //async_void(&Event::updata_time, this, 5)-1;
				auto cpu_num =  std::thread::hardware_concurrency()*2+2-1;
				//LOG_DBG <<"hardware_concurrency = "<<cpu_num;
				for (size_t i = 0; i < cpu_num; ++i)
				{
					run_thread_vector_.push_back(std::thread(&Event::worker, this));
				}

                std::thread t([this]() {
                    while (!exit_flag_)
                    {
                        try
                        {
                            updata_time(timer_check_);
                        }
                        catch (std::exception& e)
                        {
                            printf("[Event] updata_time err,msg:%s\n", e.what());
                        }
                        

                        while (!exit_flag_ && !run_flag_)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(timer_check_));
                        }
                    }
                    });

                timer_update_thread_.swap(t);
			}
            void start()
            {
                run_flag_ = true;
                list_condition_.notify_all();
            }
            void stop()
            {
                run_flag_ = false;
                list_condition_.notify_all();
            }
			void post(task_func func)
			{
                std::unique_lock<std::mutex> lock(list_lock_);
                task_list_.push_back(std::move(func));
                list_condition_.notify_one();
			}
            void post_first(task_func func)
            {
                std::unique_lock<std::mutex> lock(list_lock_);
                task_list_.push_front(std::move(func));
                list_condition_.notify_one();
            }
			void worker()
			{
				while (!exit_flag_)
				{
                    // task是一个函数类型，从任务队列接收任务
                    task_func task;

                    {
                        printf("worker is lock!\n");
                        //给互斥量加锁，锁对象生命周期结束后自动解锁
                        std::unique_lock<std::mutex> lock(list_lock_);

                        //（1）当匿名函数返回false时才阻塞线程，阻塞时自动释放锁。
                        //（2）当匿名函数返回true且受到通知时解阻塞，然后加锁。
                        list_condition_.wait(lock, \
                            [this] 
                            {
                                //已退出
                                if (exit_flag_)
                                    return true;
                                //未退出 且 任务链表不为空 且在运行
                                if (!task_list_.empty()&&run_flag_)
                                    return true;
                                return false;
                            });

                        if (exit_flag_ && task_list_.empty())
                            break;

                        //从任务队列取出一个任务
                        task = std::move(task_list_.front());
                        task_list_.pop_front();
                        printf("worker is unlock!\n");
                        //printf("[Event] task_list_ size %d\n", task_list_.size());
                    }                           
                    // 自动解锁
                    //printf("test in \n");
                    try
                    {
                        printf("[Event] run task in\n");
                        task();                      // 执行这个任务
                        printf("[Event] run task out\n");
                    }
                    catch (std::exception &e)
                    {
                        printf("[Event] run task err,msg:%s\n",e.what());
                    }
                    //printf("test out \n");
				}
			
                printf("worker is exit!\n");
            }
            //检查间隔
            void updata_time(timer_ms check_time_ms = 5)
            {
                timer_ms sleep_time = check_time_ms;
                //更新时间
                {
                    std::unique_lock<std::mutex> lock(timer_map_lock_);
                    auto now = clock();
                    if (last_clock_ != 0)
                        sleep_time -= (now - last_clock_);
                    last_clock_ = now;
                    //取出超时事件
                    auto pend = timer_map_ms_.upper_bound(now);
                    auto p = timer_map_ms_.begin();
                    while (p != pend)
                    {
                        auto t = p->second;
                        async_void_first(t->handler, t->id);
                        //t->handler(t->id);
                        p = timer_map_ms_.erase(p);
                        timer_map_id_.erase(t->id);

                    }
                }

                if(sleep_time>0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));

               // async_void(&Event::updata_time, this, check_time_ms);
            }
        private:
			//工作线程
			std::vector<run_thread_item> run_thread_vector_;
			
            std::atomic_bool exit_flag_;
            std::atomic_bool run_flag_;

            //任务列表
            //锁
            std::mutex list_lock_;
            //条件变量 
            std::condition_variable list_condition_;             
            //任务队列
            std::list<task_func> task_list_;

            //定时器线程
            //锁
            std::mutex timer_map_lock_;
            //计时id 自增
            timer_id round_id_;
            //上次更新时间
            timer_ms last_clock_;
            //时间更新周期
            timer_ms timer_check_;
            //更新线程
            run_thread_item timer_update_thread_;
            //计时器集合
            std::unordered_map<timer_id, shared_timer> timer_map_id_;
            std::multimap<timer_ms, shared_timer> timer_map_ms_;

		};
	}

}

#define NS_EVENT ns::event::Event::instance()
#define NS_EVENT_ASYNC_VOID(...) NS_EVENT.async_void(##__VA_ARGS__)
#define NS_EVENT_ASYNC(...) NS_EVENT.async(##__VA_ARGS__)
#define NS_ADD_TIMER(ms,handler) NS_EVENT.add_timer(ms,handler)
#define NS_DEL_TIMER(id) NS_EVENT.del_timer(id)
#define NS_CPU_NUM NS_EVENT.cpu_num()

#endif
