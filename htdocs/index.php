<?php
require_once("config.php");
?>
<!DOCTYPE HTML>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>Craig2KML</title>
<style>
body {
	width: 700px;
	border-left: 1px solid black;
	border-right: 1px solid black;
	margin: 0 auto 0 auto;
	padding: 10px;
}
</style>
</head>

<body>
	<h1>Craig2KML</h1>
	
	<p>Craig2KML takes a Craigslist search page and  maps them on Google Maps.</p>
	
	<img src="img/craigslist.jpg" width="335" />
	<img src="img/map.jpg" width="335" />
	
	
	<p>Source code is <a href="https://github.com/jefftimesten/craig2kml">available at github</a>.</p>
	
	<h2>Option 1: Form</h2>
	<p>Paste a Craigslist search link into the field below.  </p>
	<form action="craig2kml.php">
		<input type="text" size="100" name="url" />
		<input type="submit" value="Submit" />
	</form>
	
	<h2>Option 2: Bookmarklet</h2>
	<p>Drag this link to your bookmark bar.  Then, when you are on the Craigslist search page, 
	click the bookmark.</p>
	<a href="javascript:location.href='<?=$urlbase?>/craig2kml.php?url='+encodeURIComponent(location.href);">craig2kml</a>
</body>
</html>
