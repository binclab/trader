// main.js

// Parse URL parameters
const params = new URLSearchParams(window.location.search);

const clientId = params.get("clientId");
if (clientId) {
    localStorage.setItem("clientId", clientId);
    localStorage.setItem("hostname", params.get("hostname"));
}
