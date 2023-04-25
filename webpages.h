const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
  <title>DAB Radio Server</title>
  <link rel="icon" href="data:,">
  <meta name=viewport content="width=device-width, initial-scale=1, user-scalable=yes, charset=utf-8">
  <link rel="stylesheet" type="text/css" href="/style.css">
  </head>
  <body>
    <div class="topnav">
      <h1>DAB Radio Server</h1>
    </div>
    <hr>
    <div class="divinfo">
    <table>     
      <tr>
        <td style="text-align:left;font-size: small; color: yellow;" colspan="3">
          <h4><span id="DABTime">%DABTime%</span> - <span id="DABFreq">%DABFreq%</span></h4>
        </td>      
      </tr> 
      <tr>
        <td style="text-align:center;font-size: medium; color: cyan;" colspan="3">
          <h1><span id="DABName">%DABName%</span></h1>
        </td>      
      </tr>      
      <tr>
        <td style="text-align:center;font-size: medium; color: #ff4500;" colspan="3">
          <h2><span id="DABRDS">%DABRDS%</span></h2> 
        </td>
      </tr>     
      <tr>
        <td style="text-align:right;">
          <a href="/tunedown"><button class="button">Tune down</button></a> 
        </td>
        <td style="text-align:left;">
          <a href="/tuneup"><button class="button">Tune up</button></a>
        </td>
        <td rowspan="4" width=150px>
           <span id="DABLogo">%DABLogo%</span>
        </td> 
      </tr>
      <tr>
        <td style="text-align:right;">
          <a href="/memdown"><button class="button">Memory down</button></a> 
        </td>
        <td style="text-align:left;">
          <a href="/memup"><button class="button">Memory up</button></a>
        </td>
      </tr>
      <tr>
        <td style="text-align:right;">
          <a href="/channeldown"><button class="button">Channel down</button></a> 
        </td>
        <td style="text-align:left;">
          <a href="/channelup"><button class="button">Channel up</button></a>
        </td>
      </tr>
      <tr>
        <td style="text-align:center;" colspan="2">
          <span id="stationList">%stationList%</span>
        </td>
      </tr>
      <tr>
        <td style="text-align:right;">
          <a href="/volumedown"><button class="button">Volume down</button></a> 
        </td>
        <td style="text-align:left;">
          <a href="/volumeup"><button class="button">Volume up</button></a>
        </td>
      </tr>
      <tr>
        <td style="text-align:right;">
          <a href="/mute"><button class="button"><span id="myMute">Mute %myMute%</span></button></a> 
        </td>
        <td style="text-align:left;">
          <a href="/mode"><button class="button"><span id="myMode">Mode %myMode%</span></button></a>
        </td>
      </tr>
    </table>
    </div>
    <hr>

    <div class="topnav" style="background-color: lightblue; ">
      <table>
          <tr>
            <td style="text-align:left; color:black;">
              <h4>PI4RAZ</h4>
            </td>
            <td style="text-align:right;">
              <a href="/settings"><button class="button">Settings</button></a>
              <a href="/reboot"><button class="button">Reboot</button></a>
            </td>
          </tr>
      </table>
    </div>

  <script>
  // alert("Oliebol");

    function changeActualChannel(){
      var e = document.getElementById("stationsList");
      window.location.href = "/gochannel?channel="+e.value.toString();
    }

  if (!!window.EventSource) {
      var source = new EventSource('/events');
    
    source.addEventListener('DABName', function(e) {
      document.getElementById("DABName").innerHTML = e.data;
    }, false);

    source.addEventListener('DABRDS', function(e) {
      document.getElementById("DABRDS").innerHTML = e.data;
    }, false);

    source.addEventListener('DABLogo', function(e) {
      document.getElementById("DABLogo").innerHTML = e.data;
    }, false);

    source.addEventListener('DABTime', function(e) {
      document.getElementById("DABTime").innerHTML = e.data;
    }, false);

    source.addEventListener('DABFreq', function(e) {
      document.getElementById("DABFreq").innerHTML = e.data;
    }, false);

    source.addEventListener('selectedStation', function(e) {
      let element = document.getElementById("stationsList");
      element.value = e.data;
    }, false);

    source.addEventListener('myMute', function(e) {
      document.getElementById("myMute").innerHTML = "Mute " + e.data;
    }, false);

    source.addEventListener('myMode', function(e) {
      document.getElementById("myMode").innerHTML = "Mode " + e.data;
    }, false);
  }

  if(typeof window.history.pushState == 'function') {
    if (window.location.href.lastIndexOf('/command')>10 || window.location.href.lastIndexOf('/reboot')>10){
      console.log(window.location.href);
      window.location.href =  window.location.href.substring(0,window.location.href.lastIndexOf('/'))
    }
  }
  </script>
  </body>
  </html>
)rawliteral";

