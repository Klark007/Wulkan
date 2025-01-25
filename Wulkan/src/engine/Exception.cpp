#include "Exception.h"

const char* EngineException::what() const throw()
{
	return format_msg.c_str();
}