<?php

/* Copyright 2010 Gary Briggs

This file is part of obdgpslogger.

obdgpslogger is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

obdgpslogger is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with obdgpslogger.  If not, see <http://www.gnu.org/licenses/>.
*/

# Live updates for obdgpslogger via google earth

# This works in three stages:
#  Stage 0: Present UI and options in HTML. Submitting triggers Stage 1
#  Stage 1: Download seed KML containing options and link to stage 2
#  Stage 2: Actual KML


# Borrows code from http://code.google.com/apis/kml/articles/phpmysqlkml.html
#   to create kml

include_once("styles.php");

# Length in seconds of sample to output
$samplelength = 10;
if(!empty($_REQUEST['samplelength'])) {
	$samplelength = (int)$_REQUEST['samplelength'];
}

# Target mpg
$targetmpg = 20;
if(!empty($_REQUEST['targetmpg'])) {
	$targetmpg = (int)$_REQUEST['targetmpg'];
}

# Time to go back [from now]
$startdelta = 10;
if(!empty($_REQUEST['startdelta'])) {
	$startdelta = (int)$_REQUEST['startdelta'];

	# If they request "current", make sure it matches the sample length
	if(-1 == $startdelta) {
		$startdelta = $samplelength;
	}
}

# Update frequency
$updaterate = 4;
if(!empty($_REQUEST['updaterate'])) {
	$updaterate = (int)$_REQUEST['updaterate'];
}

# Database filename
$dbfilename = "ces2010.db";
if(!empty($_REQUEST['dbfilename'])) {
	$dbfilename = $_REQUEST['dbfilename'];
}

# Debug mode. Mostly just changes the content-type so the browser will render.
$debug = 0;

# Internal prefix on styles. Mainly a uniquefying thing
$styleprefix = "LiveOBDKMLStyle";

# Number of different colors to allow
$stylecount = 2;

# Stage 0: UI of options
function stage0() {
	global $samplelength, $startdelta, $dbfilename, $targetmpg, $styleprefix;

	# Yay magic numbers. This is the first time in the ces2010 db
	$ces2010start = time() - 1262889649;

	print <<< EOF

	<HTML>
	<HEAD><TITLE>Launch OBDGPSlogger Live</TITLE></HEAD><BODY>
	<H1>Launch OBDGPSlogger Live</H1>
	<FORM METHOD="GET">
	<INPUT TYPE="hidden" NAME="stage" VALUE="1">
	<TABLE>
	<TR><TD>Target MPG</TD><TD><INPUT TYPE="text" NAME="targetmpg" VALUE="$targetmpg"></TD></TR>
	<TR><TD>Sample Length (Seconds)</TD><TD><INPUT TYPE="text" NAME="samplelength" VALUE="$samplelength"></TD></TR>
	<TR><TD>Start Time</TD><TD>
		<SELECT NAME="startdelta">
			<OPTION VALUE="-1">Live Data</option>
			<OPTION VALUE="$ces2010start" SELECTED>Start of ces2010.db</option>
		</SELECT>
	</TD></TR>
	<TR><TD>Update Rate</TD><TD>
		<SELECT NAME="updaterate">
			<OPTION VALUE="-1">Never</option>
			<OPTION VALUE="1">1 Second</option>
			<OPTION VALUE="2">2 Seconds</option>
			<OPTION VALUE="4" SELECTED>4 Seconds</option>
			<OPTION VALUE="8">8 Seconds</option>
			<OPTION VALUE="16">16 Seconds</option>
			<OPTION VALUE="32">32 Seconds</option>
		</SELECT>
	</TD></TR>
	<TR><TD>DB File</TD><TD><INPUT TYPE="text" NAME="dbfilename" VALUE="$dbfilename"></TD></TR>
	<TR><TD><INPUT TYPE="Submit"></TD></TR>
	</FORM>
EOF;

	print <<< EOF

	</BODY></HTML>
EOF;
}

# Stage 1: Seed KML to open in Google Earth
function stage1() {
	global $samplelength, $startdelta, $dbfilename, $updaterate, $targetmpg, $styleprefix;

	global $debug;
	if($debug) {
		header('Content-type: text/plain');
	} else {
		header('Content-type: application/vnd.google-earth.kml+xml');
		header('Content-Disposition: attachment; filename=liveobdseed.kml');
	}

	$url = "http://" . $_SERVER['SERVER_NAME'] . filter_var($_SERVER['PHP_SELF'], FILTER_SANITIZE_STRING) .
		"?startdelta=$startdelta" . "&" .
		"samplelength=$samplelength" . "&" .
		"dbfilename=$dbfilename" . "&" .
		"targetmpg=$targetmpg" . "&" .
		"stage=2";

	print <<< EOF
	<kml xmlns="http://www.opengis.net/kml/2.2">
		<NetworkLink>
    			<name>OBDGPSLogger live updates</name>
    			<Link>
				<href><![CDATA[
				$url
				]]></href>
EOF;

	if($updaterate > 0) {
		print <<< EOF
    				<refreshMode>onInterval</refreshMode>
    				<refreshInterval>$updaterate</refreshInterval>
EOF;
	}
	print <<< EOF
			</Link>
		</NetworkLink>
	</kml>
EOF;
	exit(0);

}