const char settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
  <title>DAB Radio Server</title>
  <link rel="icon" href="data:,">
  <meta name=viewport content="width=device-width, initial-scale=1, user-scalable=yes, charset=utf-8">
  <link rel="stylesheet" type="text/css" href="/style.css">
  </head>
  <body>
    <div class="topnav">
      <h1>DAB Radio Server</h1>
      <br>
      <h2>Settings</h2>
    </div>
    <hr>
    <div class="divinfo">
      <form action="/store" method="get">

      <div class="divinfo">
        <table>
          <tr>
            <td style="text-align:right;font-size: medium; color: white">
              WiFi SSID: 
            </td>
            <td style="text-align:left;font-size: medium;">
              <input type="text" name="wifiSSID" value="%wifiSSID%">
            </td>
          </tr>

          <tr>
            <td style="text-align:right;font-size: medium; color: white">
              WiFi Password:
            </td>
            <td style="text-align:left;font-size: medium;">
              <input type="password" name="wifiPass" value="%wifiPass%">
            </td>
          </tr>

          <tr>
            <td style="text-align:right;font-size: medium;">
              Debugmode:
            </td>
            <td style="text-align:left;font-size: medium;">
              <input type="checkbox" name="isDebug" value="isDebug" %isDebug%>
            </td>
          </tr>

          <tr>
            <td></td>
            <td style="text-align:left;font-size: medium;">
              <input style="font-size: medium" type="submit" value="Submit">
            </td>
          </tr>
        </table>
      </div>
      
    </form><br>
  </div>
  <hr>

  <div class="topnav" style="background-color: lightblue; ">
    <table>
        <tr>
          <td style="text-align:left; color:black">
            <h4>PI4RAZ</h4>
          </td>
          <td style="text-align:right">
            <a href="/"><button class="button">Main</button></a>
            <a href="/reboot"><button class="button">Reboot</button></a>
          </td>
        </tr>
    </table>
  </div>

  <script>
  if(typeof window.history.pushState == 'function') {
    if (window.location.href.lastIndexOf('/command')>10 || window.location.href.lastIndexOf('/reboot')>10 || window.location.href.lastIndexOf('/store')>10){
      console.log(window.location.href);
      window.location.href =  window.location.href.substring(0,window.location.href.lastIndexOf('/'))
    }
  }
  </script>

  </body>
  </html>
)rawliteral";

const char css_html[] PROGMEM = R"rawliteral(
html {
    font-family: Arial; 
    display: inline-block; 
    text-align: center;
    background-color: gray; 
}
p { 
    font-size: 1.2rem;
}
body {  
    max-width:900px;
    margin: 0 auto;
    background-color: gray;
}
.topnav { 
    overflow: hidden; 
    background-color: blue; 
    color: white; 
    font-size: 1rem; 
    line-height: 0%;
    text-align: center;
    word-wrap: break-word;
}
.divinfo {  
    overflow: hidden; 
    background-color: gray; 
    color: white; 
    font-size: 0.7rem; 
    height:100;
    line-height: 0%;
}
.content { 
    padding: 20px; 
}
.button {
    height:30px;
    width:110px;
    background-color: white;
    border-radius: 6px;
    border: 1px solid black;
}
select {
    height:30px;
    width:226px;
    background-color: white;
    padding: 0px;
}
table {
  width: 100%;
}
// table, td, tr {
//   border: 1px solid;
// }
})rawliteral";

