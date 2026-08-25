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
    </style>
  </head>
  <body>
    <div class="card">
      <h1>Smart Clock</h1>
      <div class="version" id="version">{version}</div>
      <hr />
      <div>
        Device Location:
        <div class="value" id="device_location">{device_location}</div>
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
      <h3>Pengaturan Variabel</h3>
      <form action="/set_brightness" method="GET">
        <label>Brightness Level:</label>
        <input
          type="range"
          name="brightness"
          min="0"
          max="15"
          value="{brightness_input}"
        />
        <button type="submit">Simpan</button>
      </form>
      <form action="/set_time_format" method="GET">
        <label>Time Format:</label>
        <select name="time_format">
          <option value="0">24 hour</option>
          <option value="1">12 hour (AM/PM)</option>
        </select>
        <button type="submit">Simpan</button>
      </form>
      <form action="/set_device_location" method="GET">
        <label>Ubah Lokasi Perangkat:</label>
        <input
          type="text"
          name="device_location"
          id="device_location"
          required
        />
        <button type="submit">Simpan</button>
      </form>
    </div>

    <div class="card">
      <h3>Systems</h3>
      <div>
        IP Address:
        <div class="value" id="ip_address">{ip_address}</div>
      </div>
      <div>
        DNS Name:
        <div class="value" id="dns_name">{dns_name}</div>
      </div>
      <button class="btn-reset" onclick="resetWifi()">
        Reset Koneksi WiFi
      </button>
      <br />
      <button class="btn-reset" onclick="location.href = '/server-ota'">
        Update Firmware
      </button>
    </div>

    <script>
    //   setInterval(function () {
    //     fetch("/data")
    //       .then((response) => response.json())
    //       .then((data) => {
    //         // document.getElementById("device_location").innerText = data.device_location;
    //         // document.getElementById("version").innerText = "v" + data.version;
    //         // document.getElementById("brightness").innerText = data.brightness + "/15";
    //         // document.getElementById("time_format").innerText = data.time_format == "1" ? "12 hour (AM/PM)" : "24 hour";
    //         // document.getElementById("temperature").innerText = data.temperature;
    //         // document.getElementById("humidity").innerText = data.humidity;
    //         // document.getElementById("offset_temp").innerText = data.offset_temp;
    //         // document.getElementById("offset_hum").innerText = data.offset_hum;
    //         // document.getElementById("min_hum_start").innerText = data.min_hum_start;
    //         // document.getElementById("max_hum_stop").innerText = data.max_hum_stop;
    //       });
    //   }, 2000);

      // function to reset wifi connection
      function resetWifi() {
        if (confirm("Apakah Anda yakin ingin mereset koneksi WiFi?")) {
          fetch("/reset_wifi")
            .then((response) => {
              if (response.ok) {
                alert("Koneksi WiFi telah direset.");
              } else {
                alert("Gagal mereset koneksi WiFi.");
              }
            })
            .catch((error) => {
              console.error("Error:", error);
              alert("Terjadi kesalahan saat mereset koneksi WiFi.");
            });
        }
      }
    </script>
  </body>
</html>
)=====";