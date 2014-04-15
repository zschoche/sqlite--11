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
#include "expected.hpp"

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

struct sql_exception : std::runtime_error {
	int code;
	std::string message;
	std::string query;

	sql_exception(int const result, std::string text)
	    : std::runtime_error(boost::str(boost::format("(%1%): %2%") % result % text)), code(result), message(std::move(text)) {}

	sql_exception(int const result, std::string text, std::string _query)
	    : std::runtime_error(boost::str(boost::format("(%1%): '%2%' on query %3%") % result % text % query)), code(result), message(std::move(text)), query(std::move(_query)) {}
};

inline sql_exception create_error(const int result, const connection_handle& handle) {
		std::string message(sqlite3_errmsg(handle.get()));
		return sql_exception(result, std::move(message));
}
inline sql_exception create_error(const int result, const statement_handle& handle) {
		std::string message(sqlite3_errmsg(sqlite3_db_handle(handle.get())));
		return sql_exception(result, std::move(message));
}

template<typename T>
inline expected<int> return_sqlite_value(int result, T&& handle) {
	if (result == SQLITE_OK) {
		return expected<int>(result);
	} else {
		return expected<int>::from_exception(create_error(result, std::forward<T>(handle)));
	}
}

inline void verify_result(const int result, const connection_handle &handle) {
	if (result != SQLITE_OK) {
		std::string message(sqlite3_errmsg(handle.get()));
		throw sql_exception(result, std::move(message));
	}
}

template<typename T>
inline void verify_result(const int result, const connection_handle &handle, T&& query) {
	if (result != SQLITE_OK) {
		std::string message(sqlite3_errmsg(handle.get()));
		throw sql_exception(result, std::move(message), std::forward<T>(query));
	}
}

inline void verify_result(const int result, const statement_handle &handle) {
	if (result != SQLITE_OK) {
		std::string message(sqlite3_errmsg(sqlite3_db_handle(handle.get())));
		throw sql_exception(result, std::move(message));
	}
}

template<typename T>
inline void verify_result(const int result, const statement_handle &handle, T&& query) {
	if (result != SQLITE_OK) {
		std::string message(sqlite3_errmsg(sqlite3_db_handle(handle.get())));
		throw sql_exception(result, std::move(message), std::forward<T>(query));
	}
}

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


	inline int execute(const std::string& str) {
		auto const result = sqlite3_exec(handle.get(), str.c_str(), nullptr, nullptr, nullptr);
		verify_result(result, handle, std::move(str));
		return result;
	}


	std::string get_current_error() const {
		return std::string(sqlite3_errmsg(handle.get()));
	}
 

	void open(char const *filename) {
		auto local = connection_handle{};

		auto const result = sqlite3_open(filename, local.get_address_of());

		if (SQLITE_OK != result) {
			throw sql_exception{result, sqlite3_errmsg(local.get())};
		}

		handle = std::move(local);
	}

};

struct statement {
	statement_handle handle;
	const std::string query;

	template<typename Connection>
	static inline statement create(Connection&& c, std::string sql) {
		statement result(std::move(sql));
		result.prepare(std::forward<Connection>(c));
		return result;
	}


	std::string get_current_error() const {
		return std::string(sqlite3_errmsg(sqlite3_db_handle(handle.get())));
	}		

	inline statement &bind(const int index, const int32_t value) {
		const int result = sqlite3_bind_int(handle.get(), index, value);
		verify_result(result, handle, query);
		return *this;
	}

	inline statement &bind(const int index, const int64_t value) {
		const int result = sqlite3_bind_int64(handle.get(), index, value);
		verify_result(result, handle, query);
		return *this;
	}

	inline statement &bind(const int index, const double value) {
		const int result = sqlite3_bind_double(handle.get(), index, value);
		verify_result(result, handle, query);
		return *this;
	}

