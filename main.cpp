#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <stdexcept>
#include <string_view>
#include <charconv>

#include "thread_safe_queue.h"

constexpr long long g_main_sleep_ms = 30000;

namespace 
{
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<long long> dist(1, 2000);

	//like stoi
	template <typename T, typename ... Args>
	void SvToVal(const std::string_view str, T& t, Args... args) 
	{
		std::from_chars_result res = std::from_chars(str.data(), str.data() + str.size(), t, args...);

		if (res.ec == std::errc::invalid_argument)
			throw std::invalid_argument{ "invalid_argument" };
		else if (res.ec == std::errc::result_out_of_range)
			throw std::out_of_range{ "out_of_range" };
	}

	class Stopper final
	{
	public:
		void request_stop() noexcept
		{
			stop_ = true;
		}
		bool stop_requested() const noexcept
		{
			return stop_;
		}
	private:
		volatile bool stop_ = false;
	};

	Request* GetRequest(Stopper& stopSignal)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
		if (stopSignal.stop_requested())
			return NULL;

		std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
		try 
		{
			return new Request();
		}
		catch (const std::bad_alloc& e)
		{
			std::cerr << e.what() << '\n';
			return NULL;
		}
		
	}

	void ProcessRequest(Request* request, Stopper& stopSignal)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
	}

	void Produce(ThreadSafeQueue& queue, Stopper& stopSignal)
	{
		while (!stopSignal.stop_requested())
		{
			std::unique_ptr<Request> request(GetRequest(stopSignal));
			queue.push(request);
		};
	}

	void Consume(ThreadSafeQueue& queue, Stopper& stopSignal)
	{
		while (!stopSignal.stop_requested())
		{
			std::unique_ptr<Request> request = nullptr;
			queue.pop(request);
			ProcessRequest(request.get(), stopSignal);
		};
	}
}


int main(int argc, char** argv) try
{

	int consumers_cnt = 2, producers_cnt = 2;

	if (argc == 5)
	{
		const std::string_view c_arg = argv[1];
		const std::string_view p_arg = argv[3];
		if (c_arg != "-c" || p_arg != "-p")
			throw std::invalid_argument("Invalid args: expect -c [NUM] -p [NUM]");
		const std::string_view c = argv[2];
		const std::string_view p = argv[4];

		SvToVal(c, consumers_cnt);
		SvToVal(p, producers_cnt);
	}
	else
		std::cout << "producers and consumers count setted to default value = 2";

	ThreadSafeQueue queue;
	std::vector<std::thread> producers;
	std::vector<std::thread> consumers;
	Stopper stop_signal;

	for (int N{}; N < consumers_cnt; ++N)
		producers.emplace_back(Produce, std::ref(queue), std::ref(stop_signal));

	for (int N{}; N < producers_cnt; ++N)
		consumers.emplace_back(Consume, std::ref(queue), std::ref(stop_signal));

	std::this_thread::sleep_for(std::chrono::milliseconds(g_main_sleep_ms));
	stop_signal.request_stop();
	queue.done();
	for (auto& p : producers)
		p.join();

	for (auto& c : consumers)
		c.join();
	return EXIT_SUCCESS;
}
catch (const std::exception& e)
{
	std::cerr << e.what() << '\n';
	return EXIT_FAILURE;
}
