void spiffSetup() { //fungsi untuk inisialisasi spiffs
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
}

String readFile(const char *path) { //fungsi untuk membaca file spiffs
  File file = SPIFFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading " + String(path));
    return String();
  }
  String fileContent;
  while (file.available()) {
    fileContent += (char)file.read();
  }
  return fileContent;
}

void writeFile(const char *path, const char *fileContent) { //fungsi untuk menulis file ke spiffs
  File file = SPIFFS.open(path, "w");
  if (file.print(fileContent)) {
    Serial.println("- file written " + String(path));
  } else {
    Serial.println("- write failed " + String(path));
  }
}

void readSettings() { //membaca konfigurasi alat
  mode = readFile("/mode.txt");
  if (mode == "") {
    dataDefault();
    mode = readFile("/mode.txt");
  }
}

void dataDefault() { //mengembalikan ke pengaturan awal, mode = dewasa
  writeFile("/mode.txt", "dewasa");
}
