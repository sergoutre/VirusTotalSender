#include "VirusTotal.h"

namespace VTSender
{
	std::string* VirusTotal::getApiKey()
	{
		return &apiKey_m;
	}

	void VirusTotal::setTotal(int total)
	{
		total_m = total;
	}

	int VirusTotal::getTotal() const
	{
		return total_m;
	}

	void VirusTotal::setPositives(int positives)
	{
		positives_m = positives;
	}

	int VirusTotal::getPositives() const
	{
		return positives_m;
	}

	void VirusTotal::setMessage(std::string message)
	{
		message_m = message;
	}

	std::string VirusTotal::getMessage() const
	{
		return message_m;
	}

	void VirusTotal::setScanDate(std::string scanDate)
	{
		scanDate_m = scanDate;
	}

	std::string VirusTotal::getScanDate() const
	{
		return scanDate_m;
	}

	void VirusTotal::setScanId(std::string scanId)
	{
		scanId_m = scanId;
	}

	std::string VirusTotal::getScanId() const
	{
		return scanId_m;
	}

	void VirusTotal::setPermalink(std::string permalink)
	{
		permalink_m = permalink;
	}

	std::string VirusTotal::getPermalink() const
	{
		return permalink_m;
	}

	void VirusTotal::zeroFields()
	{
		message_m = scanDate_m = scanId_m = permalink_m = "unknown";
		total_m = positives_m = 0;
	}
} // namespace VTSender