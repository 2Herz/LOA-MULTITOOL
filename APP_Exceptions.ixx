export module LostArkMultiTool.Core.APPExceptions;

import std;

export namespace LostArkMultiTool::Core
{
	//	API 키가 잘못됐을 때 (HTTP 401)
	class APIKeyInvalidException : public std::runtime_error
	{
	public:
		APIKeyInvalidException();
	};

	//	API 호출 한도를 넘었을 때 (HTTP 429)
	class APIRateLimitException : public std::runtime_error
	{
	public:
		APIRateLimitException();
	};
}