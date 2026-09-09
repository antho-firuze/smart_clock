// --- TEKS HTML (Menggunakan PROGMEM agar hemat RAM) ---
const char INDEX_PAGE[] PROGMEM = R"=====(
<!doctype html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Smart Clock</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        text-align: center;
        background: #f4f4f4;
        margin: 0;
        padding: 20px;
      }
      .card {
        background: white;
        padding: 20px;
        border-radius: 10px;
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
        max-width: 400px;
        margin: 20px auto;
      }
      h1 {
        color: #333;
        margin-block-end: 0em;
      }
      .version {
        font-size: 20px;
        font-weight: bold;
        color: #007bff;
        margin-block-end: 0.67em;
      }
      .value {
        font-size: 24px;
        font-weight: bold;
        color: #007bff;
        margin: 10px 0;
      }
      form {
        margin: 20px 0;
      }
      select,
      input[type="number"],
      input[type="text"],
      input[type="password"],
      input[type="time"],
      input[type="range"],
      input[type="file"] {
        width: 85%;
        padding: 10px;
        font-size: 16px;
        margin: 10px 0;
        border: 1px solid #ccc;
        border-radius: 5px;
      }
      button {
        background: #28a745;
        color: white;
        border: none;
        padding: 10px 20px;
        font-size: 16px;
        border-radius: 5px;
        cursor: pointer;
      }
      button:hover {
        background: #218838;
      }
      .btn-reset {
        background: #dc3545;
        margin-top: 15px;
      }
      .btn-reset:hover {
        background: #c82333;
      }
      /* The container wrapper around the switch */
      .switch {
        position: relative;
        display: inline-block;
        width: 60px;
        height: 34px;
      }

      /* Hide the default HTML checkbox */
      .switch input {
        opacity: 0;
        width: 0;
        height: 0;
      }

      /* The visual track (slider background) */
      .slider {
        position: absolute;
        cursor: pointer;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: #ccc;
        border-radius: 34px;
        transition: 0.4s;
      }

      /* The circular knob inside the switch */
      .slider::before {
        position: absolute;
        content: "";
        height: 26px;
        width: 26px;
        left: 4px;
        bottom: 4px;
        background-color: white;
        border-radius: 50%;
        transition: 0.4s;
      }

      /* Track changes to "Checked" state: change background color */
      .switch input:checked + .slider {
        background-color: #4CAF50; /* Green when turned on */
      }

      /* Track changes to "Checked" state: slide the knob to the right */
      .switch input:checked + .slider::before {
        transform: translateX(26px);
      }
    </style>
  </head>
  <body>
    <div class="card">
      <h1>Smart Clock</h1>
      <div class="version" id="version">{version}</div>
      <hr />
      <div>
        Brightness Mode:
        <div class="value" id="brightness_mode">{brightness_mode}</div>
      </div>
      <div>
        Brightness Level:
        <div class="value" id="brightness">{brightness}</div>
      </div>
      <div>
        Time Format:
        <div class="value" id="time_format">{time_format}</div>
      </div>
    </div>

    <div class="card">
      <h3>Variable Settings</h3>
      <form action="/set_device_location" method="GET">
        <label>Device Location:</label>
        <input
          type="text"
          name="device_location"
          id="device_location"
          value="{device_location}"
          required
        />
        <button type="submit">Simpan</button>
      </form>
      <form action="/set_display_screen" method="GET">
        <label>Number of Screens:</label>
        <select name="display_screen">
          <option value="4" {display_screen_4}>4 Screens</option>
          <option value="8" {display_screen_8}>8 Screens</option>
        </select>
        <button type="submit">Simpan</button>
      </form>
      <form action="/set_brightness_mode" method="GET">
        <label>Brightness Mode:</label>
        <select name="brightness_mode">
          <option value="0" {brightness_manual}>Manual</option>
          <option value="1" {brightness_auto}>Auto</option>
        </select>
        <button type="submit">Simpan</button>
      </form>
      <form action="/set_brightness" method="GET">
        <label>Brightness Level: <output id="brightness_level">{brightness_value}</output></label>
        <input type="range" name="brightness" min="{min_brightness}" max="{max_brightness}" value="{brightness_value}" {brightness_level_state} oninput="brightness_level.value = this.value" />
        <button type="submit" {brightness_level_state}>Simpan</button>
      </form>
      <form action="/set_min_brightness_limit" method="GET">
        <label>Min. Brightness Limit: <output id="min_limit">{min_brightness_limit}</output></label>
        <input type="range" name="min_brightness_limit" min="{min_brightness}" max="{max_brightness}" value="{min_brightness_limit}" {brightness_limit_state} oninput="min_limit.value = this.value" />
        <button type="submit" {brightness_limit_state}>Simpan</button>
      </form>
      <form action="/set_max_brightness_limit" method="GET">
        <label>Max. Brightness Limit: <output id="max_limit">{max_brightness_limit}</output></label>
        <input type="range" name="max_brightness_limit" min="{min_brightness}" max="{max_brightness}" value="{max_brightness_limit}" {brightness_limit_state} oninput="max_limit.value = this.value" />
        <button type="submit" {brightness_limit_state}>Simpan</button>
      </form>
      <form action="/set_time_format" method="GET">
        <label>Time Format:</label>
        <select name="time_format">
          <option value="0" {time_format_24}>24 hour</option>
          <option value="1" {time_format_12}>12 hour (AM/PM)</option>
        </select>
        <button type="submit">Simpan</button>
      </form>
      <form action="/set_custom_text" method="GET">
        <label>Custom Text:</label>
        <input
          type="text"
          name="custom_text0"
          id="custom_text0"
          value="{custom_text0}"
        />
        <input
          type="text"
          name="custom_text1"
          id="custom_text1"
          value="{custom_text1}"
        />
        <input
          type="text"
          name="custom_text2"
          id="custom_text2"
          value="{custom_text2}"
        />
        <button type="submit">Simpan</button>
      </form>
    </div>

        <div class="card">
      <h3>WiFi Manager</h3>
      <div>
        Status:
        <div class="value" id="wifi_status">{wifi_status}</div>
      </div>
      <div>
        IP Address:
        <div class="value" id="local_ip_address">{local_ip_address}</div>
      </div>
      <form action="/save_wifi" method="POST">
        <label>Home Wi-Fi SSID:</label>
        <input type="text" name="ssid" placeholder="Enter Wi-Fi Name" value="{ssid_value}" required />
      </br>
        <label>Wi-Fi Password:</label>
        <input type="password" name="password" placeholder="Enter Password" value="{ssid_password}" />
      </br>
        <label>Auto Connect:</label>
        </br>
        <label class="switch">
          <input type="checkbox" name="auto_connect" {auto_connect} />
          <span class="slider"></span>
        </label>
      </br></br>
        <button type="submit">Save & Connect</button>
      </form>
    </div>

    <div class="card">
      <h3>Systems</h3>
      <div>
        SSID:
        <div class="value" id="SSID">{ssid}</div>
      </div>
      <div>
        IP Address:
        <div class="value" id="ip_address">{ip_address}</div>
      </div>
      <button class="btn-reset" onclick="restartDevice()">
        Restart Device
      </button>
      <br />
      <button class="btn-reset" onclick="location.href = '/server-ota'">
        Update Firmware
      </button>
    </div>

    <script>
       setInterval(function () {
         fetch("/data")
           .then((response) => response.json())
           .then((data) => {
            document.getElementById("wifi_status").innerText = data.wifi_status;
            document.getElementById("local_ip_address").innerText = data.local_ip_address;
           });
       }, 2000);

      // function to restart device
      function restartDevice() {
        if (confirm("Apakah Anda yakin ingin me-restart perangkat?")) {
          fetch("/restart_device")
            .then((response) => {
              if (response.ok) {
                alert("Perangkat telah di restart.");
              } else {
                alert("Gagal me-restart perangkat.");
              }
            })
            .catch((error) => {
              console.error("Error:", error);
              alert("Terjadi kesalahan saat me-restart perangkat.");
            });
        }
      }
    </script>
  </body>
</html>
)=====";