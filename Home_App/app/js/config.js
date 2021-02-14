host = '89.136.134.190';	// hostname or IP address
//host = '192.168.0.16';	// hostname or IP address
port = 9001;
topic = '/home/#';		// topic to subscribe to
useTLS = false;
username = null;
password = null;
// username = "";
// password = "";

// path as in "scheme:[//[user:password@]host[:port]][/]path[?query][#fragment]"
//    defaults to "/mqtt"
//    may include query and fragment
//
// path = "/mqtt";
// path = "/data/cloud?device=12345";

cleansession = true;


topics = {
			"lampa":
				{
					"command":"/home/birou/lampa/command",
					"state":"/home/birou/lampa/state"
				},
			"neopixel_1":
				{
					"command":"/home/birou/neopixel_1/command",
					"state":"/home/birou/neopixel_1/state",
					"commandDim":"/home/birou/neopixel_1/commandDim",
					"stateDim":"/home/birou/neopixel_1/stateDim"
				},
			"neopixel_2":
				{
					"command":"/home/birou/neopixel_2/command",
					"state":"/home/birou/neopixel_2/state",
					"commandDim":"/home/birou/neopixel_2/commandDim",
					"stateDim":"/home/birou/neopixel_2/stateDim"
				},
			"neopixel_3":
				{
					"command":"/home/birou/neopixel_3/command",
					"state":"/home/birou/neopixel_3/state",
					"commandDim":"/home/birou/neopixel_3/commandDim",
					"stateDim":"/home/birou/neopixel_3/stateDim"
				},
			"debug": "/home/debug"
		};

colors = {
			"neopixel_1":"",
			"neopixel_2":"",
			"neopixel_3":""	
		};

range = {
			"neopixel_1":{
					"min":"",
					"max":""
			},
			"neopixel_2":{
					"min":"",
					"max":""
			},
			"neopixel_3":{
					"min":"",
					"max":""
			}
};