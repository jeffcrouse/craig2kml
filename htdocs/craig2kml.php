<?php
require_once("functions.php");
require_once("config.php");

$error=true;

// Make sure that a URL is set and that it is a valid Craigslist link
if(isset($_REQUEST['url']))
{	
	$url = urldecode($_REQUEST['url']);
	if(filter_var($url, FILTER_VALIDATE_URL, FILTER_FLAG_SCHEME_REQUIRED))
	{
		$filebase 	= 	rand_str();
		$kmlfile 	= 	"{$kmldir}/{$filebase}.kml";
		$kmlurl		=	"{$urlbase}/{$kmlfile}";
		$logfile	=	"{$kmldir}/{$filebase}.log";
		$logurl		=	"{$urlbase}/{$logfile}";
		$cmd = sprintf('LD_LIBRARY_PATH="%s:$LD_LIBRARY_PATH" %s -v -o "%s" -u "%s" -c %s -d %s', 
			$ld_library_path, 
			$craig2kml_exe, 
			$kmlfile, $url,
			$craig2kml_configfile,
			$craig2kml_cachedir);
		`$cmd &>$logfile &`;
		$error = false;
	}
}
?>
<!DOCTYPE HTML>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>Craig2KML</title>
<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.5/jquery.min.js"></script>

<script type="text/javascript">
$(document).ready(function(){
	<?php if(!$error): ?>
	load_log();  // If there are no errors, try to load the generated kml file
	<?php endif; ?>
});

var attempts=0;
function load_log() {
	attempts++;
	$.get("<?=$logfile?>", function(result){

		var lines = jQuery.trim(result).split("\n");
		var lastline = lines[lines.length-1];
		
		if(attempts > 15) {
			$("#result").html("Failed. Please try again.");
			return;
		} else if(lastline.match("^ERROR")) {
			$("#result").html(lastline);
			return;
		} else if(lastline.match("^DONE")) {
			$(location).attr('href', "http://maps.google.com/maps?q=<?=$kmlurl?>");
			return;
		} else {
			window.setTimeout(load_log, 1000); // wait a second, then load again
		}
	});
}
</script>
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
	
	<?php if($error): ?>
	<p>You must provide a Craigslist search page.</p>
	
	<p>Example:</p>
	<blockquote>
		http://newyork.craigslist.org/search/nfa/brk?query=williamsburg&srchType=A&minAsk=&maxAsk=2000&bedrooms=&addTwo=purrr
	</blockquote>
	<?php else: ?>

	<div id="result">
		<p>Loading <?=$url?></p>
		<img src="img/ajax-loader.gif" />
		
		<p><small><a href="<?=$logurl?>">logfile</a></small></p>
	</div>

	<?php endif; ?>
</body>
</html>
