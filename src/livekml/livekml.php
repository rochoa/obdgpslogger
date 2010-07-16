<?php
# Live updates for obdgpslogger via google earth

# This works in three stages:
#  Stage 0: Present UI and options in HTML. Submitting triggers Stage 1
#  Stage 1: Download seed KML containing options and link to stage 2
#  Stage 2: Actual KML


# Borrows code from http://code.google.com/apis/kml/articles/phpmysqlkml.html
#   to create kml

# Length in seconds of sample to output
$samplelength = 10;
if(!empty($_REQUEST['samplelength'])) {
	$samplelength = (int)$_REQUEST['samplelength'];
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

$debug = 0;

# Stage 0: UI of options
function stage0() {
	global $samplelength, $startdelta, $dbfilename;

	# Yay magic numbers. This is the first time in the ces2010 db
	$ces2010start = time() - 1262889649;

	print <<< EOF

	<HTML>
	<HEAD><TITLE>Launch OBDGPSlogger Live</TITLE></HEAD><BODY>
	<H1>Launch OBDGPSlogger Live</H1>
	<FORM METHOD="GET">
	<INPUT TYPE="hidden" NAME="stage" VALUE="1">
	<TABLE>
	<TR><TD>Sample Length (Seconds)</TD><TD><INPUT TYPE="text" NAME="samplelength" VALUE="$samplelength"></TD></TR>
	<TR><TD>Start Time</TD><TD>
		<SELECT NAME="startdelta">
			<OPTION VALUE="-1">Live Data</option>
			<OPTION VALUE="$ces2010start">Start of ces2010.db</option>
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
	global $samplelength, $startdelta, $dbfilename, $updaterate;

	global $debug;
	if($debug) {
		header('Content-type: text/plain');
	} else {
		header('Content-type: application/vnd.google-earth.kml+xml');
		header('Content-Disposition: attachment; filename=liveobdseed.kml');
	}

	$url = "http://" . $_SERVER['SERVER_NAME'] . $_SERVER['PHP_SELF'] .
		"?startdelta=$startdelta&samplelength=$samplelength&dbfilename=$dbfilename&stage=2";

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
	global $samplelength, $startdelta, $dbfilename;

	global $debug;

	if($debug) {
		header('Content-type: text/plain');
	} else {
		header('Content-type: application/vnd.google-earth.kml+xml');
		header('Content-Disposition: attachment; filename=liveobd.kml');
	}

	$db = new PDO('sqlite:ces2010.db');
	if(!$db) {
		print("Error opening database\n");
		exit(1);
	}


	$sql = "SELECT gps.lat AS lat,gps.lon AS lon, obd.vss AS vss FROM " .
		"gps LEFT JOIN obd ON gps.time=obd.time " .
		"WHERE gps.time >= :starttime " .
		"AND gps.time <= :endtime";

	$stmt = $db->prepare($sql);
	if(!$stmt) {
		print("Couldn't prepare statement $sql\n");
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
	$docNode = $parNode->appendChild($dnode);


	// Creates a Placemark and append it to the Document.
	$node = $dom->createElement('Placemark');
	$placeNode = $docNode->appendChild($node);

	// Creates an id attribute and assign it the value of id column.
	$placeNode->setAttribute('id', 'livegpspos');

	// Human friendly name for the trace
	$pointNode = $dom->createElement('name', "Height represents speed");
	$placeNode->appendChild($pointNode);

	// Creates a Point element.
	$pointNode = $dom->createElement('LineString');
	$placeNode->appendChild($pointNode);

	$coorStr = "";
	while(false != ($row = $stmt->fetch(PDO::FETCH_ASSOC))) {

		// Creates a coordinates element and gives it the value of the lng and lat columns from the results.
		$coorStr .= $row['lon'] . ','  . $row['lat'] . ','  . $row['vss'] . "\n";

		$lastlon = $row['lon'];
		$lastlat = $row['lat'];
	}

	$coorNode = $dom->createElement('coordinates', $coorStr);
	$pointNode->appendChild($coorNode);

	$extrudeNode = $dom->createElement('extrude', '1');
	$pointNode->appendChild($extrudeNode);

	$tessellateNode = $dom->createElement('tessellate', '1');
	$pointNode->appendChild($tessellateNode);

	$altitudeModeNode = $dom->createElement('altitudeMode', 'relativeToGround');
	$pointNode->appendChild($altitudeModeNode);

	$kmlOutput = $dom->saveXML();
	echo $kmlOutput;
}

$stage = 0;
if(!empty($_REQUEST['stage'])) {
	$stage = $_REQUEST['stage'];
}

switch($stage) {
	case 1:
		stage1();
		break;
	case 2:
		stage2();
		break;
	default:
		stage0();
		break;
}

?>

