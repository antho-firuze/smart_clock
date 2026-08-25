// --- TEKS HTML (Menggunakan PROGMEM agar hemat RAM) ---
const char OTA_PAGE[] PROGMEM = R"=====(
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
      <form method="POST" action="/update" enctype="multipart/form-data">
        <h3>Firmware Upload</h3>
        <input type="file" name="update" /><br />
        <button type="submit" class="btn-reset" value="Update">Update</button>
      </form>
    </div>
  </body>
</html>
)=====";