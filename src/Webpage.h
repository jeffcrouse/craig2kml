/*
 *  Webpage.h
 *  craig2kml
 *
 *  Created by Jeffrey Crouse on 2/14/11.
 *  Copyright 2011 Eyebeam. All rights reserved.
 *
 */

#pragma once
#include <iostream>
#include <stdio.h>
#include <vector>
#include <stdexcept>
#include <tidy.h>
#include <buffio.h>
#include <string>
#include <map>
#include <curl/curl.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "libxml/xmlsave.h"
#include <locale>
#include <iostream>
#include <fstream>

using namespace std;
class Webpage {
public:
	
	Webpage();
	~Webpage();

	// Load in a URL
	bool open(string url, bool wellFormed=false, bool useCache=true);
	
	// Run TidyLib on 'contents'
	void tidy_me();
	
	// Gets a map of the content and href od all links within a given xpath expression
	map<string,string> getLinks(string exp);
	
	// Node stuff
	const char* getNodeAsString(string exp);
	const char* getNodeContents(string exp);
	const char* getNodeAttribute(string exp, string atrrib);

	// Cache stuff
	bool loadFromCache();
	bool saveToCache();
	
	//int contentLength() {	return contents.length();	}
	
	void setVerbose(bool _verbose);
	
	static string download(string url, bool verbose=false);
	static string userAgent;
	static string cacheDirectory;
	
protected:
	
	static bool libxmlInited;
	bool verbose;
	xmlXPathObjectPtr xpath(string exp);
	static int writeData(char *data, size_t size, size_t nmemb, std::string *buffer);
	char cachefile[255];
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	string contents;
};

