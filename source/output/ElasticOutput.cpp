#include "output/ElasticOutput.h"
#include "tools/logger.h"
#include "tools/tools.h"
#include "tools/json.hpp"
#include <functional>
#include <regex>
#include <exception> 
#include <codecvt>
#include <locale>
#include <curl/curl.h>

size_t getUrlResponse(void* buffer, size_t size, size_t count, void* data) {
    std::string* str = (std::string*)data;
    (*str).append((char*)buffer, size * count);
    return size * count;
};

//��б��ת˫б��
void pathConvert_Single2Double(std::string& s) {
    std::string::size_type pos = 0;
    while ((pos = s.find('\\', pos)) != std::string::npos) {
        s.insert(pos, "\\");
        pos = pos + 2;
    }
}

const std::string ElasticOutPut::confile = "config/elastic.conf";

void ElasticOutPut::output(std::string outputString) {
    //char* msg = outputString.c_str();
    if (!initialized())
    {
        MyLogger::writeLog("SocketOutPut is not initialized.\n");
    }
    if (0 == outputString.size()) return;

    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        MyLogger::writeLog("open curl failed...");
        return;
    }

    std::string url = "http://" + this->ip_port + "/" + _index + "/_bulk";
    std::stringstream ss;
    ss << outputString;

    std::string body, s, responseStr;
    while (std::getline(ss, s, '\n') && 0 < s.size()) {
        pathConvert_Single2Double(s);
        body += std::string("{ \"index\" : { \"_index\" : \"" + _index + "\", \"_type\" : \"_doc\"} }\n") + s + "\n";
    }

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "elastic:bupthtles");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &getUrlResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string err = "[-] curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)) + "\n";
        MyLogger::error(err);
    }

    nlohmann::json js = nlohmann::json::parse(responseStr);
    if (js.at("errors")) {
        MyLogger::error(responseStr);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

STATUS ElasticOutPut::init() {

    STATUS status = this->parseIPAndPort();

    if (status != STATUS_SUCCESS)
        return status;

    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
        std::cout << "load library socket failed..." << std::endl;
        system("pause");
    }
    std::ifstream conf(this->confile);
    if (!conf.is_open()) {
        MyLogger::error("elastic.conf open failed!");
        exit(-1);
    }
    conf >> this->_index;
    if (0 == _index.size()) {
        MyLogger::error("elasticsearch index is NULL!");
        exit(-1);
    }
    conf.close();

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "elastic:bupthtles");
    // create elastic index
    std::string url = "http://" + this->ip_port + "/" + _index;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    std::string body = "{\"settings\":{\"refresh_interval\":\"300s\",\"number_of_shards\":\"5\",\"merge\":{\"scheduler\":{\"max_thread_count\":\"100\"},\"policy\":{\"segments_per_tier\":\"5\",\"max_merged_segment\":\"100mb\"}},\"max_result_window\":\"50000\",\"number_of_replicas\":\"1\"}}";
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string err = "[-] curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)) + "\n";
        MyLogger::error(err);
    }
    curl_easy_cleanup(curl);
    // create elastic index mapping
    url = url + "/_mapping";
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "elastic:bupthtles");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    nlohmann::json js;
    js["dynamic"] = true;
    js["properties"]["Event"]["type"] = "text";
    js["properties"]["PID"]["type"] = "long";
    js["properties"]["PName"]["type"] = "text";
    js["properties"]["PPID"]["type"] = "long";
    js["properties"]["PPName"]["type"] = "text";
    js["properties"]["TID"]["type"] = "long";
    js["properties"]["TimeStamp"]["type"] = "long";
    js["properties"]["Host-UUID"]["type"] = "text";
    js["properties"]["args"]["type"] = "text";
    body = js.dump();

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string err = "[-] curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)) + "\n";
        MyLogger::error(err);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return STATUS_SUCCESS;

}
