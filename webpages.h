
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
  <title>DAB Radio Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- <meta http-equiv="refresh" content="1">  -->
  <link rel="icon" href="data:,">
  <link rel="stylesheet" type="text/css" href="style.css">
  </head>
  <body>
    <div class="topnav">
      <h1>DAB Radio Server</h1>
    </div>
    <hr>
    <div class="divinfo">
      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:center;font-size: medium; color: cyan">
              <h1><span id="DABName">%DABName%</span></h1> 
            </td>
          </tr>
          <tr>
            <td class="hwidth" style="text-align:center;font-size: medium; color: green">
              <h2><span id="DABRDS">%DABRDS%</span></h2> 
            </td>
          </tr>
        </table>
      </div>
  </div>
  <hr>

  <div class="topnav" style="background-color: lightblue; ">
    <table class="fwidth">
        <tr>
          <td class="hwidth" style="text-align:left">
            <h4>Copyright (c) Robert de Kok, PA2RDK</h4>
          </td>
          <td style="text-align:right">
            <a href="/settings"><button>Settings</button></a>
            <a href="/reboot"><button>Reboot</button></a>
          </td>
        </tr>
    </table>
  </div>

  <script>
  if (!!window.EventSource) {
      var source = new EventSource('/events');
    
    source.addEventListener('DABName', function(e) {
      document.getElementById("DABName").innerHTML = e.data;
    }, false);

    source.addEventListener('DABRDS', function(e) {
      document.getElementById("DABRDS").innerHTML = e.data;
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
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- <meta http-equiv="refresh" content="1">  -->
  <link rel="icon" href="data:,">
  <link rel="stylesheet" type="text/css" href="style.css">
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
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium; color: white">
              WiFi SSID: 
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="wifiSSID" value="%wifiSSID%">
            </td>
          </tr>

          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium; color: white">
              WiFi Password:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="password" name="wifiPass" value="%wifiPass%">
            </td>
          </tr>

          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Debugmode:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="checkbox" name="isDebug" value="isDebug" %isDebug%>
            </td>
          </tr>

          <tr>
            <td></td>
            <td class="fwidth" style="text-align:left;font-size: medium;">
              <input style="font-size: medium" type="submit" value="Submit">
            </td>
          </tr>
        </table>
      </div>
      
    </form><br>
  </div>
  <hr>

  <div class="topnav" style="background-color: lightblue; ">
    <table class="fwidth">
        <tr>
          <td class="hwidth" style="text-align:center">
            <h4>Copyright (c) Robert de Kok, PA2RDK</h4>
          </td>
          <td style="text-align:right">
            <a href="/"><button>Main</button></a>
            <a href="/reboot"><button>Reboot</button></a>
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

const char style_html[] PROGMEM = R"rawliteral(
<style>
html {
    font-family: Arial; 
    display: inline-block; 
    text-align: center;
    background-color: black; 
}
p { 
    font-size: 1.2rem;
}
body {  
    margin: 0;
    background-color: black; 
}
.hwidth {
    width: 50%;
}

.fwidth {
    width: 100%;
}

.freqdisp {
    border: none;
    background-color: black;
    color: yellow;
    font-size: xx-large;
    border-bottom: 1px solid gray;
    text-align: center;
}

.topnav { 
    overflow: hidden; 
    background-color: blue; 
    color: white; 
    font-size: 1rem; 
    line-height: 0%;
    text-align: center;
}
.divinfo { 	
    overflow: hidden; 
    background-color: black; 
    color: white; 
    font-size: 0.7rem; 
    height:100;
    line-height: 0%;
}
.freqinfo { 	
    overflow: hidden; 
    background-color: black;
    color: yellow; 
    font-size: 1rem; 
    line-height: 0%;
}
.content { 
    padding: 20px; 
}
.card { 
    background-color: lightblue; 
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); 
    font-size: 0.7rem; 
}
.cards { 
    max-width: 1000px; 
    margin: 0 auto; 
    display: grid; 
    grid-gap: 1rem; 
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); 
}
.reading { 
    font-size: 1.4rem;  
}
</style>
)rawliteral";