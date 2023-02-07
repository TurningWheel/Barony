/*
 * EOS Mocks to make it work with the editor.
 */
#include "Config.hpp"

#ifdef USE_EOS

#include "main.hpp"
#include "eos.hpp"

EOSFuncs EOS;

bool EOSFuncs::initAuth(std::string hostname, std::string tokenName)
{
	return true;
}

bool EOSFuncs::initPlatform(bool enableLogging)
{
	return true;
}

void EOSFuncs::readFromFile()
{
}

void EOSFuncs::readFromCmdLineArgs()
{
}

#endif
