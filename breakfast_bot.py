def request_handler(request):
    if request['method'] == 'GET':
        return r"""<!DOCTYPE html>
<html>
<style>
	body{
		background-color:lightblue;
	}
	h1{
		display:inline-block;
		margin:10px;
		margin-top:0;
		padding:10px;
		border:5px solid black;
		border-radius: 5px;
	}
</style>

<head>
	<title>BreakfastBot</title>
	<h1>BreakfastBot Controller</h1>

	<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>
</head>

<body>
	<br>
	<select id="mydropdown">
		<option value="0">Brake</option>
		<option value="1">Forwards</option>
		<option value="2">Backwards</option>
		<option value="3">Turn right</option>
		<option value="4">Turn left</option>
	</select>

	<button onClick="myFunction()" >POST to server</button>

	<br>

	<button onClick="brake()" >Brake</button>
	<button onClick="forwards()" >Forwards</button>
	<button onClick="backwards()" >Backwards</button>
	<button onClick="right()" >Turn Right</button>
	<button onClick="left()" >Turn Left</button>

	<script type="text/javascript">
		function myFunction() {
			var e = document.getElementById("mydropdown");
		 	var command = e.options[e.selectedIndex].value;
		 	commandString = String(command);
		 	var XHR = new XMLHttpRequest();
			XHR.open('POST', 'http://iesc-s1.mit.edu/608dev/sandbox/pwang21/breakfastbot.py');
			// Add the required HTTP header for form data POST requests
			XHR.setRequestHeader('Content-Type', 'application/json');
			// Finally, send our data.
			XHR.send("{\"state\":\""+commandString+"\"}");
		}

		function brake() {
			//var e = document.getElementById("mydropdown");
		 	//var command = e.options[e.selectedIndex].value;
		 	//commandString = String(command);
		 	var XHR = new XMLHttpRequest();
			XHR.open('POST', 'http://iesc-s1.mit.edu/608dev/sandbox/pwang21/breakfastbot.py');
			// Add the required HTTP header for form data POST requests
			XHR.setRequestHeader('Content-Type', 'application/json');
			// Finally, send our data.
			XHR.send("{\"state\":\"0\"}");
		}
		function forwards() {
			//var e = document.getElementById("mydropdown");
		 	//var command = e.options[e.selectedIndex].value;
		 	//commandString = String(command);
		 	var XHR = new XMLHttpRequest();
			XHR.open('POST', 'http://iesc-s1.mit.edu/608dev/sandbox/pwang21/breakfastbot.py');
			// Add the required HTTP header for form data POST requests
			XHR.setRequestHeader('Content-Type', 'application/json');
			// Finally, send our data.
			XHR.send("{\"state\":\"1\"}");
		}
		function backwards() {
			//var e = document.getElementById("mydropdown");
		 	//var command = e.options[e.selectedIndex].value;
		 	//commandString = String(command);
		 	var XHR = new XMLHttpRequest();
			XHR.open('POST', 'http://iesc-s1.mit.edu/608dev/sandbox/pwang21/breakfastbot.py');
			// Add the required HTTP header for form data POST requests
			XHR.setRequestHeader('Content-Type', 'application/json');
			// Finally, send our data.
			XHR.send("{\"state\":\"2\"}");
		}
		function right() {
			//var e = document.getElementById("mydropdown");
		 	//var command = e.options[e.selectedIndex].value;
		 	//commandString = String(command);
		 	var XHR = new XMLHttpRequest();
			XHR.open('POST', 'http://iesc-s1.mit.edu/608dev/sandbox/pwang21/breakfastbot.py');
			// Add the required HTTP header for form data POST requests
			XHR.setRequestHeader('Content-Type', 'application/json');
			// Finally, send our data.
			XHR.send("{\"state\":\"3\"}");
		}
		function left() {
			//var e = document.getElementById("mydropdown");
		 	//var command = e.options[e.selectedIndex].value;
		 	//commandString = String(command);
		 	var XHR = new XMLHttpRequest();
			XHR.open('POST', 'http://iesc-s1.mit.edu/608dev/sandbox/pwang21/breakfastbot.py');
			// Add the required HTTP header for form data POST requests
			XHR.setRequestHeader('Content-Type', 'application/json');
			// Finally, send our data.
			XHR.send("{\"state\":\"4\"}");
		}
	</script>
</body>
</html>
"""
