#ifndef CODEX_BROKER_CLIENT_COMMANDS_H
#define CODEX_BROKER_CLIENT_COMMANDS_H

#include "../helpers/string_to_hex.h"

namespace client_commands
{

	/**
	 * Process an "eval" request from a client
	 */
	template <typename proxy>
	void process_eval(
		const std::string &identity, const std::vector<std::string> &message, const command_context<proxy> &context)
	{
		std::string job_id = message.at(1);
		task_router::headers_t headers;

		// Load headers terminated by an empty frame
		auto it = std::begin(message) + 2;

		while (true) {
			// End of headers
			if (it->size() == 0) {
				++it;
				break;
			}

			// Unexpected end of message - do nothing and return
			if (std::next(it) == std::end(message)) {
				context.logger->warn() << "Unexpected end of message from frontend. Skipped.";
				return;
			}

			// Parse header, save it and continue
			size_t pos = it->find('=');
			size_t value_size = it->size() - (pos + 1);

			headers.emplace(it->substr(0, pos), it->substr(pos + 1, value_size));
			++it;
		}

		task_router::worker_ptr worker = context.router->find_worker(headers);

		if (worker != nullptr) {
			std::vector<std::string> request = {"eval", job_id};
			context.logger->debug() << " - incomming job '" << job_id << "'";

			// Forward remaining messages to the worker without actually understanding them
			for (; it != std::end(message); ++it) {
				request.push_back(*it);
			}

			if (worker->free) {
				// If the worker isn't doing anything, just forward the request
				worker->free = false;
				context.sockets->send_workers(worker->identity, request);
				context.logger->debug() << " - sent to worker '"
										<< helpers::string_to_hex(worker->identity) << "'";
			} else {
				// If the worker is occupied, queue the request
				worker->request_queue.push(request);
				context.logger->debug() << " - saved to queue for worker '"
										<< helpers::string_to_hex(worker->identity) << "'";
			}

			context.sockets->send_clients(identity, std::vector<std::string>{"accept"});
			context.router->deprioritize_worker(worker);
		} else {
			context.sockets->send_clients(identity, std::vector<std::string>{"reject"});
			context.logger->error() << "Request '" << job_id << "' rejected. No worker available.";
		}
	}
} // namespace


#endif // CODEX_BROKER_CLIENT_COMMANDS_H
