
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
	cout << "typical: (-u|--url ) #### [(-o|--outfile) ####] \n";
	cout << "  where:\n";
	cout << "  -u (--url) [required]\n";
	cout << "     is the Craigslist search page URL to be translated\n";
	cout << "  -o (--outfile) is the file in which the kml will be saved";
	cout << "     defaults to 'craigslist.kml' if not specified.\n";
}

int main (int argc, char* argv[])
{
	const char* outfile="craigslist.kml";
	const char* url=NULL;

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
			outfile = argv[++i];  // parsing action goes here
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
		else if (strcmp(argv[i], "--version") == 0) {
			fprintf(stderr, "craig2kml version %f", VERSION);
			exit(0);
		}
	}

	if(url==NULL) {
		help();
		exit(1);
	}

	/* Set up libXML */
	xmlInitParser();
	
	// Make sure we can open the output file.
	ofstream myfile;
	myfile.open(outfile);
	if(!myfile.is_open()) {
		cerr << "Couldn't open " << outfile << " for writing." << endl;
		return -1;
	}
	
	// Create the main page with all of the listings
	cerr << "opening " << url << endl;
	Webpage listingsPage(url);
	

	// All of the listings are within /body/blockquote/p
	// These are the links to individual listing pages
	map<string,string> links;
	map<string,string>::iterator it;
	links = listingsPage.getLinks("/body/blockquote/p/a");
	int numLinks = links.size();
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
		cout << "Parsing " << i << " out of " << numLinks << ": " << title << endl;
		try {
			Webpage listing(url);
			
			string addr = listing.getGoogleAddress();
			string description = listing.getNodeAsString("//div[@id='userbody']");
			
			if(!addr.empty())
			{
				char geocodeURL[255];
				sprintf(geocodeURL, "http://maps.googleapis.com/maps/api/geocode/xml?sensor=false&address=%s", addr.c_str());
				
				Webpage geocode(geocodeURL, true);
				
				
				float* latlng = geocode.getGoogleLatLng();
				
				if(latlng == NULL) {
					cout << "geocode failed for " << addr << endl;	
					continue;
				}
				
				// Create <coordinates>.
				CoordinatesPtr coordinates = factory->CreateCoordinates();
				// Create <coordinates>-122.0816695,37.42052549<coordinates>
				coordinates->add_latlng(latlng[0],latlng[1]);
				
				// Create <Point> and give it <coordinates>.
				PointPtr point = factory->CreatePoint();
				point->set_coordinates(coordinates);  // point takes ownership
				
				// Create <Placemark> and give it a <name> and the <Point>.
				PlacemarkPtr placemark = factory->CreatePlacemark();
				placemark->set_name(title);
				placemark->set_geometry(point);  // placemark takes ownership
				placemark->set_description(description);
				
				folder->add_feature(placemark);
			}
		} catch(exception& e) {
			cout << e.what() << endl;
			continue;
		}
		i++;
	}

	
	// Serialize to XML
	std::string xml = kmldom::SerializePretty(kml);

	// Write it to file
	myfile << xml;
	myfile.close();
	
	
	/* Shutdown libxml */
    xmlCleanupParser();
}
