function HTTPGET(url) {
	var req = new XMLHttpRequest();
	req.open("GET", url, false);
	req.send(null);
	return req.responseText;
}

var getStats = function() {
	var api_key, site_token, data, response, visitors, max, avg, active, meta, dict;

	api_key = window.localStorage.getItem('api_key');
	site_token = window.localStorage.getItem('site_token');

	if (api_key === null || site_token === null) {
		dict = {
			"KEY_ERROR": 'No keys found. Please check your settings.'
		};

		Pebble.sendAppMessage(dict);
		return;
	}

	data = HTTPGET("https://api.gosquared.com/now/v3/overview?api_key="+api_key+"&site_token="+site_token);
	response = JSON.parse(data);
	visitors = response.visitors;
	max = response.summary.max;
	avg = response.summary.avg;
	avg = avg.toFixed(2);
	active = response.active;
	percentage = Math.round((visitors / max * 100) * 1.8) + 180;
	meta = active + ' active \navg: ' + avg + ' max: ' + max

	dict = {
		"KEY_PERCENTAGE": percentage,
		"KEY_VISITORS": visitors,
		"KEY_STATS": meta
	};

	Pebble.sendAppMessage(dict);
};

Pebble.addEventListener("showConfiguration", function() {
  Pebble.openURL('http://pebble-config.herokuapp.com/config?title=GoSquared&fields=apikey_apiKey,siteToken_siteToken');
});

Pebble.addEventListener("webviewclosed", function(e) {
  var options = JSON.parse(decodeURIComponent(e.response));
  
  window.localStorage.setItem('site_token', options.siteToken);
  window.localStorage.setItem('api_key', options.apiKey);
});

Pebble.addEventListener("ready",
	function(e) {
		getStats();
	}
);