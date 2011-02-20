/*
 *  Webpage.cpp
 *  earthify
 *
 *  Created by Jeffrey Crouse on 2/14/11.
 *  Copyright 2011 Eyebeam. All rights reserved.
 *
 */

#include "Webpage.h"

// -------------------------------------------------------------
string Webpage::userAgent = "Mozilla/5.0";
string Webpage::cacheDirectory = "";
bool Webpage::libxmlInited = false;



// -------------------------------------------------------------
Webpage::Webpage()
{
	verbose = true;
	if(!Webpage::libxmlInited)
	{
		if(verbose)
			cerr << "Initializing libxml." << endl;
		xmlInitParser();
		Webpage::libxmlInited = true;
	}
}


// -------------------------------------------------------------
Webpage::~Webpage()
{
	xmlFreeDoc(doc);
	//xmlXPathFreeContext(xpathCtx); 
}


// -------------------------------------------------------------
void Webpage::setVerbose(bool _verbose)
{
	verbose = _verbose;
}


// -------------------------------------------------------------
bool Webpage::open(string url, bool wellFormed, bool useCache)
{	
	if(Webpage::cacheDirectory.empty())
	{
		useCache=false;
	}
	
	bool loaded=false;
	if(useCache)
	{
		locale loc;
		const collate<char>& coll = use_facet<collate<char> >(loc);
		long myhash = coll.hash(url.data(),url.data()+url.length());
		sprintf(cachefile, "%s/%ld.cache", Webpage::cacheDirectory.c_str(), myhash);
		
		loaded = loadFromCache();
	}
	
	if(!loaded)
	{
		contents = download(url, verbose);
		if(!wellFormed)
		{
			tidy_me();
		}
		if(useCache)
		{
			saveToCache();
		}
	}
	
	// get rid of doctype line.  It messes up the parser
	string prefix = "<!DOCTYPE";
	if(contents.compare(0, prefix.size(), prefix)==0)
	{
		size_t pos = contents.find(">");
		contents.erase(0, pos+1);
	}
	
	if(verbose)
		cerr << "Parsing document. Length: " << contents.length() << endl;
	
	doc = xmlParseMemory(contents.c_str(), contents.length());
	if (doc == NULL) {
		if(verbose)
			cerr << "Error: unable to parse HTML" << endl;
		return false;
	}
	
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		xmlFreeDoc(doc);
		if(verbose)
			cerr << "Error: unable to create new XPath context" << endl;
		return false;
	}
	
	return true;
}


// -------------------------------------------------------------
bool Webpage::saveToCache()
{
	ofstream myfile;
	myfile.open(cachefile, ios::out);
	if(myfile.is_open())
	{
		if(verbose) 
			cerr << "Writing cachefile: " << cachefile << endl;
		myfile << contents;
		myfile.close();
		return true;
	}
	else
	{
		if(verbose) 
			cerr << "ERROR:  Couldn't write to " << cachefile << endl;
		return false;
	}
}
	
// -------------------------------------------------------------
bool Webpage::loadFromCache()
{
	string line;
	ifstream myfile(cachefile);
	if(myfile.is_open())
	{
		if(verbose) 
			cerr << "loading from cache: " << cachefile << endl;
		
		while ( myfile.good() )
		{
			getline(myfile,line);
			contents += line+"\n";
		}
		myfile.close();
		return true;
	} else {
		return false;
	}
}


// -------------------------------------------------------------
string Webpage::download(string url, bool verbose)
{
	string str;
	
	if(verbose) 
		cerr << "downloading..." << endl;
	
	static char errorBuffer[CURL_ERROR_SIZE];
	CURL *curl = curl_easy_init();
	CURLcode result;
	if (!curl) {
		throw "Couldn't create CURL object.";
	}
	
	// Set the headers
	struct curl_slist *headers=NULL;
	char user_agent_header[255];
	sprintf(user_agent_header, "User-Agent: %s", Webpage::userAgent.c_str());
	headers = curl_slist_append(headers, user_agent_header);
	
	
	// TO DO:
	// Set timeout limit
	// put the whoel thing in a try block
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
	result = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);
	
	
	// Did we succeed?
	if (result != CURLE_OK)
	{
		throw std::runtime_error(errorBuffer);						
	}
	
	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	char status_msg[255];
	sprintf(status_msg, "HTTP status code: %ld", http_code);
	if(verbose) cerr << status_msg << endl;
	if (http_code != 200 || result == CURLE_ABORTED_BY_CALLBACK)
	{
		throw std::runtime_error("HTTP error");	
	}
	
	return str;
}


