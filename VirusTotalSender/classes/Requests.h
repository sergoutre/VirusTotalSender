#ifndef REQUESTS_H
#define REQUESTS_H

#include <string>
#include <tuple>

namespace VTSender
{
    class Requests
    {
    public:
        enum class Method
        {
            GET,
            POST
        };

        Requests();
        ~Requests();

        static std::tuple<int, std::string> makeRequest(std::string url, std::string params, const int method = static_cast<int>(Method::GET));

        static std::tuple<int, std::string> sendFile(std::string apiUrl, std::string apiKey, std::string filePath);

        static void openLink(std::string link);
    private:

        static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    };
} // namespace VTSender

#endif // REQUESTS_H