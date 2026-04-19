# ESP32 webserver and frontend integration

Here's how it works:

1. **The ESP32 code uses SPIFFS** to serve the frontend. You can see this on line 6 (`#include "SPIFFS.h"`) and lines 453-458 where it mounts the SPIFFS filesystem and serves static files from it:

```453:458:ESP32-Control/src/main.cpp
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
```

2. **SPIFFS is a separate flash partition**, not part of the firmware binary. For PlatformIO to upload files to SPIFFS, they need to be placed in an `ESP32-Control/data/` directory. That `data/` folder currently **does not exist** in the repo.

3. **The upload is a two-step process**, as noted in the comments in `platformio.ini`:

```11:13:ESP32-Control/platformio.ini
; pio.exe run --target uploadfs
; pio.exe run --target upload && pio device monitor
```

- `pio run --target upload` flashes the firmware (the compiled C++ code).
- `pio run --target uploadfs` uploads the SPIFFS filesystem image (the frontend files) to the separate flash partition.

4. **There is a missing link in the pipeline.** The UI build (`npm run build` in `UI/`) outputs to `UI/dist/`, and `postbuild.js` patches the asset paths to be relative. However, there is no automation that copies the built files from `UI/dist/` into `ESP32-Control/data/`. You'd need to manually copy the contents of `UI/dist/` into `ESP32-Control/data/` before running `pio run --target uploadfs`.

**Summary of the full workflow that would be needed:**

1. `cd UI && npm run build` — builds the frontend into `UI/dist/`
2. Copy `UI/dist/*` into `ESP32-Control/data/`
3. `cd ESP32-Control && pio run --target uploadfs` — uploads frontend to SPIFFS partition
4. `pio run --target upload` — flashes the firmware

So to directly answer your question: when you flash the ESP32 with `pio run --target upload`, only the firmware is flashed. The frontend must be uploaded separately to the SPIFFS partition, and you first need to ensure the built frontend files are in the `ESP32-Control/data/` directory.

Run this command to upload frontend to ESP32:

`& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t uploadfs`
`& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t erase`
