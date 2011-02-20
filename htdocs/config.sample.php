<?php
// where is the script installed?
$urlbase="http://www.mysite.com";

// If you hve installed the prerequisite libraries in a non-standard location,
// you should specify it here.
$ld_library_path="/path/to/lib";

// Path to the craig2kml executable
$craig2kml_exe = "/path/to/craig2kml";

// Config file
$craig2kml_configfile = "craig2kml.config";

// Directory in which to save cache files
// Make sure this directory and exists and that it is writable by craig2kml
$craig2kml_cachedir = "cache";

// Directory in which to save KML files
// Make sure this directory and exists and that it is writable by craig2kml
$kmldir = "kml";
?>