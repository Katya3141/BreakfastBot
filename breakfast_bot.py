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
		<option value="2l0l1l0l">413</option>
		<option value="2l0l1r0r">412</option>
		<option value="2l0r1l0l1r0r">Flounge</option>
		<option value="2l0r1l0l2l0l1l0l">422</option>
		<option value="2l0r1l0l2l0l1r0r">423</option>
		<option value="2l0r1l0l2l0l2r0r">424</option>
	</select>

	<button onClick="myFunction()" >POST to server</button>

	<br>

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
	</script>
</body>
</html>
"""
