/*
 *  Craig2KML.h
 *  craig2kml
 *
 *  Created by Jeffrey Crouse on 2/16/11.
 *  Copyright 2011 Eyebeam. All rights reserved.
 *
 */

#include <iostream>
#include <kml/dom.h>
#include <kml/engine.h>

using kmldom::CoordinatesPtr;
using kmldom::KmlPtr;
using kmldom::KmlFactory;
using kmldom::PlacemarkPtr;
using kmldom::PointPtr;
using kmldom::FolderPtr;
using kmlengine::Bbox;
using namespace std;

class Craig2KML {
public:
	
	KmlFactory* factory;
	KmlPtr kml;
	FolderPtr rootFolder;
	vector<PlacemarkPtr> mappablePlacemarks;
	vector<PlacemarkPtr> unmappablePlacemarks;
	Bbox bbox;
	
	Craig2KML(string title, bool verbose) {

		factory = KmlFactory::GetFactory();
		kml = factory->CreateKml();
		
		
		// Create the root folder
		rootFolder = factory->CreateFolder();
		rootFolder->set_name("Craig2KML");
		
		// Add it to the main KML object
		kml->set_feature(rootFolder);  // kml takes ownership.
		
		// Create the description for the main folder
		time_t t = time(0); //obtain the current time_t value
		tm now=*localtime(&t); //convert it to tm
		char tmdescr[255]={0};
		strftime(tmdescr, sizeof(tmdescr)-1, "%A, %B %d %Y. %X", &now);
		char desc[1024];
		sprintf(desc, "\"%s\" on %s", title.c_str(), tmdescr);
		if(verbose) cerr << desc << endl;
		rootFolder->set_description(desc);
	}
	
	string serialize()
	{
		FolderPtr mappableListings = factory->CreateFolder();
		mappableListings->set_name("Mappable Listings");
		rootFolder->add_feature(mappableListings);  // kml takes ownership.
		
		
		FolderPtr unmappableListings= factory->CreateFolder();
		unmappableListings->set_name("Unmappable Listings");
		rootFolder->add_feature(unmappableListings);  // kml takes ownership.
		
		double mid_lat, mid_lon;
		bbox.GetCenter(&mid_lat, &mid_lon);
		
		// Set the position of all the unmapable listings
		for(int i=0; i<unmappablePlacemarks.size(); i++)
		{
			CoordinatesPtr coordinates = factory->CreateCoordinates();
			coordinates->add_latlng(mid_lat, mid_lon);
			
			PointPtr point = factory->CreatePoint();
			point->set_coordinates(coordinates);
			
			unmappablePlacemarks[i]->set_geometry(point);  
			
			unmappableListings->add_feature( unmappablePlacemarks[i] );
		}
		
		for(int i=0; i<mappablePlacemarks.size(); i++)
		{
			mappableListings->add_feature(mappablePlacemarks[i]);	
		}
		
		return SerializePretty(kml);	
	}
	
	void addMappable(string title, string description, float lat, float lng)
	{
		bbox.ExpandLatLon(lat, lng);
		
		PlacemarkPtr placemark = factory->CreatePlacemark();
		placemark->set_name(title);
		placemark->set_description(description);
	
		CoordinatesPtr coordinates = factory->CreateCoordinates();
		coordinates->add_latlng(lat, lng);
		
		PointPtr point = factory->CreatePoint();
		point->set_coordinates(coordinates);  // point takes ownership
		
		placemark->set_geometry(point);  // placemark takes ownership
		
		mappablePlacemarks.push_back(placemark);
		
	}
	
	void addUnmappable(string title, string description)
	{
		PlacemarkPtr placemark = factory->CreatePlacemark();
		placemark->set_name(title);
		placemark->set_description(description);
		
		unmappablePlacemarks.push_back(placemark);
	}
};