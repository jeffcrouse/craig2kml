
#include <iostream>
#include <kml/dom.h>
#include "Webpage.h"
#include <fstream>

using namespace std;
using kmldom::CoordinatesPtr;
using kmldom::KmlPtr;
using kmldom::KmlFactory;
using kmldom::PlacemarkPtr;
using kmldom::PointPtr;

#define VERSION 0.1


void help() {
	cout << "typical: (-u|--url ) #### [(-o|--outfile) ####]" << endl;
	cout << "  where:" << endl;
	cout << "  -u (--url) [required]" << endl;
	cout << "      the Craigslist search page URL to be translated" << endl;
	cout << "  -o (--outfile) is the file in which the kml will be saved" << endl;
	cout << "     prints to stdout if no file is provided." << endl;
	cout << "  -c (--config) use custom config values";
	cout << "  -v (--verbose) print messages to stderr";
}

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
	
	
	const char* outfilepath=NULL;
	const char* url=NULL;
	const char* configfilename=NULL;
	bool verbose=false;
	ofstream outfile;
	
	
	// Parse command line options.
	for(int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--outfile") == 0)
		{
			if (i+1 == argc) {
				// error messages intermingled with parsing logic
				cerr << "Invalid " << argv[i];
				cerr << " parameter: no outfile specified\n";
				help();
				exit(1); // multiple exit points in parsing algorithm
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

	// We can't do anything without a URL
	if(url==NULL)
	{
		help();
		exit(1);
	}
	
	// If the user provided an output file, try to open it.
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
		string line, name, value;
		ifstream configfile(configfilename);
		if (configfile.is_open())
		{
			while( configfile.good() )
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
			cerr << "Unable to open configfile" << endl; 
		}
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
		cerr << "Retrieved " << numLinks << " links." << endl;
	
	// Get the factory singleton to create KML elements.
	KmlFactory* factory = KmlFactory::GetFactory();

	// Create <kml>
	KmlPtr kml = factory->CreateKml();
	
	// Make a folder for all of the mapable listings
	kmldom::FolderPtr folder = factory->CreateFolder();
	folder->set_name("Mapable Listings");
	
	// Add it to the main KML object
	kml->set_feature(folder);  // kml takes ownership.
	
	
	// Loop through all of the links on the page.
	int i=0;
	for(it = links.begin(); it != links.end(); ++it)
	{
		string title = it->first;
		string url = it->second;
		if(verbose)
			cerr << "Parsing " << i << " out of " << numLinks << ": " << title << endl;
		i++;
		
		try {
			Webpage listing(url);
			string addr = listing.getNodeAttribute(config["craigslist_google_maps_link"], "href");
			addr.erase(0, config["craigslist_google_maps_link_prefix"].length());
			
			string description = listing.getNodeAsString(config["craigslist_item_description"]);
			
			if(addr.empty())
			{
				if(verbose) cerr << "No Google Maps found on " << url << " Skipping." << endl;
				continue;
			}

			string geocodeURL = config["google_geocode_base"] + addr;
			
			if(verbose) cerr << "calling " << geocodeURL << endl;
			
			Webpage geocode(geocodeURL, true);
			const char* status = geocode.getNodeContents(config["google_geocode_status"]);
			if(strcmp(status, "OK")!=0)
			{
				if(verbose) cerr << "geocode failed for " << addr << endl;	
				continue;
			}

		
			float lat = atof(geocode.getNodeContents(config["google_geocode_lat"]));
			float lng = atof(geocode.getNodeContents(config["google_geocode_lng"]));
			
			if(verbose) cerr << "Adding placemark at " << lat << ", " << lng << endl;
			
			// Create <coordinates>.
			CoordinatesPtr coordinates = factory->CreateCoordinates();
			
			// Create <coordinates>-122.0816695,37.42052549<coordinates>
			coordinates->add_latlng(lat, lng);
			
			// Create <Point> and give it <coordinates>.
			PointPtr point = factory->CreatePoint();
			point->set_coordinates(coordinates);  // point takes ownership
			
			// Create <Placemark> and give it a <name> and the <Point>.
			PlacemarkPtr placemark = factory->CreatePlacemark();
			placemark->set_name(title);
			placemark->set_geometry(point);  // placemark takes ownership
			placemark->set_description(description);
			
			// Add it to the folder
			folder->add_feature(placemark);		
		} catch(exception& e) {
			if(verbose)
				cerr << e.what() << endl;
			continue;
		}
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
