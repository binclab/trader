// main.js

// Parse URL parameters
const params = new URLSearchParams(window.location.search);

const clientId = params.get("client_id");
if (clientId) {
    localStorage.setItem("client_id", clientId);
    localStorage.setItem("redirect_uri", params.get("redirect_uri"));
}
