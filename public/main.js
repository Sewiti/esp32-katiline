const tempPollMS = 15000;  // 15sec
const histPollMS = 300000; // 5min

var lastUpdateTemp = 0;
var lastUpdateChart = 0;
var hist = [];
var histChart;

const updateTemp = () => {
	lastUpdateTemp = Date.now();
	fetch("temp")
		.then(res => res.json())
		.then(data => {
			let temp = data.tempC.toFixed(1);

			document.title = `Katilo temperatūra ${temp}°C`;
			document.getElementById("temp").innerText = temp;
		})
		.catch(reason => console.log(reason));
};

const updateHist = (fresh = false) => {
	lastUpdateChart = Date.now();
	fetch("hist")
		.then(res => {
			return res.text();
		})
		.then(body => parseHist(body))
		.then(data => {
			if (fresh) {
				hist = data;
			} else {
				for (let i = 0; i < data.length; i++) {
					appendHist(data[i]);
				}
			}
			updateHistChart();
			if (fresh) { scrollToEnd(); }
		})
		.catch(reason => console.log(reason));
};

const updateHistChart = () => {
	histChart.data.labels = [];
	histChart.data.datasets[0].data = [];

	for (let i = 0; i < hist.length; i++) {
		let label = hist[i].date.toLocaleString("lt", {
			month: "long",
			day: "numeric",
			hour: "numeric",
			minute: "numeric"
		});
		label = label.charAt(0).toUpperCase() + label.slice(1);

		histChart.data.labels.push(label);
		histChart.data.datasets[0].data.push(hist[i].temp);
	}

	let e = document.getElementById("hist").parentElement;
	let maxWidth = e.parentElement.clientWidth;
	let width = Math.max(hist.length * 6, maxWidth);
	e.style.width = width + "px";
	histChart.update();
};

const appendHist = (v) => {
	let i;
	for (i = 0; i < hist.length; i++) {
		if (hist[i].date.getTime() == v.date.getTime()) { return; }
		if (hist[i].date.getTime() > v.date.getTime()) { break; }
	}
	hist = [...hist.slice(0, i), v, ...hist.slice(i)];
};

const parseHist = (body) => {
	let data = [];
	let lines = body.split("\n");
	for (let i = 0; i < lines.length; i++) {
		let parts = lines[i].split(",");
		if (parts.length != 2) { continue; }
		data.push({
			date: new Date(
				+parts[0].substr(0, 4),	 	// Year
				+parts[0].substr(5, 2) - 1, // Month
				+parts[0].substr(8, 2),	 	// Day
				+parts[0].substr(11, 2),	// Hour
				+parts[0].substr(14, 2)	 	// Minute
			),
			temp: +parts[1]
		});
	}
	return data;
};

const scrollToEnd = () => {
	let e = document.getElementById("hist").parentElement.parentElement;
	e.scrollLeft = e.scrollWidth;
};

window.addEventListener("focus", () => {
	if (Date.now() - lastUpdateTemp > tempPollMS * 2) { updateTemp(); }
	if (Date.now() - lastUpdateChart > histPollMS * 2) { updateHist(true); }
});

window.addEventListener("load", () => {
	var style = getComputedStyle(document.body);
	let primaryCol = style.getPropertyValue("--primary-color");
	let primaryColAlt = style.getPropertyValue("--primary-color-60");
	let fgCol = style.getPropertyValue("--fg-color");

	histChart = new Chart(document.getElementById("hist"), {
		type: "line",
		data: {
			datasets: [{
				fill: {
					above: primaryColAlt,
					target: "origin"
				},
				borderColor: primaryCol,
				borderWidth: 4,
				hoverBorderWidth: 0,
				pointBackgroundColor: primaryCol,
				hoverBackgroundColor: primaryCol,
				pointRadius: 0,
				pointHoverRadius: 8,
				pointHitRadius: 32,
				tension: 0.4
			}]
		},
		options: {
			scales: {
				x: {
					grid: { color: fgCol + "2" } // Risky alpha
				},
				y: {
					grid: { color: fgCol + "2" }, // Risky alpha
					position: "right",
					suggestedMax: 80,
					suggestedMin: 50
				}
			},
			plugins: {
				legend: { display: false },
				tooltip: {
					displayColors: false,
					callbacks: { label: ctx => ctx.raw.toFixed(1) + "°C" }
				}
			},
			maintainAspectRatio: false,
			animation: false,
		}
	});

	updateTemp();
	updateHist(true);
	setInterval(updateTemp, tempPollMS);
	setInterval(() => { updateHist(); }, histPollMS);
});
