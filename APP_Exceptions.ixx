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

	//	로스트아크 API 서버 점검 중일 때 (HTTP 503)
	class APIMaintenanceException : public std::runtime_error
	{
	public:
		APIMaintenanceException();
	};

	//	그 밖의 HTTP 오류 (상태 코드를 담아서 5xx는 일시적 오류로 재시도 판단)
	class APIRequestException : public std::runtime_error
	{
	public:
		APIRequestException(int statusCode, const std::string& message);

		[[nodiscard]] int GetStatusCode() const noexcept;
		[[nodiscard]] bool IsServerError() const noexcept;	//	5xx (서버 쪽 일시적 오류)

	private:
		int m_statusCode{};
	};

	//	같은 회차의 다른 요청이 실패해서 중단됨 (원본 OperationCanceledException → 실패 원인으로 보지 않음)
	class RequestCanceledException : public std::runtime_error
	{
	public:
		RequestCanceledException();
	};
}