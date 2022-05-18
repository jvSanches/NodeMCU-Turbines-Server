
// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266LLMNR.h>

#include<SPI.h>




// Replace with your network credentials
#define ssid "JASSY.VISITORS"
#define password "jassy@apollo"

#define PARAM_INPUT_1 "id"
#define PARAM_INPUT_2 "state"


AsyncWebServer server(80);

byte inverters_on[10] = {0,0,0,0,0,0,0,0,0,0};
float inverters_freq[10] = {3,3,3,3,3,3,3,3,3,3};

void setFreq(int id, float nfreq){
    nfreq = int(nfreq*10)/10.0;
    inverters_freq[id-1] = nfreq;
    digitalWrite(D0, LOW);
    delay(10);
    SPI.transfer('f');
    delay(10);
    SPI.transfer(id);
    delay(10);
    SPI.transfer(int(nfreq*10)/256);
    delay(10);
    SPI.transfer(int(nfreq*10)%256);
    delay(10);
    SPI.transfer('\n');
    delay(10);
    digitalWrite(D0, HIGH);
}
float getFreq(int id){
    return(inverters_freq[id-1]);
}

void setState(int id, int nState){
    inverters_on[id-1] = nState;
    digitalWrite(D0, LOW);
    delay(10);
    SPI.transfer('s');
    delay(10);
    SPI.transfer(id);
    delay(10);
    SPI.transfer(nState);
    delay(10);
    SPI.transfer('\n');
    delay(10);
    digitalWrite(D0, HIGH);
}
byte getState(int id){
    return inverters_on[id-1];
}

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE HTML><html><head>
    <title>ESP Input Form</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
    html {
    font-family: Helvetica;
    display: inline-block;
    margin: 0px auto;
    text-align: center;
    }
    .button {
    background-color: #4c51af;
    border: none;
    color: white;
    padding: 16px 40px;
    text-decoration: none;
    font-size: 30px;
    margin: 2px;
    cursor: pointer;
    }
    .button2 {
    background-color: #555555;
    }
    </style>
    </head><body>
    <h1>Sala de Turbinas</h1>
    <a href="http://%IP%">%IP%</a>
    <h2>Selecionar inversor</h2>
    <p><a href="/1/"><button class="button">1</button></a>
    <p><a href="/2/"><button class="button">2</button></a>
    <p><a href="/3/"><button class="button">3</button></a>
    <p><a href="/4/"><button class="button">4</button></a>

    </form><br>
    </form>
    </body></html>)rawliteral";

const char page_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE HTML><html><head>
    <title>ESP Input Form</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
    html {
    font-family: Helvetica;
    
    display: inline-block;
    margin: 0px auto;
    text-align: center;
    }
    .button {
    background-color: #4c51af;
    border: none;
    color: white;
    padding: 10px 20px;
    text-decoration: none;
    font-size: 20px;
    margin: 2px;
    cursor: pointer;
    }
    .button2 {
    background-color: #555555;
    }
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #4CAF50}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    </style>
    </head><body>
    <h1>Sala de Turbinas</h1>
    <p><a href="/"><button class="button">Inicio</button></a></p>
    <br>
    <br>
    <br>
    <h2>Inversor %BANCADA%
    <script>function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();
    if(element.checked){ xhr.open("GET", "/update?id="+element.id+"&state=1", true); }
    else { xhr.open("GET", "/update?id="+element.id+"&state=0", true); }
    xhr.send();
    setTimeout(window.location.reload.bind(window.location), 3000);
    }
    </script>
    <script> function sendValue(element) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/update?id="+element.id+"&state="+element.value, true);
    xhr.send();
    }
    </script>
    

    </form><br>
    </form>
    </body></html>)rawliteral";

void build_page(int id){
    return;
}
int active_id = 0;
String switchState(int id){
  if(getState(id)){
    return "checked";
  }
  else {
    return "";
  }
}
String processor(const String& var)
{
 
  if(var == "BANCADA"){
    String interface = String(active_id);  
    interface += "</h2>";
    interface += "<label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\" "+ String(active_id) +"  \" " + switchState(active_id) + " ><span class=\"slider\"></span></label> <p>Frequencia (Hz):</p>";
    interface += "<input style=\"width:180px;font-size:60px;text-align:center;\" type=\"number\" step=\"0.1\" name=\"freq\" value=\""+ String(getFreq(active_id),1) +"\" onchange=\"sendValue(this)\" id=\" "+ String(active_id+10) +"  \" " + switchState(active_id) + ">";
    
    return interface;
  }else if(var == "IP"){
    String ipnum = WiFi.localIP().toString().c_str();
    return ipnum;
  }
 
  return String();
}

void notFound(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", index_html);
}

void setTurbines(int param1, float param2){
  if ((param1 > 0)&&(param1<=10)){
    if ((param2 == 0)||(param2 ==1)){
      setState(param1, int(param2));
      }
  }else if ((param1 > 10)&&(param1<=20)){
    if ((param2 > 0)&&(param2 <100)){
      setFreq(param1-10, param2);
      }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(D0, OUTPUT);
  digitalWrite(D0, HIGH);
  SPI.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  LLMNR.begin("turbinas");

  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i<10; i++){
    digitalWrite(LED_BUILTIN, !(digitalRead(LED_BUILTIN)));
    delay(50);
  }

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html,processor);
  });
  server.on("/1", HTTP_GET, [](AsyncWebServerRequest *request){
    active_id = 1;
    request->send_P(200, "text/html", page_html, processor);
  });
    server.on("/2", HTTP_GET, [](AsyncWebServerRequest *request){
    active_id = 2;
    request->send_P(200, "text/html", page_html, processor);
  });
    server.on("/3", HTTP_GET, [](AsyncWebServerRequest *request){
    active_id = 3;
    request->send_P(200, "text/html", page_html, processor);
  });
    server.on("/4", HTTP_GET, [](AsyncWebServerRequest *request){
    active_id = 4;
    request->send_P(200, "text/html", page_html, processor);
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      setTurbines(inputMessage1.toInt(), inputMessage2.toFloat());
    }
    
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  //   server.on("/2", HTTP_GET, [](AsyncWebServerRequest *request){
  //   active_id = 2;
  //   request->send_P(200, "text/html", page_html, processor);
  // });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
    }    
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  
  if (WiFi.status() == WL_CONNECTED){
    digitalWrite(LED_BUILTIN, LOW);
  }else{
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
