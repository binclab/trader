// main.js
const params = new URLSearchParams(window.location.search);

if (params.get("client_id") && params.get("redirect_uri")) {
    localStorage.setItem("client_id", params.get("client_id"));
    localStorage.setItem("redirect_uri", params.get("redirect_uri"));
    console.log("Client ID and Redirect URI stored. Starting OAuth flow...");
    startOauth();
}

async function startOauth() {
    const clientId = localStorage.getItem("client_id");
    const redirectUri = localStorage.getItem("redirect_uri");

    // Generate random state
    const random = crypto.getRandomValues(new Uint8Array(16));
    const state = Array.from(random).map(b => b.toString(16).padStart(2, '0')).join('');

    // Generate PKCE code verifier
    const array = crypto.getRandomValues(new Uint8Array(64));
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~';
    const codeVerifier = Array.from(array).map(v => chars[v % chars.length]).join('');

    // Derive code challenge
    const data = new TextEncoder().encode(codeVerifier);
    const hashBuffer = await crypto.subtle.digest('SHA-256', data);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    const base64 = btoa(String.fromCharCode.apply(null, hashArray));
    const codeChallenge = base64
        .replace(/\+/g, '-')
        .replace(/\//g, '_')
        .replace(/=+$/, '');

    // Persist verifier and state
    localStorage.setItem("pkce_code_verifier", codeVerifier);
    localStorage.setItem("oauth_state", state);

    // Build OAuth URL
    const authParams = new URLSearchParams({
        response_type: "code",
        client_id: clientId,
        redirect_uri: "https://trader.binclab.com/index",
        scope: "trade account_manage",
        state: state,
        code_challenge: codeChallenge,
        code_challenge_method: "S256"
    });

    console.log("Redirecting to OAuth URL:", "https://auth.deriv.com/oauth2/auth?" + authParams.toString());

    // Perform redirect
    //window.location.href = "https://auth.deriv.com/oauth2/auth?" + authParams.toString();
}