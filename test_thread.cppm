#include <cstddef>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
 
export module threadpool;

//export struct ThreadPool {};

#if 1

export struct ThreadPool {

	explicit ThreadPool(std::size_t num_threads = std::thread::hardware_concurrency()) {
		for (std::size_t i = 0; i <= num_threads; ++i) {
			threads_.emplace_back([this] {
				while (true) {
					std::function<void()> task;

					{
						std::unique_lock<std::mutex> lock(queue_mutex_);
					
						// ok dans le fond, du moment que le cv_.wait commence, le lock
						// est mis sur pause et les autres threads peuvent y aller.

						// du moment que le .wait se termine, le lock est repris par le thread.

						cv_.wait(lock, [this] {
							return !tasks_.empty() || stop_;
						});

						// du moment que l'on reviens ici, le lock est repris par le thread

						if (stop_ && tasks_.empty()) return;

						task = std::move(tasks_.front());
						tasks_.pop();
					}

					task();
				}
			});
		}
	}

	~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(queue_mutex_);
			stop_ = true;
		}

		cv_.notify_all();

		for (auto& thread : threads_) {
			thread.join();
		}
	}

	void enqueue(std::function<void()> task) {
		{
			std::unique_lock<std::mutex> lock(queue_mutex_);
			tasks_.push(std::move(task));
		}
		cv_.notify_one();
	}

private:
	std::vector<std::thread> threads_;
	std::condition_variable cv_;

	std::queue<std::function<void()>> tasks_;
	std::mutex queue_mutex_;

	bool stop_ = false;
};

#endif
