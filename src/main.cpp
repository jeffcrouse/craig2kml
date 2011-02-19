/*
 *  main.cpp
 *  craig2kml
 *
 *  Created by Jeffrey Crouse on 2/14/11.
 *  Copyright 2011 Eyebeam. All rights reserved.
 *
 */

#define VERSION 0.1

#include <iostream>
#include <fstream>
#include "Webpage.h"
#include "SearchResult.h"
#include "Listing.h"
#include "Craig2KML.h"


// All of these vars are set with command line options
const char* outfilepath=NULL;
const char* url=NULL;
const char* configfilename=NULL;
bool verbose=false;
int maxListings=999;



// Some helper functions.
void load_config_file(const char* filename, map<string,string>& config);
void parse_args(int argc, char* argv[]);
void help();
map<string,string> default_config();
string truncate(string str, int n=60);

// -----------------------------------------
int main (int argc, char* argv[])
{	
	// Set default config options
	map<string,string> config = default_config();

	// Parse command line options.
	parse_args(argc, argv);

	// We can't do anything without a URL
	if(url==NULL)
	{
		help();
		exit(1);
	}
	
	
	// Parse the config file if it exists.
	if(configfilename!=NULL)
	{
		load_config_file(configfilename, config);
	}

	// Set the user agent for all Webpage operations
	Webpage::userAgent = config["user_agent"];
	
	
	if(verbose) 
		cerr << "opening " << truncate(url) << endl;
	
	// Get the links from the main listings page.
	Webpage* listingsPage;
	map<string,string> links;
	try {
		// Create the main page with all of the listings
		listingsPage = new Webpage(url, false, false, verbose);
		links = listingsPage->getLinks(config["craigslist_links"]);
		if(verbose)
			cerr << "Retrieved " << links.size() << " links" << endl;
		
	} catch(string s) {
		if(verbose) 
			cerr << "ERROR:  " << s << endl;
		return -1;
	}
	
	
	// Create the document we will be outputting
	Craig2KML c2k(listingsPage->getNodeContents("//title"), verbose);
	
		
	// Loop through all of the links on the page.
	int i=0;
	for(map<string,string>::iterator it=links.begin(); it!=links.end() && (i<maxListings); ++it)
	{
		// TO DO:  Both of these varnames already exist in a higher scope!
		string title = it->first;
		string url = it->second;
		string description;
		if(verbose) 
			cerr << "Parsing " << i++ << " out of " << links.size() << ": " << title << endl;

		bool mappable=true;
		try {
			Webpage listing(url, false, true, verbose);
			string addr = listing.getNodeAttribute(config["craigslist_google_maps_link"], "href");
			addr.erase(0, config["craigslist_google_maps_link_prefix"].length());
			
			description = listing.getNodeAsString(config["craigslist_item_description"]);
			
			if(!addr.empty())
			{
				string geocodeURL = config["google_geocode_base"] + addr;
				if(verbose) cerr << "calling " << geocodeURL << endl;
				Webpage geocode(geocodeURL, true, true, verbose);
				const char* status = geocode.getNodeContents(config["google_geocode_status"]);
				if(strcmp(status, "OK")==0)
				{
					float lat = atof(geocode.getNodeContents(config["google_geocode_lat"]));
					float lng = atof(geocode.getNodeContents(config["google_geocode_lng"]));
					
					if(verbose) 
						cerr << "Adding placemark at " << lat << ", " << lng << endl;

					c2k.addMappable(title, description, lat, lng);
				}
				else {
					if(verbose) cerr << "Geocode failed. Unmappable." << endl;
					mappable=false;
				}
			}
			else {
				if(verbose) cerr << "No address found. Unmappable." << endl;
				mappable=false;
			}
			
		} catch(exception& e) {
			if(verbose) cerr << "ERROR:  " << e.what() << endl;
			mappable=false;
		}
		
		if(!mappable)
		{
			c2k.addUnmappable(title, description);
		}
	}
	
	
	// Decide where to put the output
	std::ofstream realOutFile;
	if(outfilepath!=NULL)
		realOutFile.open(outfilepath, std::ios::out);
	std::ostream & outFile = (realOutFile.is_open() ? realOutFile : std::cout);
	
	// Send the serialized file to whatever output we have set.
	outFile << c2k.serialize();
	
	
	// Shutdown libxml
    xmlCleanupParser();
}




