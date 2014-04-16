/*
*
*	Author: Philipp Zschoche, https://zschoche.org
*
*/
#ifndef __SQLITE_HPP__
#define __SQLITE_HPP__

#include "sqlite3/sqlite3.h"
#include <boost/format.hpp>
#include "handle.hpp"

namespace sqlite {


enum datatype {
	INTEGER = 1,
	FLOAT = 2,
	TEXT = 3,
	BLOB = 4,
	NULL_TYPE = 5
};

struct connection_handle_traits {
	using pointer = sqlite3 *;

	static pointer invalid() noexcept { return nullptr; }

	static bool close(pointer value) noexcept { return sqlite3_close(value) == SQLITE_OK; }
};
struct statement_handle_traits {
	using pointer = sqlite3_stmt *;

	static pointer invalid() noexcept { return nullptr; }

	static bool close(pointer value) noexcept { return sqlite3_finalize(value) == SQLITE_OK; }
};

using connection_handle = utils::unique_handle<connection_handle_traits>;
using statement_handle = utils::unique_handle<statement_handle_traits>;


struct sql_error : std::exception {
	int code;
	std::string message;
	sql_error(int _code, std::string _message)
	:code(std::move(_code)), message(std::move(_message)) {}

	const char* what() const noexcept override  {
		return message.c_str();
	}

};

struct sql_result {
	int value;

	static inline sql_result from_cmd(int value,const connection_handle& handle) {
		if (value == SQLITE_OK) {
			return sql_result(value);
		} else {
			return sql_result(value, sql_error(value, sqlite3_errmsg(handle.get())));
		} 
	}
	static inline sql_result from_cmd(int value,const statement_handle& handle) {
		if (value == SQLITE_OK) {
			return sql_result(value);
		} else {
			return sql_result(value, sql_error(value, sqlite3_errmsg(sqlite3_db_handle(handle.get()))));
		} 
	}

	static inline sql_result from_stmt(int value,const statement_handle& handle) {
		if (value == SQLITE_ROW || value == SQLITE_DONE) {
			return sql_result(value);
		} else {
			return sql_result(value, sql_error(value, sqlite3_errmsg(sqlite3_db_handle(handle.get()))));
		} 
	}

	sql_result(sql_result& rhs) :value(rhs.value),m_saw_error(rhs.m_saw_error),m_err(rhs.m_err) {
		if(!m_saw_error) {
			rhs.m_saw_error = true;
		}
	}

	sql_result(sql_result&& rhs): value(std::move(rhs.value)),m_saw_error(std::move(rhs.m_saw_error)),m_err(std::move(rhs.m_err)) {}

	sql_result& operator=(sql_result& rhs) {
		value = rhs.value;
		m_err = rhs.m_err;
		m_saw_error = rhs.m_saw_error;
		if(!m_saw_error) {
			rhs.m_saw_error = true;
		}
		return *this;
	}

	sql_result& operator=(sql_result&& rhs) {
		value = std::move(rhs.value);
		m_err = std::move(rhs.m_err);
		m_saw_error = std::move(rhs.m_saw_error);
		return *this;

	}
	 ~sql_result() {
	 	if(!m_saw_error) {
			throw *m_err;
		}
	 }


	inline bool has_data() const noexcept { return value == SQLITE_ROW; }
	inline bool ok() const noexcept { return value == SQLITE_OK; }
	inline bool done() const noexcept { return value == SQLITE_DONE; }

	void throw_error() { m_saw_error = true; throw *m_err; }

	inline const boost::optional<sql_error>& error() noexcept { 
		m_saw_error = true; 
		return m_err; 
	}
	
	explicit operator bool() const {
		return has_data() && ok() && done(); // should be okay in the context
	}

	private:
	bool m_saw_error;
	boost::optional<sql_error> m_err;
	
	sql_result(int _value) : value(std::move(_value)), m_saw_error(true) {}
	sql_result(int _value, sql_error err) : value(std::move(_value)), m_saw_error(false),m_err(std::move(err)) {}

};

struct connection {

	connection_handle handle;

	static inline connection create(const char *connection_string) {
		connection result;
		result.open(connection_string);
		return result;
	}

	static inline connection create_memory() {
		connection result;
		result.open(":memory:");
		return result;
	}


	inline sql_result execute(const std::string& str) const {
		const auto r = sqlite3_exec(handle.get(), str.c_str(), nullptr, nullptr, nullptr);
		return sql_result::from_cmd(r, handle);
	}


	std::string get_current_error() const {
		return std::string(sqlite3_errmsg(handle.get()));
	}
 

	sql_result open(const std::string& filename) {
		auto local = connection_handle{};
		auto const r = sqlite3_open(filename.c_str(), local.get_address_of());
		handle = std::move(local);
		return sql_result::from_cmd(r, handle);
	}

};

struct statement {
	statement_handle handle;

