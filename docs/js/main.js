// main.js
const params = new URLSearchParams(window.location.search);

if (params.get("client_id") && params.get("server")) {
    localStorage.setItem("client_id", params.get("client_id"));
    localStorage.setItem("server", params.get("server"));
    console.log("Client ID and Server stored. Starting OAuth flow...");
    startOauth();
}

if (params.get("code") && params.get("scope") && params.get("state")) {
    const codeVerifier = localStorage.getItem("pkce_code_verifier");
    const oauthState = localStorage.getItem("oauth_state");
    const server = localStorage.getItem("server");
    if (!oauthState || state !== oauthState) {
        document.body.innerHTML = "<h2>Invalid OAuth state.</h2>";
        return;
    }
    document.body.innerHTML = "<h2>Processing login…</h2>";
    try {
        const data = await exchangeCode(code, codeVerifier);
        if (data.ok) {
            localStorage.setItem("logged_in", "1");
            window.history.replaceState({}, "", location.pathname);
            showDashboard();
        } else {
            document.body.innerHTML = "<h2>Token exchange failed.</h2>";
        }
    } catch (e) {
        console.error("Token exchange error:", e);
        document.body.innerHTML = "<h2>Token exchange failed.</h2>";
    }
    return true;
}

async function startOauth() {
    const clientId = localStorage.getItem("client_id");

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

    // Perform redirect
    window.location.href = "https://auth.deriv.com/oauth2/auth?" + authParams.toString();
}

async function exchangeCode(code, codeVerifier) {
    const server = localStorage.getItem("server");
    return new Promise((resolve, reject) => {
        const wsAuth = new WebSocket(`wss://${server}:5000/api/oauth/exchange`);

        wsAuth.onopen = () => {
            // Send token exchange request
            wsAuth.send(JSON.stringify({
                action: "exchange_code",
                code: code,
                code_verifier: codeVerifier
            }));
        };

        wsAuth.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                if (data.type === "exchange_result") {
                    wsAuth.close();
                    resolve(data); // { ok: true, token: "...", ... }
                }
            } catch (err) {
                wsAuth.close();
                reject(err);
            }
        };

        wsAuth.onerror = (err) => {
            reject(err);
        };

        wsAuth.onclose = (evt) => {
            // If closed before resolving, reject
            if (evt.code !== 1000) {
                reject(new Error("WebSocket closed unexpectedly"));
            }
        };
    });
}