// -----------------------------------------
void help()
{
	cout << "typical: (-u|--url ) #### [(-o|--outfile) ####]" << endl;
	cout << "  where:" << endl;
	cout << "  -c (--config) use custom config values";
	cout << "  -m (--max) maximum number of listings to include";
	cout << "  -o (--outfile) is the file in which the kml will be saved" << endl;
	cout << "     prints to stdout if no file is provided." << endl;
	cout << "  -u (--url) [required]" << endl;
	cout << "      the Craigslist search page URL to be translated" << endl;
	cout << "  -v (--verbose) print messages to stderr";
}


// -----------------------------------------
void parse_args(int argc, char* argv[])
{
	for(int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--outfile") == 0)
		{
			if (i+1 == argc) {
				// error messages intermingled with parsing logic
				cerr << "Invalid " << argv[i];
				cerr << " parameter: no outfile specified\n";
				help();
				exit(1);
			}
			outfilepath = argv[++i];  // parsing action goes here
		}
		else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--url") == 0)
		{
			if (i+1 == argc) {
				cerr << "Invalid " << argv[i];
				cerr << " parameter: no URL specified\n";
				help();
				exit(1);
			}
			url = argv[++i];
		}
		else if(strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-c") == 0)
		{
			if (i+1 == argc) {
				cerr << "Invalid " << argv[i];
				cerr << " parameter: no config file specified\n";
				help();
				exit(1);
			}
			configfilename = argv[++i];
		}
		else if(strcmp(argv[i], "--max") == 0 || strcmp(argv[i], "-m") == 0)
		{
			if (i+1 == argc) {
				cerr << "Invalid " << argv[i];
				cerr << " parameter: no integer provided\n";
				help();
				exit(1);
			}
			maxListings = atoi(argv[++i]);
		}
		else if(strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0)
		{
			verbose=true;
		}
		else if (strcmp(argv[i], "--version") == 0)
		{
			fprintf(stderr, "craig2kml version %f", VERSION);
			exit(0);
		} 
	}
}

// -----------------------------------------
string truncate(string str, int n)
{
	if(str.length()>n)
	{
		string shortened(str);
		shortened.erase(n);
		return shortened+"...";
	}
	else
	{
		return str;
	}

}

// -----------------------------------------
map<string,string> default_config()
{
	map<string,string> defaultConfig;
	defaultConfig["craigslist_links"]				= "//body/blockquote/p/a";
	defaultConfig["craigslist_google_maps_link"]	= "//div[@id='userbody']//small/a";
	defaultConfig["google_geocode_base"]			= "http://maps.googleapis.com/maps/api/geocode/xml?sensor=false&address=";
	defaultConfig["google_geocode_status"]			= "/GeocodeResponse/status";
	defaultConfig["google_geocode_lat"]			= "/GeocodeResponse/result/geometry/location/lat";
	defaultConfig["google_geocode_lng"]			= "/GeocodeResponse/result/geometry/location/lng";
	defaultConfig["craigslist_google_maps_link_prefix"] = "http://maps.google.com/?q=loc%3A+";
	defaultConfig["craigslist_item_description"]	= "//div[@id='userbody']";
	defaultConfig["user_agent"]					= "Mozilla/5.0";
	return defaultConfig;
}


// -----------------------------------------
void load_config_file(const char* filename, map<string,string>& config)
{
	string line, name, value;
	ifstream configfile(filename);
	if (configfile.is_open())
	{
		while(configfile.good())
		{
			getline(configfile, line);
			if(!line.empty() && line[0]!='#')
			{
				istringstream liness( line );
				getline( liness, name, ' ' );
				getline( liness, value, ' ' );
				if(config.find(name) != config.end())
				{
					if(verbose) cerr << "setting " << name << " to " << value << endl;
					config[name] = value;
				}
				else {
					cerr << "Unrecognized config option: " << name << endl;
				}
			}
		}
		configfile.close();
	}
	else 
	{
		cerr << "Unable to open configfile.  Using defaults." << endl; 
	}	
}