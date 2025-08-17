#ifndef NECRO_DB_REQUEST_H
#define NECRO_DB_REQUEST_H

#include <cstdint>
#include <memory>
#include <functional> 
#include <variant>
#include <string>
#include <optional>

#include <mysqlx/xdevapi.h>

class DBRequest
{
public:
	bool										m_done = false;
	int											m_enumVal;
	std::vector<mysqlx::Value>					m_bindParams;
	bool										m_fireAndForget;

	mysqlx::SqlResult							m_sqlRes;
	std::function<bool(mysqlx::SqlResult&)>		m_callback;
	std::function<void()>						m_noticeFunc;

	std::optional<std::weak_ptr<void>>			m_cancelToken;

	DBRequest(int enumVal, bool fireAndForget) : m_enumVal(enumVal), m_fireAndForget(fireAndForget)
	{
		m_done = false;
		m_callback = nullptr;
		m_noticeFunc = nullptr;
	}
};

#endif