	// binds null
	inline statement &bind(const int index) {
		const int result = sqlite3_bind_null(handle.get(), index);
		verify_result(result, handle, query);
		return *this;
	}

	inline statement &bind(const int index, sqlite3_value *value) {
		const int result = sqlite3_bind_value(handle.get(), index, value);
		verify_result(result, handle, query);
		return *this;
	}

	inline statement &bind_zeroblob(const int index, int size) {
		const int result = sqlite3_bind_zeroblob(handle.get(), index, size);
		verify_result(result, handle, query);
		return *this;
	}

	inline statement &bind(int const index, const std::string &value) {
		auto const result = sqlite3_bind_text(handle.get(), index, value.c_str(), value.size(), SQLITE_STATIC);
		verify_result(result, handle, query);
		return *this;
	}

	inline statement &bind_by_copy(int const index, const std::string &value) {
		auto const result = sqlite3_bind_text(handle.get(), index, value.c_str(), value.size(), SQLITE_TRANSIENT);
		verify_result(result, handle, query);
		return *this;
	}

	inline statement &bind(int const index, const void *blob, int size) {
		auto const result = sqlite3_bind_blob(handle.get(), index, blob, size, SQLITE_STATIC);
		verify_result(result, handle, query);
		return *this;
	}

	inline statement &bind_by_copy(int const index, const void *blob, int size) {
		auto const result = sqlite3_bind_blob(handle.get(), index, blob, size, SQLITE_TRANSIENT);
		verify_result(result, handle, query);
		return *this;
	}

	inline void reset_binding() {
		 sqlite3_reset(handle.get());
		 sqlite3_clear_bindings(handle.get());
		//verify_result(result, handle, query);
	}

	inline const statement& step_careless() const throw(sql_exception) {
		auto const result = sqlite3_step(handle.get());

		if (result != SQLITE_ROW && result != SQLITE_DONE) {
			verify_result(result, handle, query);
		}
		return *this;
	}

	inline expected<bool> step() const noexcept {
		auto const result = sqlite3_step(handle.get());

		if (result == SQLITE_ROW) {
			return expected<bool>(true);
		} else if (result == SQLITE_DONE) {
			return expected<bool>(false);
		} else {
			return expected<bool>::from_exception(create_error(result, handle));
		}
	}

	template<typename F>
	inline void step_all(F f)  {
		 expected<bool> result = step();
		while(result.valid() && result.get() == true) {
			f(*this);
			result = step();
		}
		if(!result.valid()) {
			throw result.get(); //rethorw
		}
	}

	inline int64_t rowid() const { return sqlite3_last_insert_rowid(sqlite3_db_handle(handle.get())); }
	inline int64_t get_int64(const int column = 1) const { return sqlite3_column_int64(handle.get(), column); }
	inline int32_t get_int(const int column = 1) const { return sqlite3_column_int(handle.get(), column); }
	inline double  get_double(const int column = 1) const { return sqlite3_column_double(handle.get(), column); }
	inline std::string get_string(const int column = 1) const { return std::string(reinterpret_cast<const char*>(sqlite3_column_text(handle.get(), column))); }
	inline sqlite3_value* get_value(const int column = 1) const { return sqlite3_column_value(handle.get(), column); }
	inline datatype get_type(const int column = 1) const { return static_cast<datatype>(sqlite3_column_type(handle.get(), column)); }

	private:

	statement(std::string sql): query(std::move(sql)) {}

	template<typename Connection>
	statement &prepare(Connection&& c) {
		handle.reset();

		auto const result = sqlite3_prepare_v2(c.handle.get(), query.c_str(), query.size(), handle.get_address_of(), nullptr);
		verify_result(result, c.handle, query);
		return *this;
	}
	//not sure what i should do with them.
	//int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
	//const void *sqlite3_column_blob(sqlite3_stmt*, int iCol);
};

} /* namespace sqlite  */

#endif /* __SQLITE_HPP__ */
