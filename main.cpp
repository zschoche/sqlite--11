/* Boost.MultiIndex example of serialization of a MRU list.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include <vector>
#include <iostream>
#include <chrono>
#include "sqlite.hpp"

int main() {

	try {
		sqlite::connection c = sqlite::connection::create_memory();
		c.execute("drop table if EXISTS Hens");
		c.execute("create table Hens ( Id int primary key, Name text not null )");
		sqlite::statement insert = sqlite::statement::create(c, "insert into Hens (Id, Name) values (?, ?)");
		insert.bind(1, 101).bind(2, "Henrietta").step();
		insert.reset_binding();
		insert.bind(1, 102).bind(2, "Rowena").step();
		sqlite::statement select = sqlite::statement::create(c, "select Id, rowid, Name from Hens");

		while(select.step()) {
			std::cout << "Id:" << select.get_int(1) << " Name:" << select.get_string(2) <<  std::endl;

		}
	}
	catch (sqlite::sql_exception const &e) {
		std::cerr << "error(" << e.code << "): " << e.message.c_str() << std::endl;
	}

	return 0;
}
