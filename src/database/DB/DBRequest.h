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
	int											m_enumVal;
	std::vector<mysqlx::Value>					m_bindParams;
	bool										m_fireAndForget;

	mysqlx::SqlResult							m_sqlRes;
	boost::asio::io_context&					m_callbackContexRef;
	std::function<bool(mysqlx::SqlResult&)>		m_callback;
	std::function<void()>						m_noticeFunc;

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
