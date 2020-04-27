const char FILE_INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />   
    <meta name="viewport" content="width=device-width,initial-scale=1.0">
    <link href="https://fonts.googleapis.com/css?family=Nova+Mono" rel="stylesheet">
    <link href="style.css" rel="stylesheet">
    <title>smartBMS</title>
    <!-- including ECharts file-->
    <!--
    <script src="jquery.js" integrity="sha256-CSXorXvZcTkaix6Yvo6HppcZGetbYMGWSFlBw8HfCJo=" crossorigin="anonymous"></script>
    <script src="echarts.simple.min.js" integrity="sha256-CxlxgbUXeGRJaKmzCtFtuI75BynXJMwegbBzSX9ANjY=" crossorigin="anonymous"></script>
     -->
    <script src="https://cdn.jsdelivr.net/npm/echarts@4.7.0/dist/echarts.common.min.js">
    </script>
</head>
<body>
    <div class="header">
        <div class="logocontainer">
            <img class="logo" src="logo.gif" width="191" height="48" alt="smartBMS Monitor" />
            <div id="refreshbar"></div>
        </div>
        <div class="header-right">
            <a id="home" class="active" href="#home">Home</a>
            <a id="modules" href="#modules">Modules</a>
            <a id="integration" href="#integration">Integration</a>
            <a id="settings" href="#settings">Settings</a>
            <a id="about" href="#about">About</a>
        </div>
    </div>
    <div id="info" class="info">
        <div id="voltage1" class="stat"><span class="x t">Voltage:</span><span id="totvoltage" class="x v"></span></div>
        <div id="current1" class="stat"><span class="x t">Current:</span><span id="currenta" class="x v"></span></div>
        <div id="range1" class="stat"><span class="x t">SOC:</span><span id="soc" class="x v"></span></div>
        </div>

    <!-- prepare a DOM container with width and height -->
    <div id="graph1" style="width:100%;height:768px;"></div>
    
    <script type="text/javascript">
        var labels =[1,2,3,4,5,6,7,8,9,10,11,12,13,14];
        var voltages = [4.12, 4.12, 4.12, 4.12, 4.12, 4.12, 4.12, 4.12, 4.12, 4.12, 4.12, 4.12, 4.12, 4.12];
        var voltagesmin = [3, 3.0,3, 3.0,3, 3.0,3, 3.0,3, 3.0,3, 3.0,3, 3.0];
        var voltagesmax = [4.3, 4.0,4.3, 4.0,4.3, 4.0,4.3, 4.0,4.3, 4.0,4.3, 4.0,4.3, 4.0];
        //var tempint = [25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25];
        //var tempext = [19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19];
       
        var Socket;
        function init()
        {
            Socket = new WebSocket('ws://'+window.location.hostname+':81/');
            Socket.onmessage = function(event)
            {
                console.log('event.data: '+event.data);
                const str = event.data;
                const arr = str.split(",");
                if (arr[0]==='cell')
                {
                    voltages[arr[1]]= arr[2];
                
                }
                else if (arr[0]==='info')
                {
                    document.getElementById('totvoltage').innerHTML = arr[1];
                    document.getElementById('currenta').innerHTML = arr[2];
                    document.getElementById('soc').innerHTML = arr[3];
                }
                
                myChart.setOption({
		            xAxis: { data: labels },
		            series: [{ name: 'Voltage', data: voltages }
		            ,{ name: 'Min V', data: voltagesmin }
		            ,{ name: 'Max V', data: voltagesmax }]
		        });
            }
        }
        // based on prepared DOM, initialize echarts instance
        var myChart = echarts.init(document.getElementById('graph1'));

        // specify chart configuration item and data
        var labelOption = {
				normal: {
					show: true,
					position: 'insideBottom',
					distance: 15,
					align: 'left',
					verticalAlign: 'middle',
					rotate: 90,
					formatter: '{c}V',
					fontSize: 20
				}
			};

			var labelOption3 = {
				normal: {
					show: true,
					position: 'top',
					distance:5,
					formatter: '{c}V',
					fontSize: 14
				}
			};

			var labelOption4 = {
				normal: {
					show: true,
					position: 'bottom',
					distance:5,
					formatter: '{c}V',
					fontSize: 14
				}
			};

			var labelOption2 = {
				  normal: {
					  show: true,
					  position: 'insideBottom',
					  distance: 15,
					  align: 'left',
					  verticalAlign: 'middle',
					  rotate: 90,
					  formatter: '{c}°C',
					  fontSize: 22
				  }
			  };
			// specify chart configuration item and data
			var option = {
				color: ['#003366', '#006699', '#4cabce'],
				tooltip: { trigger: 'axis', axisPointer: { type: 'cross', crossStyle: { color: '#999' } } },
				legend: { data:['Voltage'] },
				xAxis: [
					{gridIndex: 0,name:'Cells',type:'category' }
					],
				yAxis: [
					{gridIndex: 0,name:'Volts',type:'value',min:2.5,max:4.25,interval:0.25,position:'left', axisLabel: { formatter: '{value}V' }}
					],
				series: [
					{ name: 'Voltage', type: 'bar', data: [], label: labelOption}
					,{ name: 'Min V', type: 'line', data: [], label: labelOption4,symbolSize: 10, symbol:['circle'], itemStyle:{normal:{lineStyle:{color:'transparent',type:'dotted'}} } }
					,{ name: 'Max V', type: 'line', data: [], label: labelOption3,symbolSize: 10, symbol:['triangle'], itemStyle:{normal:{lineStyle:{color:'transparent',type:'dotted'}} } }
					
					],
				grid: [
					{containLabel:false,
					left:'15%',
					right:'15%',
					bottom:'5%',
                    top:'20%'}
					]
			};
        // use configuration item and data specified to show chart
        myChart.setOption(option);
		//console.log("g1:",myChart);
		init();
    </script>
</body>
</html>
)=====";
