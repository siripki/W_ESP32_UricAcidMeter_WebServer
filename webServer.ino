void serverSetup() {
  //mengirimkan file webesrver ke browser
  server.serveStatic("/assets/", SPIFFS, "/assets/");

  //apabila alamat yang diketikan salah
  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(200, "text/html", "Destination NOT FOUND !");
  });

  //Untuk Autentikasi login pada brwser dengan menggunakan "admin" "admin"
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->authenticate("admin", "admin")) {
      request->send(SPIFFS, "/testweb6.html", String(), false, processor); //mengirimkan file testweb6.html dari esp ke browser
    }
    else {
      return request->requestAuthentication();
    }
  });

  //akan handle jika mode diganti
  server.on("/execute", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->authenticate("admin", "admin")) {
      if (request->hasParam("mode", true)) {
        mode = request->getParam("mode", true)->value(); //memperbarui mode
        writeFile("/mode.txt", mode.c_str()); //menulis mode yang sekarang ke spiffs
      }

      request->redirect("/");
      Serial.println("Execute OK!");
    }

    else {  //jika autentikasi yang diberikan salah, bukan "admin" "admin"
      delay(50);
      request->send(200, "text/plain", "You don't have permission!");
      Serial.println("Execute ERROR !");
    }
  });

  // Handle Web Server Events, untuk memperbarui nilai nilai pada webserver
  events.onConnect([](AsyncEventSourceClient* client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  AsyncElegantOTA.begin(&server);
  server.begin(); //memulai webserver
}

void serverSentEvents() { //mengirimkan data asamurat dan saran terbaru
  events.send(String(asamUrat).c_str(), "asamUrat", millis());
  events.send(saran.c_str(), "saran", millis());
}

String processor(const String& var) { //processor, untuk menangani Get Method dari web server
  if (var == "asamUrat") {  //memberikan nilai asam urat
    return String(asamUrat,1);
  }

  else if (var == "mode") {  //memberikan mode
    return mode;
  }

  else if (var == "saran") {  //memberikan saran
    return saran;
  }

  else if (var == "puasaAnak") {
    return puasaAnak;
  }

  else if (var == "puasaDewasa") {
    return puasaDewasa;
  }

  else if (var == "puasaLansia") {
    return puasaLansia;
  }

  else if (var == "tidakPuasaAnak") {
    return tidakPuasaAnak;
  }

  else if (var == "tidakPuasaDewasa") {
    return tidakPuasaDewasa;
  }

  else if (var == "tidakPuasaLansia") {
    return tidakPuasaLansia;
  }

  else {
    return String();
  }
}