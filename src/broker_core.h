#ifndef CODEX_BROKER_CORE_H
#define CODEX_BROKER_CORE_H

#include "spdlog/spdlog.h"
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <yaml-cpp/yaml.h>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
namespace fs = boost::filesystem;


// Our very own code includes
#include "broker_connect.h"
#include "config/log_config.h"
#include "config/broker_config.h"
#include "connection_proxy.h"
#include "commands/command_holder.h"


/**
 * Main class of whole program.
 * It handles all creation and destruction of all used parts.
 * And of course run all those parts.
 */
class broker_core
{
public:
	broker_core() = delete;

	broker_core(const broker_core &source) = delete;

	broker_core &operator=(const broker_core &source) = delete;

	broker_core(const broker_core &&source) = delete;

	broker_core &operator=(const broker_core &&source) = delete;

	/**
	 * There is only one constructor, which should get cmd parameters.
	 * Constructor initializes all variables and structures, parses cmd parameters and load configuration.
	 * @param args Cmd line parameters
	 */
	broker_core(std::vector<std::string> args);

	/**
	 * All structures which need to be explicitly destructed or unitialized should do it now.
	 */
	~broker_core();

	/**
	 * Constructors initializes all things,	all we have to do now is launch all the fun.
	 * This method creates separate thread for broker_connection and starts job_evaluator service.
	 */
	void run();

private:
	/**
	 * Setup all things around spdlog logger, creates log path/file if not existing.
	 */
	void log_init();

	/**
	 * Construct and setup broker connection.
	 */
	void broker_init();

	/**
	 * Exit whole application with return code 1.
	 * @param msg string which is copied to stderr and logger if initialized.
	 */
	void force_exit(std::string msg = "");

	/**
	 * Parse cmd line params given in constructor.
	 */
	void parse_params();

	/**
	 * Load default worker configuration from default config file
	 * or from file given in cmd parameters.
	 */
	void load_config();

	// PRIVATE DATA MEMBERS
	/** Cmd line parameters */
	std::vector<std::string> args_;

	/** Filename of default configuration of broker */
	std::string config_filename_;

	/** Loaded broker configuration */
	std::shared_ptr<broker_config> config_;

	/** Pointer to logger */
	std::shared_ptr<spdlog::logger> logger_;

	/** Pointer to task router */
	std::shared_ptr<task_router> task_router_;

	/** Pointer to sockets */
	std::shared_ptr<connection_proxy> sockets_;

	/** Main broker class */
	std::shared_ptr<broker_connect<connection_proxy>> broker_;
};

#endif // CODEX_BROKER_CORE_H