# Stage 2: Actual kml data
function stage2() {
	global $samplelength, $startdelta, $dbfilename, $targetmpg, $styleprefix, $stylecount;

	global $debug;

	$db = new PDO("sqlite:$dbfilename");
	if(!$db) {
		print("Error opening database $dbfilename\n");
		exit(1);
	}


	$sql = "SELECT gps.lat AS lat,gps.lon AS lon,obd.vss AS vss,(7.107 * obd.vss/obd.maf) AS mpg " .
		"FROM gps LEFT JOIN obd ON gps.time=obd.time " .
		"WHERE gps.time >= :starttime " .
		"AND gps.time <= :endtime";

	$stmt = $db->prepare($sql);
	if(!$stmt) {
		header("Content-type: text/plain");
		print("Couldn't prepare statement\n$sql\n");
		print_r($db->errorInfo());
		exit(1);
	}

	$starttime = time() - $startdelta;
	$endtime = $starttime + $samplelength;
	$stmt->execute(array(':starttime' => $starttime, ':endtime' => $endtime));
	

	// Creates the Document.
	$dom = new DOMDocument('1.0', 'UTF-8');

	// Creates the root KML element and appends it to the root document.
	$node = $dom->createElementNS('http://earth.google.com/kml/2.2', 'kml');
	$parNode = $dom->appendChild($node);

	// Creates a KML Document element and append it to the KML element.
	$dnode = $dom->createElement('Document');
	$dnode->setAttribute('id', 'livegpspos');
	$docNode = $parNode->appendChild($dnode);

	// Set up the styles we need
	planStyles($stylecount, $styleprefix, $dom, $dnode);

	$gaugesnode = $dom->createElement('Folder');
	$nameNode = $dom->createElement('name', 'Gauges');
	$gaugesnode->appendChild($nameNode);
	$dnode->appendChild($gaugesnode);

	$overlayNode = $dom->createElement('ScreenOverlay');
	$nameNode = $dom->createElement('name', 'Vehicle Speed');
	$overlayNode->appendChild($nameNode);

	$overlayXYNode = $dom->createElement('overlayXY');
	$overlayXYNode->setAttribute('x', '0');
	$overlayXYNode->setAttribute('y', '1');
	$overlayXYNode->setAttribute('xunits', 'fraction');
	$overlayXYNode->setAttribute('yunits', 'fraction');
	$overlayNode->appendChild($overlayXYNode);

	$screenXYNode = $dom->createElement('screenXY');
	$screenXYNode->setAttribute('x', '0');
	$screenXYNode->setAttribute('y', '1');
	$screenXYNode->setAttribute('xunits', 'fraction');
	$screenXYNode->setAttribute('yunits', 'fraction');
	$overlayNode->appendChild($screenXYNode);


	$iconNode = $dom->createElement('Icon');
	$gaugedelta = time() - $endtime;
	$gaugeurl = "http://" . $_SERVER['SERVER_NAME'] .
		dirname(filter_var($_SERVER['PHP_SELF'], FILTER_SANITIZE_STRING)) .
		'/gauge.php' .
                "?startdelta=$gaugedelta" . 
		"&amp;datacolumn=vss" . 
		"&amp;datamin=0" . 
		"&amp;datamax=255" . 
		"&amp;dataname=Vehicle Speed";

	$hrefNode = $dom->createElement('href', $gaugeurl);

	$iconNode->appendChild($hrefNode);
	$overlayNode->appendChild($iconNode);
	$gaugesnode->appendChild($overlayNode);


	// Human friendly name for the trace
	$nameNode = $dom->createElement('name', "Height=>speed, color=>mpg");
	$dnode->appendChild($nameNode);

	$docStyleNode = $dom->createElement('Style');
	$dnode->appendChild($docStyleNode);

	$listStyleNode = $dom->createElement('ListStyle');
	$docStyleNode->appendChild($listStyleNode);

	$hideChildNode = $dom->createElement('listItemType', 'checkHideChildren');
	$listStyleNode->appendChild($hideChildNode);

	$coorStr = "";
	$mpg = -1;
	$currstyle = "";
	$laststyle = "";
	$lastlon = $lastlat = 0;
	while(false != ($row = $stmt->fetch(PDO::FETCH_ASSOC))) {
		if($lastlon == $row['lon'] && $lastlat == $row['lat']) {
			continue;
		}
		$lastlon = $row['lon'];
		$lastlat = $row['lat'];


		// Creates a coordinates element and gives it the value of the lng and lat columns from the results.
		$coorStr .= $row['lon'] . ','  . $row['lat'] . ','  . $row['vss'] . "\n";

		$mpg = $row['mpg'];

		# print $mpg . "\n";
		if($mpg >= $targetmpg) {
			$currstyle = $styleprefix . 'Green';
		} else {
			$currstyle = $styleprefix . 'Red';
		}

		if($laststyle != $currstyle && "" != $currstyle) {
			renderLineString($dom, $docNode, $coorStr, $laststyle);
			$coorStr = $row['lon'] . ','  . $row['lat'] . ','  . $row['vss'] . "\n";
		}
		$laststyle = $currstyle;
	}
	renderLineString($dom, $docNode, $coorStr, $currstyle);

	if($debug) {
		header('Content-type: text/plain');
	} else {
		header('Content-type: application/vnd.google-earth.kml+xml');
		header('Content-Disposition: attachment; filename=liveobd.kml');
	}

	$kmlOutput = $dom->saveXML();
	echo $kmlOutput;
}

$stage = 0;
if(!empty($_REQUEST['stage'])) {
	$stage = (int)$_REQUEST['stage'];
}

switch($stage) {
	case 1:
		stage1();
		break;
	case 2:
		stage2();
		break;
	case 0:
	default:
		stage0();
		break;
}

?>

