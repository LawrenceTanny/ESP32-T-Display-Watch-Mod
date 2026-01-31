#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

const char* ssid = "ESP32-Transfer";
const char* password = "password";

WebServer server(80);

// NEW: JavaScript for Batch Uploading
const char* upload_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<style>
  body { font-family: sans-serif; background: #121212; color: #fff; text-align: center; padding: 20px; }
  #drop-zone { border: 2px dashed #00e5ff; padding: 50px; margin: 20px auto; width: 80%; border-radius: 10px; }
  button { background: #00e5ff; border: none; padding: 10px 20px; font-size: 16px; border-radius: 5px; cursor: pointer; }
  #status { margin-top: 20px; color: #00e5ff; }
</style>
<script>
function uploadFiles() {
  var input = document.getElementById('file-input');
  var files = input.files;
  var status = document.getElementById('status');
  
  if(files.length === 0){ alert("No files selected!"); return; }

  let i = 0;
  function uploadNext() {
    if (i >= files.length) {
      status.innerHTML = "All Uploads Complete!";
      return;
    }
    
    var formData = new FormData();
    formData.append("upload", files[i]);
    status.innerHTML = "Uploading: " + files[i].name + " (" + (i+1) + "/" + files.length + ")...";

    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/upload", true);
    xhr.onload = function() {
      if (xhr.status === 200) { i++; uploadNext(); }
      else { status.innerHTML = "Error uploading " + files[i].name; }
    };
    xhr.send(formData);
  }
  uploadNext();
}
</script>
</head>
<body>
  <h1>Tanny Watch Storage</h1>
  <div id="drop-zone">
    <input type="file" id="file-input" name="upload" multiple style="display:none" onchange="uploadFiles()">
    <button onclick="document.getElementById('file-input').click()">Select Multiple Files</button>
  </div>
  <div id="status">Ready to upload</div>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", upload_html);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    if (SPIFFS.exists(filename)) SPIFFS.remove(filename);
    File file = SPIFFS.open(filename, FILE_WRITE);
    file.close();
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    String filename = "/" + upload.filename;
    File file = SPIFFS.open(filename, FILE_APPEND);
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    server.send(200, "text/plain", "OK");
  }
}

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) return;
  
  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/upload", HTTP_POST, [](){ server.send(200); }, handleUpload);
  server.begin();
}

void loop() {
  server.handleClient();
}