// -------------------------------------------------------------
void Webpage::tidy_me()
{
	try {
		TidyDoc _tdoc = tidyCreate();
		//tidyOptSetBool(_tdoc, tidyOptGetIdForName("show-body-only"), (Bool)1);
		tidyOptSetBool(_tdoc, tidyOptGetIdForName("output-xhtml"), (Bool)1);
		tidyOptSetBool(_tdoc, tidyOptGetIdForName("quote-nbsp"), (Bool)0);
		tidyOptSetBool(_tdoc, tidyOptGetIdForName("show-warnings"), (Bool)0);
		tidyOptSetValue(_tdoc, tidyOptGetIdForName("char-encoding"), "utf8");
		//tidyOptSetBool(_tdoc, tidyOptGetIdForName("ascii-chars"), (Bool)1);
		//tidyOptSetBool(_tdoc, tidyOptGetIdForName("markup"), (Bool)1);
		//tidyOptSetValue(_tdoc, tidyOptGetIdForName("indent"), "yes");
		//tidyOptSetValue(_tdoc, tidyOptGetIdForName("newline"), "\n");
		tidyOptSetInt(_tdoc, tidyOptGetIdForName("wrap"), 5000);
		tidyParseString( _tdoc, contents.c_str() );
	
		/*
		// tidySaveBuffer doesn't seem to work with the makefile for some reason.
		TidyBuffer output = {0};
		tidySaveBuffer(_tdoc, &output);
		cout << "3. TidyBuffer size: " << output.size << endl;
		contents = string((char*)output.bp, (size_t)output.size);
		 */
		
		// tidySaveString is a tricky beast.
		tmbstr buffer = NULL;
		uint buflen = 0;
		int status;
		//status = tidySaveStdout( tdoc );
		do {
			status = tidySaveString( _tdoc, buffer, &buflen );
			//printf("tidySaveString status, buflen = %d, %d\n", status, buflen);
			if (status == -ENOMEM) {
				//printf("Need to allocate buffer of at least %d bytes in size\n", buflen);
				if(buffer) 
					free(buffer);
				buffer = (tmbstr)malloc(buflen + 1);
			}
		} while (status == -ENOMEM);
		contents = (char*)buffer;

	} catch (exception& e) {
		throw e.what();
	}
}

// -------------------------------------------------------------
xmlXPathObjectPtr Webpage::xpath(string exp)
{		
	const xmlChar* xpathExpr = BAD_CAST exp.c_str();
	xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL)
	{
		xmlXPathFreeContext(xpathCtx);
		throw std::runtime_error("Error: unable to evaluate xpath expression");
	}
	//std::cout << "results: " << xpathObj->nodesetval->nodeNr << endl;
	return xpathObj;
}

// -------------------------------------------------------------
map<string,string> Webpage::getLinks(string exp)
{
	map<string,string> links;
	xmlXPathObjectPtr obj = xpath(exp);
	xmlNodeSetPtr nodeset = obj->nodesetval;
	
	for (int i=0; i<nodeset->nodeNr; i++)
	{
		xmlChar *href, *title;
		href = xmlGetProp(nodeset->nodeTab[i], (const xmlChar *)"href");
		title = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
		links[(const char*)title] = (const char*)href;
	}

	xmlXPathFreeObject(obj);
	return links;
}


// -------------------------------------------------------------
const char* Webpage::getNodeAsString(string exp)
{
	xmlXPathObjectPtr obj = xpath(exp);
	xmlNodeSetPtr nodeset = obj->nodesetval;
	if(nodeset && nodeset->nodeNr>0)
	{
		xmlBufferPtr buf = xmlBufferCreate();
		xmlSaveCtxtPtr savectx = xmlSaveToBuffer(buf, 0, XML_SAVE_FORMAT);
		xmlNodePtr node = nodeset->nodeTab[0];
		if (savectx)
		{
			xmlSaveTree(savectx, node);
			xmlSaveClose(savectx);
		}
		return (const char*)xmlBufferContent(buf);
	}
	else
	{	
		cout << "no node found" << endl;
		return "";
	}
}

// -------------------------------------------------------------
const char* Webpage::getNodeAttribute(string exp, string attrib)
{
	const xmlChar* contents=(const xmlChar*)"";
	xmlXPathObjectPtr obj = xpath(exp);
	xmlNodeSetPtr nodeset = obj->nodesetval;
	
	if(nodeset && nodeset->nodeNr > 0)
	{
		contents = xmlGetProp(nodeset->nodeTab[0], (const xmlChar *)attrib.c_str());
	}
	xmlXPathFreeObject(obj);

	return (const char*)contents;
}

// -------------------------------------------------------------
const char* Webpage::getNodeContents(string exp)
{
	const xmlChar* contents;
	xmlXPathObjectPtr obj = xpath(exp);
	xmlNodeSetPtr nodeset = obj->nodesetval;
	
	if(nodeset && nodeset->nodeNr > 0)
	{
		contents =  xmlNodeListGetString(doc, nodeset->nodeTab[0]->children, 1);
	}
	xmlXPathFreeObject(obj);
	
	return (const char*)contents;
}

// -------------------------------------------------------------
int Webpage::writeData(char *data, size_t size, size_t nmemb, std::string *buffer)
{
	int result = 0;
	if (buffer != NULL) {
		buffer->append(data, size * nmemb);
		result = size * nmemb;
	}
	return result;
}
