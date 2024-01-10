#pragma once

#include "output.h"

class ElasticOutPut : public Output {
public:
	ElasticOutPut(std::string ip_port) {
		this->ip_port = ip_port;
	};
	~ElasticOutPut()
	{
		curl_global_cleanup();
	}
	virtual void output(std::string outputString) override;
	virtual STATUS init() override;

private:
	static const std::string confile;
	std::string _index;
};

