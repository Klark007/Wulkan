#pragma once

#include <string>
#include <fmt/core.h>
#include <exception>

// Custom exception for error handling

class EngineException : public std::exception {
public:
	EngineException(const std::string& msg, const std::string& file, int line);
	const char* what() const throw () override;
protected:
	const std::string file;
	int line;

	const std::string msg;
	const std::string format_msg;
};

// outputs the line and file the exception was thrown at
inline EngineException::EngineException(const std::string& msg, const std::string& file, int line)
	: msg{ msg }, file{ file }, line{ line }, format_msg{ fmt::format("Exception in [{}] at line {}:\n {}", file, line, msg) }
{
}


class SetupException : public EngineException {
public:
	SetupException(const std::string& msg, const std::string& file, int line) : EngineException{ msg, file, line } {};
};

class ShaderException : public EngineException {
public:
	ShaderException(const std::string& msg, const std::string& file, int line) : EngineException{ msg, file, line } {};
};

class IOException : public EngineException {
public:
	IOException(const std::string& msg, const std::string& file, int line) : EngineException{ msg, file, line } {};
};

class RuntimeException : public EngineException {
public:
	RuntimeException(const std::string& msg, const std::string& file, int line) : EngineException{ msg, file, line } {};
};

class NotImplementedException : public EngineException {
public:
	NotImplementedException(const std::string& msg, const std::string& file, int line) : EngineException{ msg, file, line } {};
};