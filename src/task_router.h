#ifndef CODEX_BROKER_ROUTER_H
#define CODEX_BROKER_ROUTER_H

#include <vector>
#include <map>
#include <queue>
#include <memory>
#include <spdlog/spdlog.h>

/* Forward declaration */
struct worker;

/**
 * Service that stores information about worker machines and routes tasks to them
 */
class task_router
{
public:
	typedef std::multimap<std::string, std::string> headers_t;
	typedef std::shared_ptr<worker> worker_ptr;
	typedef std::vector<std::string> request_t;

private:
	std::vector<worker_ptr> workers;
	std::shared_ptr<spdlog::logger> logger_;

public:
	task_router(std::shared_ptr<spdlog::logger> logger = nullptr);
	virtual void add_worker(worker_ptr worker);
	virtual worker_ptr find_worker(const headers_t &headers);
	virtual worker_ptr find_worker_by_identity(const std::string &identity);
};

/**
 * A structure that contains data about a worker machine
 */
struct worker {
	/** A unique identifier of the worker */
	const std::string identity;

	/** Headers that describe the worker's capabilities */
	const task_router::headers_t headers;

	/** False if the worker is processing a request */
	bool free;

	/** A queue of requests to be processed by the worker */
	std::queue<task_router::request_t> request_queue;

	worker(const std::string &id, const task_router::headers_t &headers) : identity(id), headers(headers), free(true)
	{
	}
};

#endif // CODEX_BROKER_ROUTER_H
