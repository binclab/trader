// main.js

// Parse URL parameters
const params = new URLSearchParams(window.location.search);

const clientId = params.get("client_id");
if (clientId) {
    localStorage.setItem("clientId", clientId);
    localStorage.setItem("hostname", params.get("redirect_uri"));
}
