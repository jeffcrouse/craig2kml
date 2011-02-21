/*
 *  Craig2KML.cpp
 *  craig2kml
 *
 *  Created by Jeffrey Crouse on 2/16/11.
 *  Copyright 2011 Eyebeam. All rights reserved.
 *
 */

#include "Craig2KML.h"

Craig2KML::Craig2KML(string title, bool verbose) {
	
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

string Craig2KML::serialize()
{
	int total = mappablePlacemarks.size() + unmappablePlacemarks.size();
	char name[255];
	
	
	FolderPtr mappableListings = factory->CreateFolder();
	sprintf(name, "Mappable Listings (%d/%d)", (int)mappablePlacemarks.size(), total);
	mappableListings->set_name(name);
	rootFolder->add_feature(mappableListings);  // kml takes ownership.
	
	
	FolderPtr unmappableListings= factory->CreateFolder();
	sprintf(name, "Unmappable Listings (%d/%d)", (int)unmappablePlacemarks.size(), total);
	unmappableListings->set_name(name);
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

void Craig2KML::addMappable(string title, string description, float lat, float lng)
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

void Craig2KML::addUnmappable(string title, string description)
{
	PlacemarkPtr placemark = factory->CreatePlacemark();
	placemark->set_name(title);
	placemark->set_description(description);
	
	unmappablePlacemarks.push_back(placemark);
}