	template<typename Connection>
	static inline statement create(Connection&& c, std::string sql) {
		statement result;
		result.prepare(std::forward<Connection>(c), std::move(sql));
		return result;
	}


	template<typename Connection>
	sql_result prepare(Connection&& c, const std::string& text) {
		handle.reset();
		const auto r = sqlite3_prepare_v2(c.handle.get(), text.c_str(), text.size(), handle.get_address_of(), nullptr);
		return sql_result::from_cmd(r, handle);
	}
	


	std::string get_current_error() const {
		return std::string(sqlite3_errmsg(sqlite3_db_handle(handle.get())));
	}		

	template<typename T>
	statement& bind(const int index, const T& value) {
		create_binding(index, value);
		return *this;
	}

	inline sql_result create_binding(const int index, const unsigned int value) {
		const int r = sqlite3_bind_int(handle.get(), index, value);
		return sql_result::from_cmd(r, handle);
	}
	inline sql_result create_binding(const int index, const int32_t value) {
		const int r = sqlite3_bind_int(handle.get(), index, value);
		return sql_result::from_cmd(r, handle);
	}

	inline sql_result create_binding(const int index, const unsigned long value) {
		const int r = sqlite3_bind_int64(handle.get(), index, value);
		return sql_result::from_cmd(r, handle);
	}


	inline sql_result create_binding(const int index, const int64_t value) {
		const int r = sqlite3_bind_int64(handle.get(), index, value);
		return sql_result::from_cmd(r, handle);
	}

	inline sql_result create_binding(const int index, const double value) {
		const int r = sqlite3_bind_double(handle.get(), index, value);
		return sql_result::from_cmd(r, handle);
	}

	// binds null
	inline sql_result create_binding(const int index) {
		const int r = sqlite3_bind_null(handle.get(), index);
		return sql_result::from_cmd(r, handle);
	}

	inline sql_result create_binding(const int index, sqlite3_value *value) {
		const int r = sqlite3_bind_value(handle.get(), index, value);
		return sql_result::from_cmd(r, handle);
	}

	inline sql_result create_binding(int const index, const char* value) {
		const int r = sqlite3_bind_text(handle.get(), index, value, strlen(value), SQLITE_STATIC);
		return sql_result::from_cmd(r, handle);

	}

	inline sql_result create_binding(int const index, const std::string &value) {
		const int r = sqlite3_bind_text(handle.get(), index, value.c_str(), value.size(), SQLITE_STATIC);
		return sql_result::from_cmd(r, handle);
	}

	inline sql_result create_binding_zeroblob(const int index, int size) {
		const int r = sqlite3_bind_zeroblob(handle.get(), index, size);
		return sql_result::from_cmd(r, handle);
	}


	inline sql_result create_binding_by_copy(int const index, const std::string &value) {
		const int r = sqlite3_bind_text(handle.get(), index, value.c_str(), value.size(), SQLITE_TRANSIENT);
		return sql_result::from_cmd(r, handle);
	}

	inline sql_result create_binding_blob(int const index, const void *blob, int size) {
		const int r = sqlite3_bind_blob(handle.get(), index, blob, size, SQLITE_STATIC);
		return sql_result::from_cmd(r, handle);
	}

	inline sql_result create_binding_by_copy(int const index, const void *blob, int size) {
		const int r = sqlite3_bind_blob(handle.get(), index, blob, size, SQLITE_TRANSIENT);
		return sql_result::from_cmd(r, handle);
	}

	inline void reset_binding() {
		 sqlite3_reset(handle.get());
		 sqlite3_clear_bindings(handle.get());
	}

	inline const statement& step_dirty() {
		const auto result = sql_result::from_stmt(sqlite3_step(handle.get()), handle);
		return *this;
	}

	inline sql_result step() const {
		const int r = sqlite3_step(handle.get());
		return sql_result::from_stmt(r, handle);

	}

	inline int64_t rowid() const { return sqlite3_last_insert_rowid(sqlite3_db_handle(handle.get())); }
	inline int64_t get_int64(const int column = 1) const { return sqlite3_column_int64(handle.get(), column); }
	inline int32_t get_int(const int column = 1) const { return sqlite3_column_int(handle.get(), column); }
	inline double  get_double(const int column = 1) const { return sqlite3_column_double(handle.get(), column); }
	inline std::string get_string(const int column = 1) const { return std::string(reinterpret_cast<const char*>(sqlite3_column_text(handle.get(), column))); }
	inline sqlite3_value* get_value(const int column = 1) const { return sqlite3_column_value(handle.get(), column); }
	inline datatype get_type(const int column = 1) const { return static_cast<datatype>(sqlite3_column_type(handle.get(), column)); }



	//not sure what i should do with them.
	//int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
	//const void *sqlite3_column_blob(sqlite3_stmt*, int iCol);
};

} /* namespace sqlite  */

#endif /* __SQLITE_HPP__ */
