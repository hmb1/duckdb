//===----------------------------------------------------------------------===//
//                         DuckDB
//
// common/fstream_util.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/constants.hpp"
#include "common/exception.hpp"

namespace duckdb {
/**
 * Fstream Utility Functions
 */
class FstreamUtil {
public:
	/**
	 * Opens a file for the given name and returns it (default mode : ios_base::in | ios_base::out)
	 */
	static void OpenFile(const string &, fstream &, ios_base::openmode mode = ios_base::in | ios_base::out);

	/**
	 * Closes the given file or throws an exception otherwise
	 */
	static void CloseFile(fstream &);

	/**
	 * Returns the size in bytes of the given file
	 */
	static size_t GetFileSize(fstream &);

	/**
	 * Reads the given file as a binary
	 */
	static data_ptr ReadBinary(fstream &);
};
} // namespace duckdb