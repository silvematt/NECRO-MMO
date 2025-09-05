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

class DBRequest
{
public:
	bool										m_done = false;
	int											m_enumVal;		// Enum val passed to Database.Prepare, to identify the query to call
	std::vector<mysqlx::Value>					m_bindParams;	// Params to bind to the query
	bool										m_fireAndForget;// Fire and forget requests do not require to capture SqlRes or callbacks

	// For requests that are not fire-and-forget, we need to capture the sql result and execute a callback that will process it and 
	// let more code be executed. DB request->DB Callback
	mysqlx::SqlResult							m_sqlRes;
	boost::asio::io_context&					m_callbackContexRef; // the io_context that should execute the callback. Server::DBCallbackCheckHandler will post the callback function to this context
	std::function<bool(mysqlx::SqlResult&)>		m_callback;

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

	DBRequest(int enumVal, boost::asio::io_context& io, bool fireAndForget) : m_enumVal(enumVal), m_callbackContexRef(io), m_fireAndForget(fireAndForget), m_cancelToken(std::nullopt), m_creationTime(std::chrono::steady_clock::now())
	{
		m_done = false;
		m_callback = nullptr;
		m_noticeFunc = nullptr;
	}
};

#endif
