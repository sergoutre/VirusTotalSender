#include "Requests.h"

#include <curl/curl.h>
#include <string>
#include <tuple>

namespace VTSender
{
	Requests::Requests()
	{
		curl_global_init(CURL_GLOBAL_DEFAULT);
	}

	Requests::~Requests()
	{
		curl_global_cleanup();
	}

	std::tuple<int, std::string> Requests::makeRequest(std::string url, std::string params, const int method)
	{
		CURL* curl = curl_easy_init();

		if (method == static_cast<int>(Method::GET))
			url += params;

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		if (method == static_cast<int>(Method::POST))
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());

		std::string resultBody;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultBody);

		curl_easy_perform(curl);

		int httpCode = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

		curl_easy_cleanup(curl);

		return { httpCode, resultBody };
	}

	std::tuple<int, std::string> Requests::sendFile(std::string apiUrl, std::string apiKey, std::string filePath)
	{
		CURL* curl = curl_easy_init();

		curl_mime* form = curl_mime_init(curl);

		curl_mimepart* field = curl_mime_addpart(form);
		curl_mime_name(field, "apikey");
		curl_mime_data(field, apiKey.c_str(), CURL_ZERO_TERMINATED);

		field = curl_mime_addpart(form);
		curl_mime_name(field, "file");
		curl_mime_filedata(field, filePath.c_str());

		curl_easy_setopt(curl, CURLOPT_URL, (apiUrl + "file/scan").c_str());

		curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

		std::string resultBody;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resultBody);

		curl_easy_perform(curl);

		int httpCode = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

		curl_easy_cleanup(curl);

		return { httpCode, resultBody };
	}

	void Requests::openLink(std::string link)
	{
		ShellExecute(NULL, "open", link.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}


	size_t Requests::writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
	{
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}
} // namespace VTSender