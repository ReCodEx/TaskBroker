#include "broker_core.h"

broker_core::broker_core(std::vector<std::string> args)
	: args_(args), config_filename_("config.yml"), logger_(nullptr), broker_(nullptr)
{
	// parse cmd parameters
	parse_params();
	// load configuration from yaml file
	load_config();
	// initialize logger
	log_init();
	// initialize curl
	curl_init();
	// notifier init
	notifier_init();
	// construct and setup broker connection
	broker_init();
}

broker_core::~broker_core()
{
	// curl finalize
	curl_fini();
}

void broker_core::run()
{
	logger_->info() << "Broker will now start brokering.";
	broker_->start_brokering();
	logger_->info() << "Broker will now end.";
	return;
}

void broker_core::parse_params()
{
	using namespace boost::program_options;

	// Declare the supported options.
	options_description desc("Allowed options for broker");
	desc.add_options()("help,h", "Writes this help message to stderr")(
		"config,c", value<std::string>(), "Set default configuration of this program");

	variables_map vm;
	try {
		store(command_line_parser(args_).options(desc).run(), vm);
		notify(vm);
	} catch (std::exception &e) {
		force_exit("Error in loading a parameter: " + std::string(e.what()));
	}


	// Evaluate all information from command line
	if (vm.count("help")) {
		std::cerr << desc << std::endl;
		force_exit();
	}

	if (vm.count("config")) {
		config_filename_ = vm["config"].as<std::string>();
	}

	return;
}

void broker_core::load_config()
{
	try {
		YAML::Node config_yaml = YAML::LoadFile(config_filename_);
		config_ = std::make_shared<broker_config>(config_yaml);
	} catch (std::exception &e) {
		force_exit("Error loading config file: " + std::string(e.what()));
	}

	return;
}

void broker_core::force_exit(std::string msg)
{
	// write to log
	if (msg != "") {
		if (logger_ != nullptr) {
			logger_->emerg() << msg;
		}
		std::cerr << msg << std::endl;
	}

	exit(1);
}

void broker_core::log_init()
{
	auto log_conf = config_->get_log_config();

	// Set up logger
	// Try to create target directory for logs
	auto path = fs::path(log_conf.log_path);
	try {
		if (!fs::is_directory(path)) {
			fs::create_directories(path);
		}
	} catch (fs::filesystem_error &e) {
		std::cerr << "Logger: " << e.what() << std::endl;
		throw;
	}

	// Create and register logger
	try {
		// Create multithreaded rotating file sink. Max filesize is 1024 * 1024 and we save 5 newest files.
		auto rotating_sink =
			std::make_shared<spdlog::sinks::rotating_file_sink_mt>((path / log_conf.log_basename).string(),
				log_conf.log_suffix,
				log_conf.log_file_size,
				log_conf.log_files_count,
				true);
		// Set queue size for asynchronous logging. It must be a power of 2.
		spdlog::set_async_mode(1048576);
		// Make log with name "logger"
		logger_ = std::make_shared<spdlog::logger>("logger", rotating_sink);
		// Set logging level to debug
		logger_->set_level(helpers::get_log_level(log_conf.log_level));
		// Print header to log
		if (helpers::compare_log_levels(spdlog::level::notice, logger_->level()) > 0) {
			logger_->emerg() << "--- Started ReCodEx broker ---";
		} else {
			logger_->notice() << "------------------------------";
			logger_->notice() << "    Started ReCodEx broker";
			logger_->notice() << "------------------------------";
		}
	} catch (spdlog::spdlog_ex &e) {
		std::cerr << "Logger: " << e.what() << std::endl;
		throw;
	}

	return;
}

void broker_core::broker_init()
{
	logger_->info() << "Initializing broker connection...";
	workers_ = std::make_shared<worker_registry>();
	sockets_ = std::make_shared<connection_proxy>();
	broker_ = std::make_shared<broker_connect<connection_proxy>>(config_, sockets_, workers_, logger_);
	logger_->info() << "Broker connection initialized.";

	return;
}

void broker_core::curl_init()
{
	// Globally init curl library

	logger_->info() << "Initializing CURL...";
	curl_global_init(CURL_GLOBAL_DEFAULT);
	logger_->info() << "CURL initialized.";

	return;
}

void broker_core::curl_fini()
{
	// Clean after curl library
	logger_->info() << "Cleanup after CURL...";
	curl_global_cleanup();
	logger_->info() << "CURL cleaned.";

	return;
}

void broker_core::notifier_init()
{
	status_notifier_ = std::make_shared<http_status_notifier>(config_->get_frontend_config());
}
