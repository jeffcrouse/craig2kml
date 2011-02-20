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

	Craig2KML(string title, bool verbose);
	string serialize();
	void addMappable(string title, string description, float lat, float lng);
	void addUnmappable(string title, string description);
	
protected:	
	KmlFactory* factory;
	KmlPtr kml;
	FolderPtr rootFolder;
	vector<PlacemarkPtr> mappablePlacemarks;
	vector<PlacemarkPtr> unmappablePlacemarks;
	Bbox bbox;
	
};