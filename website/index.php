<?php
/**
 * Busy Signal - control panel (behind the PIN gate).
 */

ini_set('session.cookie_httponly', 1);
ini_set('session.use_only_cookies', 1);
ini_set('session.cookie_samesite', 'Strict');
session_name('BUSYSIGNAL_SESSION');
session_start();

if (!isset($_SESSION['busy_signal_authenticated']) || $_SESSION['busy_signal_authenticated'] !== true) {
    header('Location: auth.php');
    exit;
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta name="theme-color" content="#0a0c10">
  <meta name="robots" content="noindex">
  <title>Busy Signal</title>
  <link rel="stylesheet" href="style.css">
</head>
<body>
  <div class="page">
    <header>
      <div class="sign-mini"><div></div><div></div></div>
      <div class="header-text">
        <h1>Busy Signal</h1>
        <p class="sub">LED status board &middot; office door</p>
      </div>
      <a class="logout" href="logout.php">Log out</a>
    </header>

    <div class="group">
      <h2>Status</h2>
      <div class="status-grid">
        <div class="status-col">
          <span class="col-label"><span class="dot busy"></span>Busy</span>
          <button class="status-btn busy" data-status="meeting">In a Meeting</button>
          <button class="status-btn busy" data-status="call">On a Call</button>
          <button class="status-btn busy" data-status="racing">Racing</button>
          <button class="status-btn busy" data-status="recording">Recording</button>
        </div>
        <div class="status-col">
          <span class="col-label"><span class="dot available"></span>Available</span>
          <button class="status-btn available" data-status="working">Working, Available</button>
          <button class="status-btn available" data-status="comein">Come on In</button>
        </div>
      </div>
    </div>

    <div class="group">
      <h2>Message</h2>
      <input type="text" id="messageInput" placeholder="Back in 10, grab a coffee..." maxlength="80">
      <div class="row">
        <button class="primary" id="sendMessageBtn">Send to sign</button>
        <button class="ghost" id="clearMessageBtn">Clear</button>
      </div>
    </div>

    <div class="group">
      <h2>Timer</h2>
      <span class="field-label">Duration</span>
      <div class="chip-row" id="chipRow">
        <button class="chip" data-min="5">5m</button>
        <button class="chip" data-min="10">10m</button>
        <button class="chip is-active" data-min="15">15m</button>
        <button class="chip" data-min="30">30m</button>
        <button class="chip" data-min="45">45m</button>
        <button class="chip" data-min="60">60m</button>
        <button class="chip" data-min="90">90m</button>
        <button class="chip" data-min="120">120m</button>
      </div>
      <input type="number" id="manualMinutes" placeholder="or type minutes (1-120)" min="1" max="120">
      <label class="checkbox-row">
        <input type="checkbox" id="attachToggle" checked>
        Attach to current status (pizza takes the status slide's beat)
      </label>
      <div class="row">
        <button class="primary" id="startTimerBtn">Start timer</button>
        <button class="ghost" id="cancelTimerBtn">Cancel</button>
      </div>
    </div>

    <div class="readout" id="readout">Ready.</div>
  </div>

  <script src="app.js"></script>
</body>
</html>
