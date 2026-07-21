(function () {
  "use strict";

  const readout = document.getElementById("readout");

  function show(msg, isError) {
    readout.textContent = msg;
    readout.classList.toggle("error", !!isError);
  }

  async function api(params, label) {
    show("Sending...");
    try {
      const res = await fetch("api.php", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: new URLSearchParams(params).toString(),
      });
      if (res.status === 401) {
        window.location.href = "auth.php";
        return null;
      }
      const body = await res.json().catch(() => ({}));
      if (!res.ok || !body.ok) {
        show(body.error || ("Error " + res.status), true);
        return null;
      }
      show(label);
      return body;
    } catch (err) {
      show("Request failed - check your connection.", true);
      return null;
    }
  }

  // ---- Status buttons ----
  const statusButtons = document.querySelectorAll(".status-btn[data-status]");
  statusButtons.forEach((btn) => {
    btn.addEventListener("click", async () => {
      const result = await api(
        { action: "status", key: btn.dataset.status },
        "Sign set: " + btn.textContent
      );
      if (result) {
        statusButtons.forEach((b) => b.classList.toggle("is-active", b === btn));
      }
    });
  });

  // ---- Message ----
  const messageInput = document.getElementById("messageInput");
  document.getElementById("sendMessageBtn").addEventListener("click", () => {
    const text = messageInput.value.trim();
    if (!text) { show("Type a message first.", true); return; }
    api({ action: "message", text: text }, "Message scrolling: “" + text + "”");
  });
  document.getElementById("clearMessageBtn").addEventListener("click", () => {
    messageInput.value = "";
    api({ action: "message_clear" }, "Message cleared.");
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
      { action: "timer", minutes: selectedMinutes, attached: attached ? "1" : "0" },
      "Timer started: " + selectedMinutes + "m (" + (attached ? "attached" : "standalone") + ")"
    );
  });
  document.getElementById("cancelTimerBtn").addEventListener("click", () => {
    api({ action: "timer_cancel" }, "Timer cancelled.");
  });
})();
