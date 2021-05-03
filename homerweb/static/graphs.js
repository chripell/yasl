
const RANGE = /(\d+-\d+-\d+) +to +(\d+-\d+-\d+)/;

function formatDate(d) {
    var month = '' + (d.getMonth() + 1),
        day = '' + d.getDate(),
        year = d.getFullYear();
    if (month.length < 2)
        month = '0' + month;
    if (day.length < 2)
        day = '0' + day;
    return [year, month, day].join('-');
}

var chart = null;

function render(data) {
    if (data.result.hasOwnProperty('error')) {
	$('#status').html(data.result.error);
	return;
    }
    $('#status').html('Drawing');
    if (chart == null) {
	var options = {
            series: data.result.series,
            chart: {
		type: 'line',
		height: 600,
		zoom: {
		    type: 'x',
		    enabled: true,
		    autoScaleYaxis: true
		},
		toolbar: {
		    autoSelected: 'zoom'
		}
            },
            dataLabels: {
		enabled: false
            },
            markers: {
		size: 0,
            },
            title: {
		text: data.result.name,
		align: 'left'
            },
            yaxis: {
		labels: {
		    formatter: function (val) {
			return val.toFixed(3);
		    },
		},
		title: {
		    text: data.result.yaxis,
		},
            },
            xaxis: {
		type: 'datetime',
		tooltip: {
		    enabled: false,
		}
            },
            tooltip: {
		shared: false,
		y: {
		    formatter: function (val) {
			return val.toFixed(3)
		    },
		},
		x: {
		    format: "yyyy-M-d HH:mm",
		}
            }
	};
	chart = new ApexCharts(document.querySelector("#chart"), options);
    } else {
	chart.updateSeries(data.result.series);
    }
    chart.render().then(function() {
	let o = "DPs";
	for (const i of data.result.series) {
	    o = o + " " + i.num;
	}
	$('#status').html(o);
    });
 }

function draw_graph() {
    $('#status').html('Loading');
    // The next 2 lines are a workaround for update seemingly not
    // working (or me not understanding). :-(
    $('#chart').html('');
    chart = null;
    const m = $('#dates').val().match(RANGE);
    let finish = new Date() / 1000;
    let start = finish - 7 * 86400;
    if (m != null) {
	let d =  new Date(m[1]);
	start = d.getTime() / 1000;
	d = new Date(m[2]);
	finish = d.getTime() / 1000 + 86400;
    }
    $.getJSON($SCRIPT_ROOT + '/homer/get_data/' +
	$('#graph').val() + '/' +
        Math.floor(start) + '/' +
        Math.floor(finish) + '/' +
	$('#maxpoints').val(), render).fail(function() {
	    window.location.href = '/homer/graphs';
	});
}

$(window).on('load', function() {
    $('#dates').dateRangePicker();
    const yesterday = new Date(new Date().setDate(new Date().getDate()-1));
    $('#dates').val(formatDate(yesterday) + " to " +
		    formatDate(new Date()))
    $('#graphs').selectmenu();
    $(document).on('click', '#button', draw_graph);
});
