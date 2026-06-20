#include <cstddef>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>

export module threadpool;

export struct ThreadPool {

	explicit ThreadPool(std::size_t num_threads = std::thread::hardware_concurrency()) {

		for (std::size_t i = 0; i <= num_threads; ++i) {



			threads_.emplace_back([this] {

				while (true) {

					std::function<void> task;

					{
						std::unique_lock<std::mutex> lock(queue_mutex_);
					
					
						// ok dans le fond, du moment que le cv_.wait commence, le lock
						// est mis sur pause et les autres threads peuvent y aller.

						// du moment que le .wait se termine, le lock est repris par le thread.



					}




				}



			});



		}


	}

	void enqueue(std::function<void()> task) {

	}


	void test() {

		// faire une queue de tasks

		// faire une pool de threads WORKERS

		// faire un mutex et lautre truc de sleep

		// for each (i = nombre de threads std::...)

		// on lock et on essaye de prendre le top de la queue si !empty()
		// on std::move et on .pop(), ensuite, on retire le lock (scope)

		// reste


	}


private:

	std::vector<std::thread> threads_;

	std::queue<std::function<void>> tasks_;
	std::mutex queue_mutex_;

	std::condition_variable cv_;

	bool stop_ = false;

};