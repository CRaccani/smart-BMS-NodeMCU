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
    <script src="echarts.simple.min.js" integrity="sha256-CxlxgbUXeGRJaKmzCtFtuI75BynXJMwegbBzSX9ANjY=" crossorigin="anonymous"></script>
</head>
<body onresize="refresh()">
    <div class="header">
        <div class="logocontainer">
            <img class="logo" src="logo.gif" width="191" height="48" alt="smartBMS Monitor" />
            <div id="refreshbar"></div>
        </div>
        <div class="header-right">
            <a id="home" class="active" href="#home">Pack Info.</a>
            <a id="modules" href="#modules">Settings</a>
            <a id="integration" href="#integration">Calibration</a>
            <a id="settings" href="#settings">Other functions</a>
            <a id="about" href="#about">About</a>
        </div>
    </div>
    <div id="info" class="info">
        <div id="voltage" class="stat"><span class="x t">Voltage:</span><span id="totvoltage" class=" x v"></span></div>
        <div id="current" class="stat"><span class="x t">Current:</span><span id="currenta" class="x v"></span></div>
        <div id="rsoc" class="stat"><span class="x t">SOC:</span><span id="soc" class="x v"></span></div>
		<div id="avevoltage" class="stat"><span class="x t">Average:</span><span id="avevolt" class=" x v"></span></div>
        <div id="maximumcell" class="stat"><span class="x t">Max:</span><span id="maxcell" class="x v"></span></div>
        <div id="minimumcell" class="stat"><span class="x t">Min:</span><span id="mincell" class="x v"></span></div>
		<div id="celldifference" class="stat"><span class="x t">Diff.:</span><span id="celldiff" class="x v"></span></div>
   </div>

    <!-- prepare a DOM container with width and height -->
    <div id="graph1" style="width:100%;height:500px;"></div>
    
    <script type="text/javascript">
        var labels =[1,2,3,4,5,6,7,8,9,10,11,12,13,14];
        var voltages = [4.3,4.3,4.3,4.3,4.3,4.3,4.3,4.3,4.3,4.3,4.3,4.3,4.3,4.3];
        var voltagesmin = [5,5,5,5,5,5,5,5,5,5,5,5,5,5];
        var voltagesmax = [0,0,0,0,0,0,0,0,0,0,0,0,0,0];
        var tempint = [25, 25];
        var balanceBit = [0,0,0,0,0,0,0,0,0,0,0,0,0,0];
       
        var Socket;

		// when resized then reload page
		function refresh(){ location.reload();}

        function init()
        {
			//console.log('----- init()');
            Socket = new WebSocket('ws://'+window.location.hostname+':81/');
            Socket.onmessage = function(event)
            {
				var Elem = document.getElementById("refreshbar");
         		Elem.style.position = "relative";
         		Elem.animate({
              		left: ['-200px', '0px']
                 	},{ duration: 1000,        // number in ms [this would be equiv of your speed].
                     easing: 'ease-in-out',
                     iterations: 1,         // infinity or a number.
                });
			
                console.log('event.data: '+event.data);
                const str = event.data;
                const arr = str.split(",");
                if (arr[0]==='cell')
                {
                    voltages[arr[1]]= arr[2];
					voltagesmax[arr[1]]= arr[3];
					voltagesmin[arr[1]]= arr[4];
					balanceBit[arr[1]]= arr[5];
                }
                else if (arr[0]==='info')
                {
                    document.getElementById('totvoltage').innerHTML = arr[1]+'V';
                    document.getElementById('currenta').innerHTML = arr[2]+'A';
                    document.getElementById('soc').innerHTML = arr[3]+'%';
      	            document.getElementById('avevolt').innerHTML = arr[4]+'V';
                    document.getElementById('maxcell').innerHTML = arr[5]+'V';
                    document.getElementById('mincell').innerHTML = arr[6]+'V';
                    document.getElementById('celldiff').innerHTML = arr[7]+'mV';
                }
                
                myChart.setOption({
		            xAxis: { data: labels },
		            series: [{ name: 'Voltage', data: voltages }
		            ,{ name: 'Min V', data: voltagesmin }
		            ,{ name: 'Max V', data: voltagesmax }
					,{ name: 'CellTemperature', data: balanceBit }]
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
			// specify chart configuration item and data
			var option = {
				color: ['#003366', '#006699', '#4cabce'],
				tooltip: { trigger: 'axis', axisPointer: { type: 'cross', crossStyle: { color: '#999' } } },
				//legend: { data:['Voltage'] },
				xAxis: [
					{gridIndex: 0,type:'category'}
					,{show: false, gridIndex: 1,type:'category'}
					],
				yAxis: [
					{gridIndex: 0,name:'',type:'value',min:2.5,max:4.3,interval:0.2,position:'left', axisLabel: { formatter: '{value}V' }}
					,{show: false,gridIndex: 1,name:'',type:'value',min:-1,max:0,position:'right'} 
					],
				series: [
					{ name: 'Voltage', type: 'bar', data:[],label: labelOption}
					,{ name: 'Min V', type: 'line', data:[],label: labelOption4,symbolSize: 10, symbol:['circle'], itemStyle:{normal:{lineStyle:{color:'transparent',type:'dotted'}} } }
					,{ name: 'Max V', type: 'line', data:[],label: labelOption3,symbolSize: 10, symbol:['triangle'], itemStyle:{normal:{lineStyle:{color:'transparent',type:'dotted'}} } }
					,{xAxisIndex:1, yAxisIndex:1, name:'isBalance',type:'bar', data: [],
						barWidth:10,itemStyle:{barBorderRadius:5, color: '#ff0000'}}
					],
				grid: [
					{containLabel:false,
						left:'5%',
						right:'5%',
						top:'15%'},
					{containLabel:false,
						left:'5%',
						right:'5%',
						bottom:'85%'}
					
					]
			};
        // use configuration item and data specified to show chart
        myChart.setOption(option);
		//console.log("myChart:",myChart);
		init();
    </script>
</body>
</html>
)=====";
