// Copyright (C) 2012 Andrei Alexandrescu.

#ifndef NONSTD_EXPECTED_ALEXANDRESCU_H_INCLUDED
#define NONSTD_EXPECTED_ALEXANDRESCU_H_INCLUDED

#include <algorithm> // std::move()
#include <exception> // std::exception_ptr

template <class T> class expected {
	union {
		T ham;
		std::exception_ptr spam;
	};

	bool gotHam;

	expected() {} // used internally

      public:
	~expected() {
		if (gotHam)
			ham.~T();
		else
			spam.~exception_ptr();
	}

	expected(const T &rhs) : ham(rhs), gotHam(true) {}

	expected(T &&rhs) : ham(std::move(rhs)), gotHam(true) {}

	expected(const expected &rhs) : gotHam(rhs.gotHam) {
		if (gotHam)
			new (&ham) T(rhs.ham);
		else
			new (&spam) std::exception_ptr(rhs.spam);
	}

	expected& operator=(const expected& rhs) {
		ham = rhs.ham;
		gotHam = rhs.gotHam;
		return *this;
	}
	
	expected& operator=(expected&& rhs) {
		ham = std::move(rhs.ham);
		gotHam = std::move(rhs.gotHam);
		return *this;
	}

	expected(expected &&rhs) : gotHam(rhs.gotHam) {
		if (gotHam)
			new (&ham) T(std::move(rhs.ham));
		else
			new (&spam) std::exception_ptr(std::move(rhs.spam));
	}

	template <class E>
	static expected<T> from_exception(const E &exception) {
		if (typeid(exception) != typeid(E)) {
			throw std::invalid_argument("slicing detected");
		}
		return from_exception(std::make_exception_ptr(exception));
	}

	static expected<T> from_exception(std::exception_ptr p) {
		expected<T> result;
		result.gotHam = false;

		new (&result.spam) std::exception_ptr(std::move(p));

		return result;
	}

	static expected<T> from_exception() {
		return from_exception(std::current_exception());
	}

	// Access:

	bool valid() const { return gotHam; }

	T &get() {
		if (!gotHam)
			std::rethrow_exception(spam);
		return ham;
	}

	const T &get() const {
		if (!gotHam)
			std::rethrow_exception(spam);
		return ham;
	}

	// Probing for Type:

	template <class E> bool has_exception() const {
		try {
			if (!gotHam)
				std::rethrow_exception(spam);
		}
		catch (const E &object) {
			return true;
		}
		catch (...) {
		}
		return false;
	}


	// Icing:

	template <class F> static expected from_code(F fun) {
		try {
			return expected(fun());
		}
		catch (...) {
			return from_exception();
		}
	}

	// auto r = expected<string>::fromCode( [] { ... } );
};

#endif // NONSTD_EXPECTED_ALEXANDRESCU_H_INCLUDED
