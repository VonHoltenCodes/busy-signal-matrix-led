(function () {
  "use strict";

  const readout = document.getElementById("readout");

  function show(msg, isError) {
    readout.textContent = msg;
    readout.classList.toggle("error", !!isError);
  }

  async function api(path, label) {
    show("Sending...");
    try {
      const res = await fetch(path, { method: "POST" });
      const body = await res.json().catch(() => ({}));
      if (!res.ok) {
        show(body.detail || ("Error " + res.status), true);
        return null;
      }
      show(label);
      return body;
    } catch (err) {
      show("Server unreachable - is it running?", true);
      return null;
    }
  }

  // ---- Status buttons ----
  const statusButtons = document.querySelectorAll(".status-btn[data-status]");
  statusButtons.forEach((btn) => {
    btn.addEventListener("click", async () => {
      const key = btn.dataset.status;
      const result = await api("/api/status/" + key, "Sign set: " + btn.textContent);
      if (result) {
        statusButtons.forEach((b) => b.classList.toggle("is-active", b === btn));
      }
    });
  });

  // ---- Message ----
  const messageInput = document.getElementById("messageInput");
  document.getElementById("sendMessageBtn").addEventListener("click", async () => {
    const text = messageInput.value.trim();
    if (!text) { show("Type a message first.", true); return; }
    await api("/api/message?text=" + encodeURIComponent(text), "Message scrolling: “" + text + "”");
  });
  document.getElementById("clearMessageBtn").addEventListener("click", () => {
    messageInput.value = "";
    api("/api/message/clear", "Message cleared.");
  });
  messageInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter") document.getElementById("sendMessageBtn").click();
  });

  // ---- Timer ----
  let selectedMinutes = 15;
  const chips = document.querySelectorAll(".chip[data-min]");
  const manualMinutes = document.getElementById("manualMinutes");

  chips.forEach((chip) => {
    chip.addEventListener("click", () => {
      selectedMinutes = Number(chip.dataset.min);
      manualMinutes.value = "";
      chips.forEach((c) => c.classList.toggle("is-active", c === chip));
    });
  });

  manualMinutes.addEventListener("input", () => {
    const v = Number(manualMinutes.value);
    if (v > 0) {
      selectedMinutes = v;
      chips.forEach((c) => c.classList.remove("is-active"));
    }
  });

  document.getElementById("startTimerBtn").addEventListener("click", () => {
    const attached = document.getElementById("attachToggle").checked;
    api(
      "/api/timer?minutes=" + selectedMinutes + "&attached=" + attached,
      "Timer started: " + selectedMinutes + "m (" + (attached ? "attached" : "standalone") + ")"
    );
  });
  document.getElementById("cancelTimerBtn").addEventListener("click", () => {
    api("/api/timer/cancel", "Timer cancelled.");
  });
})();
