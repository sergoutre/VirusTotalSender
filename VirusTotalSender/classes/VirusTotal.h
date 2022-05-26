#ifndef VIRUS_TOTAL_H
#define VIRUS_TOTAL_H

#include <string>
#include <map>

namespace VTSender
{
	class VirusTotal
	{
	public:
		enum class Responses
		{
			OK = 200,
			RATE_LIMIT_EXCEEDED = 204,
			BAD_REQUEST = 400,
			FORBIDDEN = 403
		};
		enum class File
		{
			MAX_SIZE = 32
		};

		inline static const std::string apiUrl = "http://www.virustotal.com/vtapi/v2/";

		inline static const std::map<std::string, std::string> urlPaths =
		{
			{"fReport", "file/report"},
			{"fScan", "file/scan"},
			{"uReport", "url/report"},
			{"uScan", "url/scan"}
		};

		static std::string* getApiKey();

		void setTotal(int total);
		int getTotal() const;

		void setPositives(int positives);
		int getPositives() const;

		void setMessage(std::string message);
		std::string getMessage() const;

		void setScanDate(std::string scanDate);
		std::string getScanDate() const;

		void setScanId(std::string scanId);
		std::string getScanId() const;

		void setPermalink(std::string permalink);
		std::string getPermalink() const;

		void zeroFields();

	private:
		inline static std::string apiKey_m;

		std::string scanId_m = "unknown";
		std::string scanDate_m = "unknown";
		std::string message_m = "unknown";
		std::string permalink_m = "unknown";

		int total_m = 0;
		int positives_m = 0;
	};
} // namespace VTSender
#endif // VIRUS_TOTAL_H

