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
#include <kml/dom.h>
#include <kml/engine.h>
#include "Webpage.h"

using namespace std;
using kmldom::CoordinatesPtr;
using kmldom::KmlPtr;
using kmldom::KmlFactory;
using kmldom::PlacemarkPtr;
using kmldom::PointPtr;
using kmldom::FolderPtr;
using kmlengine::Bbox;

// Some global vars.
const char* outfilepath=NULL;
const char* url=NULL;
const char* configfilename=NULL;
bool verbose=false;
int maxListings=999;

// Some helper functions.
void load_config_file(const char* filename, map<string,string>& config);
void parse_args(int argc, char* argv[]);
void help();

// -----------------------------------------
int main (int argc, char* argv[])
{
	// Default config options
	map<string,string> config;
	config["craigslist_links"]				= "/body/blockquote/p/a";
	config["craigslist_google_maps_link"]	= "//div[@id='userbody']//small/a";
	config["google_geocode_base"]			= "http://maps.googleapis.com/maps/api/geocode/xml?sensor=false&address=";
	config["google_geocode_status"]			= "/GeocodeResponse/status";
	config["google_geocode_lat"]			= "/GeocodeResponse/result/geometry/location/lat";
	config["google_geocode_lng"]			= "/GeocodeResponse/result/geometry/location/lng";
	config["craigslist_google_maps_link_prefix"] = "http://maps.google.com/?q=loc%3A+";
	config["craigslist_item_description"]	= "//div[@id='userbody']";
	config["user_agent"]					= "Mozilla/5.0";
	
	// Parse command line options.
	parse_args(argc, argv);

	// We can't do anything without a URL
	if(url==NULL)
	{
		help();
		exit(1);
	}
	
	// If the user provided an output file, try to open it.
	ofstream outfile;
	if(outfilepath!=NULL)
	{
		outfile.open(outfilepath);
		if(!outfile.is_open()) {
			cerr << "Couldn't open " << outfilepath << " for writing." << endl;
			return -1;
		}	
	}
	
	// Parse the config file if it exists.
	if(configfilename!=NULL)
	{
		load_config_file(configfilename, config);
	}

	
	Webpage::userAgent = config["user_agent"];
	
	
	/* Set up libXML */
	xmlInitParser();
	
	
	// Create the main page with all of the listings
	if(verbose) cerr << "opening " << url << endl;
	Webpage listingsPage(url);
	

	// All of the listings are within /body/blockquote/p
	// These are the links to individual listing pages
	// first = title
	// second = link
	map<string,string> links;
	map<string,string>::iterator it;
	links = listingsPage.getLinks(config["craigslist_links"]);
	int numLinks = links.size();
	
	if(verbose)
		cerr << "Content length: " << listingsPage.contentLength() << endl;
	
	if(verbose)
		cerr << "Retrieved " << numLinks << " links." << endl;
	
	// Get the factory singleton to create KML elements.
	KmlFactory* factory = KmlFactory::GetFactory();

	// Create <kml>
	KmlPtr kml = factory->CreateKml();
	
	// Create the root folder
	FolderPtr root = factory->CreateFolder();
	root->set_name("Craig2KML");

	// Add it to the main KML object
	kml->set_feature(root);  // kml takes ownership.
	
	
	// Create the description for the main folder
	time_t t = time(0); //obtain the current time_t value
	tm now=*localtime(&t); //convert it to tm
	char tmdescr[255]={0};
	strftime(tmdescr, sizeof(tmdescr)-1, "%A, %B %d %Y. %X", &now);
	char desc[1024];
	sprintf(desc, "\"%s\" %s on %s", listingsPage.getNodeContents("//title"), url, tmdescr);
	if(verbose) cerr << desc << endl;
	root->set_description(desc);
	
	
	// Make a folder for all of the mapable listings
	FolderPtr mapableListings = factory->CreateFolder();
	mapableListings->set_name("Mapable Listings");
	
	// Add it to the main KML object
	root->add_feature(mapableListings);  // kml takes ownership.
	
	
	// Loop through all of the links on the page.
	int i=0;
	Bbox bbox;
	vector<PlacemarkPtr> unmappable;
	for(it = links.begin(); it != links.end() && (i<maxListings); ++it)
	{
		string title = it->first;
		string url = it->second;
		if(verbose) cerr << "Parsing " << i << " out of " << numLinks << ": " << title << endl;
		
		i++;
		try {
			Webpage listing(url);
			string addr = listing.getNodeAttribute(config["craigslist_google_maps_link"], "href");
			addr.erase(0, config["craigslist_google_maps_link_prefix"].length());
			
			string description = listing.getNodeAsString(config["craigslist_item_description"]);
			
			// Create <Placemark> and give it a <name> and the <Point>.
			PlacemarkPtr placemark = factory->CreatePlacemark();
			placemark->set_name(title);
			placemark->set_description(description);
			
			if(!addr.empty())
			{
				string geocodeURL = config["google_geocode_base"] + addr;
				if(verbose) cerr << "calling " << geocodeURL << endl;
				Webpage geocode(geocodeURL, true);
				
				const char* status = geocode.getNodeContents(config["google_geocode_status"]);
				if(strcmp(status, "OK")==0)
				{
					float lat = atof(geocode.getNodeContents(config["google_geocode_lat"]));
					float lng = atof(geocode.getNodeContents(config["google_geocode_lng"]));
					
					// Expand the bounding box of all mappable listings
					bbox.ExpandLatLon(lat, lng);
					
					if(verbose) cerr << "Adding placemark at " << lat << ", " << lng << endl;
					
					// Create <coordinates>.
					CoordinatesPtr coordinates = factory->CreateCoordinates();
					
					// Create <coordinates>-122.0816695,37.42052549<coordinates>
					coordinates->add_latlng(lat, lng);
					
					// Create <Point> and give it <coordinates>.
					PointPtr point = factory->CreatePoint();
					point->set_coordinates(coordinates);  // point takes ownership
					
					placemark->set_geometry(point);  // placemark takes ownership
					
					// Add it to the folder
					mapableListings->add_feature(placemark);
				}
				else
				{
					if(verbose) cerr << "geocode failed for " << addr << endl;
					unmappable.push_back(placemark);
				}
			}
			else
			{
				if(verbose) cerr << "No Google Maps found on " << url << " Unmapable." << endl;
				unmappable.push_back(placemark);
			}

		} catch(exception& e) {
			if(verbose) cerr << "ERROR:  " << e.what() << endl;
		}
	}

	double mid_lat, mid_lon;
	bbox.GetCenter(&mid_lat, &mid_lon);
	FolderPtr unmapableListings = factory->CreateFolder();
	unmapableListings->set_name("Unmapable Listings");
	
	// Add it to the main KML object
	root->add_feature(unmapableListings);  // kml takes ownership.

	for(int i=0; i<unmappable.size(); i++)
	{
		// Create <coordinates>.
		CoordinatesPtr coordinates = factory->CreateCoordinates();
		
		// Create <coordinates>-122.0816695,37.42052549<coordinates>
		coordinates->add_latlng(mid_lat, mid_lon);
		
		// Create <Point> and give it <coordinates>.
		PointPtr point = factory->CreatePoint();
		point->set_coordinates(coordinates);  // point takes ownership
		
		unmappable[i]->set_geometry(point);  // placemark takes ownership
		
		// Add it to the folder
		unmapableListings->add_feature(unmappable[i]);
		
	}
	
	
	// Serialize to XML
	std::string xml = SerializePretty(kml);

	// Write it to file
	if(outfile.is_open())
	{
		outfile << xml;
		outfile.close();
	}
	else
	{
		cout << xml << endl;
	}

	/* Shutdown libxml */
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