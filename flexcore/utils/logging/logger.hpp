#ifndef SRC_LOGGING_LOGGER_HPP_
#define SRC_LOGGING_LOGGER_HPP_

#include "flexcore/extended/node_fwd.hpp"
#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <syslog.h>

namespace fc
{

/**
 * \brief Enumeration of severity levels corresponding to the posix syslog api.
 * See man 3 syslog.
 */
enum class level : char
{
	emergency = LOG_EMERG,
	alert = LOG_ALERT,
	critical = LOG_CRIT,
	error = LOG_ERR,
	warning = LOG_WARNING,
	notice = LOG_NOTICE,
	info = LOG_INFO,
	debug = LOG_DEBUG
};

/**
 * \brief A handle that will run the deleter function on destruction.
 * This is used to make sure that a stream that is added to the logger using add_stream_log will be
 * deregistered before it goes out of scope.
 */
class stream_handle
{
public:
	stream_handle(std::function<void()> deleter);

	// moveable, but not copyable
	stream_handle(const stream_handle&) = delete;
	stream_handle(stream_handle&&) = default;
	stream_handle& operator=(const stream_handle&) = delete;
	stream_handle& operator=(stream_handle&&) = default;

	~stream_handle();
private:
	std::function<void()> deleter;
};

/**
 * \brief A class for writing messages to multiple logs backends.
 */
class logger
{
public:
	/// get the singleton instance of logger.
	static logger& get();

	/// add log backends of the specified types
	void add_file_log(std::string filename);
	void add_syslog_log(std::string progname);

	enum class flush { true_ = true, false_ = false };
	enum class cleanup { true_ = true, false_ = false };
	/**
	 * \brief add an ostream backend to the logger.
	 * The reference to the stream will be stored, so make sure that the stream is (either one):
	 *     a) a global.
	 *     b) will be removed from the logger before the stream is destroyed.
	 *
	 * \param stream The stream to which to write.
	 * \param flush Flush to the stream on every log write.
	 * \param cleanup Remove the stream from the logger when the returned stream_handle is destroyed.
	 *
	 * \returns A stream_handle that does (or does not) perform cleanup on destruction.
	 */
	stream_handle add_stream_log(std::ostream& stream, logger::flush flush,
	                             logger::cleanup cleanup);

private:
	logger();
	logger(const logger&) = delete;
	logger& operator=(const logger&) = delete;
};

/**
 * \brief A client of the logger.
 * This class allows to write log messages to the registered logger backends.  The client is not
 * MT-safe but models a value type, so a copy of a client can safely be used in another thread.
 */

class log_client
{
public:
	/// Write msg to the log with the specified severity level.
	void write(const std::string& msg, level = level::info);
	/// Construct a log_client with the region name "null"
	log_client();
	/// Construct a log_client which logs from the passed region.
	log_client(const node* node_);

	log_client(const log_client&);
	log_client& operator=(log_client);
	log_client(log_client&&);
	~log_client();
private:
	class log_client_impl;
	std::unique_ptr<log_client_impl> log_client_pimpl;
};

class stream_log_client

{
public:
	stream_log_client(log_client log, level severity = level::info);

	struct stream_log_proxy
	{
		log_client* log;
		const level severity;
		std::ostringstream ss;

		stream_log_proxy(const std::string& msg, log_client& log, const level severity);
		stream_log_proxy(stream_log_proxy&& other);
		~stream_log_proxy();

		stream_log_proxy& operator<<(const std::string& msg);

	};

	stream_log_proxy operator<<(const std::string& msg);

	static_assert(std::is_move_constructible<stream_log_proxy>::value,
	              "stream_log_proxy should be move constructible");

private:
	log_client log;
	const level severity;
};

inline stream_log_client::stream_log_client(log_client log, level severity)
    : log(std::move(log)), severity(severity)
{
}

inline auto stream_log_client::operator<<(const std::string& msg) -> stream_log_proxy
{
	stream_log_proxy proxy(msg, log, severity);
	return proxy;
}

inline stream_log_client::stream_log_proxy::stream_log_proxy(const std::string& msg,
                                                             log_client& log, const level severity)
    : log(&log), severity(severity)
{
	ss << msg;
}

inline stream_log_client::stream_log_proxy::stream_log_proxy(stream_log_proxy&& other)
    : log(other.log), severity(other.severity), ss(std::move(other.ss))
{
	other.log = nullptr;
}

inline stream_log_client::stream_log_proxy::~stream_log_proxy()
{
	if (log)
		log->write(ss.str(), severity);
}

inline auto stream_log_client::stream_log_proxy::operator<<(const std::string& msg)
    -> stream_log_proxy&
{
	ss << msg;
	return *this;
}


} // namespace fc
#endif // SRC_LOGGING_LOGGER_HPP_

