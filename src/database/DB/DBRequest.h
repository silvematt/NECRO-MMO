#ifndef NECRO_DB_REQUEST_H
#define NECRO_DB_REQUEST_H

#include <cstdint>
#include <memory>
#include <functional> 
#include <variant>
#include <string>
#include <optional>

#include <mysqlx/xdevapi.h>
#include <boost/asio.hpp>

// ------------------------------------------------------------------------------------------------------------------------------------------
// To Support transactions, a DBRequest can be composed of multiple steps
// ------------------------------------------------------------------------------------------------------------------------------------------
struct DBRequestStep
{
	uint32_t									m_enumVal;		// Enum val passed to Database.Prepare, to identify the query to call
	std::vector<mysqlx::Value>					m_bindParams;	// Params to bind to the query
};

// ------------------------------------------------------------------------------------------------------------------------------------------
// DBRequest is a container for a request a socket (or client, or however we want to call it) can make.
// ------------------------------------------------------------------------------------------------------------------------------------------
class DBRequest
{
public:
	uint32_t									m_errorCode = 0;

	bool										m_done = false;
	std::vector<DBRequestStep>					m_steps;

	bool										m_committed = false;
	bool										m_fireAndForget;// Fire and forget requests do not require to capture SqlRes or callbacks, like a databse logging request

	// For requests that are not fire-and-forget, we need to capture the sql result and execute a callback that will process it and 
	// let more code be executed. DB request->DB Callback
	std::vector<mysqlx::SqlResult>										m_sqlResults; // Store one for each step
	boost::asio::io_context&											m_callbackContexRef; // the io_context that should execute the callback. This context is the same context that was used by the socket that made this DBRequest. Server::DBCallbackCheckHandler will post the callback function to this context
	std::function<bool(uint32_t ec, std::vector<mysqlx::SqlResult>&)>	m_callback; // One callback for the whole transaction, with all the results from all the steps as parameter

	// A notice function allows to call code in the DB thread as soon as this DBRequest is executed and it's been put on the respQueue.
	// For that reason, it can be executed before/after the function's callback.
	// It was used in the AuthLegacy code to wake up the main thread from the poll()
	std::function<void()>						m_noticeFunc;

	// The cancelToken allows us skip executing the query if the socket that initiated the request died/got kicked. 
	// If cancel token is set and is expired (initiator destroyed), the request will be ignored.
	// If cancel token is set and not expired, the request will be executed.
	// If cancel token is never set, the request will always be executed.
	std::optional<std::weak_ptr<void>>			m_cancelToken;

	std::chrono::steady_clock::time_point		m_creationTime;

	DBRequest(boost::asio::io_context& io, bool fireAndForget) : m_callbackContexRef(io), m_fireAndForget(fireAndForget), m_cancelToken(std::nullopt), m_creationTime(std::chrono::steady_clock::now())
	{
		m_done = false;
		m_callback = nullptr;
		m_committed = false;
		m_noticeFunc = nullptr;
		m_errorCode = 0;
	}

	bool IsTransaction()
	{
		return m_steps.size() > 1;
	}

	bool IsValid()
	{
		return !m_steps.empty();
	}
};

#endif
