// main.js
const params = new URLSearchParams(window.location.search);
const clientId = params.get("client_id");
const redirectUri = params.get("redirect_uri");

if (clientId && redirectUri) {
    localStorage.setItem("client_id", clientId);
    localStorage.setItem("redirect_uri", redirectUri);
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
    const hash = await crypto.subtle.digest('SHA-256', data);
    const bytes = new Uint8Array(hash);
    let str = '';
    for (const b of bytes) str += String.fromCharCode(b);
    const codeChallenge = btoa(str)
        .replace(/\+/g, '-')
        .replace(/\//g, '_')
        .replace(/=+$/, '');

    // Persist verifier and state for later token exchange
    localStorage.setItem("pkce_code_verifier", codeVerifier);
    localStorage.setItem("oauth_state", state);

    // Build OAuth URL
    const authParams = new URLSearchParams({
        response_type: "code",
        client_id: clientId,
        redirect_uri: redirectUri, // use the exact redirect_uri
        scope: "trade account_manage",
        state: state,
        code_challenge: codeChallenge,
        code_challenge_method: "S256"
    });

    window.location.href = "https://auth.deriv.com/oauth2/auth?" + authParams.toString();
}