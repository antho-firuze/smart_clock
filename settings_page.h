// --- TEKS HTML (Menggunakan PROGMEM agar hemat RAM) ---
const char SETTINGS_PAGE[] PROGMEM = R"=====(
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
  </body>
</html>
)=====";