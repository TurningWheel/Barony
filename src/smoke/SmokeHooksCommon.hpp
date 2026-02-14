#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace SmokeHooksCommon
{
	inline bool envHasValue(const char* key)
	{
		const char* raw = std::getenv(key);
		return raw && raw[0] != '\0';
	}

	inline std::string toLowerCopy(const char* value)
	{
		std::string result = value ? value : "";
		for ( char& ch : result )
		{
			ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		}
		return result;
	}

	inline bool parseEnvBool(const char* key, bool fallback)
	{
		const char* raw = std::getenv(key);
		if ( !raw || !raw[0] )
		{
			return fallback;
		}
		const std::string value = toLowerCopy(raw);
		if ( value == "1" || value == "true" || value == "yes" || value == "on" )
		{
			return true;
		}
		if ( value == "0" || value == "false" || value == "no" || value == "off" )
		{
			return false;
		}
		return fallback;
	}

	inline int parseEnvInt(const char* key, int fallback, int minValue, int maxValue)
	{
		const char* raw = std::getenv(key);
		if ( !raw || !raw[0] )
		{
			return fallback;
		}
		char* end = nullptr;
		const long parsed = std::strtol(raw, &end, 10);
		if ( end == raw || (end && *end != '\0') )
		{
			return fallback;
		}
		return std::max(minValue, std::min(maxValue, static_cast<int>(parsed)));
	}

	inline std::string parseEnvString(const char* key, const std::string& fallback)
	{
		const char* raw = std::getenv(key);
		if ( !raw || !raw[0] )
		{
			return fallback;
		}
		return std::string(raw);
	}

	inline std::string trimCopy(const std::string& value)
	{
		const size_t begin = value.find_first_not_of(" \t\r\n");
		if ( begin == std::string::npos )
		{
			return "";
		}
		const size_t end = value.find_last_not_of(" \t\r\n");
		return value.substr(begin, end - begin + 1);
	}

	inline bool parseBoundedIntString(const std::string& value, int minValue, int maxValue, int& outValue)
	{
		if ( value.empty() )
		{
			return false;
		}
		char* end = nullptr;
		const long parsed = std::strtol(value.c_str(), &end, 10);
		if ( end == value.c_str() || (end && *end != '\0') )
		{
			return false;
		}
		if ( parsed < minValue || parsed > maxValue )
		{
			return false;
		}
		outValue = static_cast<int>(parsed);
		return true;
	}
}
