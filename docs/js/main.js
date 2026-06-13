import { showDashboard } from "./dashboard.js";

const params = new URLSearchParams(window.location.search);

if (params.get("client_id") && params.get("server")) {
    localStorage.setItem("client_id", params.get("client_id"));
    localStorage.setItem("server", params.get("server"));
    console.log("Client ID and Server stored. Starting OAuth flow...");
    startOauth();
} else if (params.get("code") && params.get("scope") && params.get("state")) {

    const server = localStorage.getItem("server");
    const scheme = location.protocol === 'https:' ? 'wss' : 'ws';
    const wsAuth = new WebSocket(`${scheme}://${server}/trader/oauth`);

    (async () => {
        const code = params.get("code");
        const state = params.get("state");
        const codeVerifier = localStorage.getItem("pkce_code_verifier");
        const oauthState = localStorage.getItem("oauth_state");

        if (!oauthState || state !== oauthState) {
            document.body.innerHTML = "<h2>Invalid OAuth state.</h2>";
            return;
        }

        document.body.innerHTML = "<h2>Processing login…</h2>";
        try {
            const data = await exchangeCode(code, codeVerifier, wsAuth);
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
    })();
} else if (localStorage.getItem("logged_in") === "1") {
    showDashboard();
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

async function exchangeCode(code, codeVerifier, wsAuth) {
    return new Promise((resolve, reject) => {
        let settled = false;

        const safeResolve = (val) => {
            if (settled) return;
            settled = true;
            try { resolve(val); } catch (e) { /* noop */ }
        };

        const safeReject = (err) => {
            if (settled) return;
            settled = true;
            try { reject(err); } catch (e) { /* noop */ }
        };

        wsAuth.onopen = () => {
            // Send token exchange request
            wsAuth.send(JSON.stringify({
                action: "exchange_code",
                code: code,
                code_verifier: codeVerifier,
                client_id: localStorage.getItem("client_id")
            }));
        };

        wsAuth.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                if (data.type === "exchange_result") {
                    // Mark settled before closing to avoid onclose race
                    safeResolve(data); // { ok: true, token: "...", ... }
                    try { wsAuth.close(1000); } catch (e) { /* ignore */ }
                }
            } catch (err) {
                try { wsAuth.close(); } catch (e) { /* ignore */ }
                safeReject(err);
            }
        };

        wsAuth.onerror = (err) => {
            safeReject(err || new Error('WebSocket error'));
        };

        wsAuth.onclose = (evt) => {
            // If closed before resolving, reject
            if (!settled) {
                safeReject(new Error("WebSocket closed unexpectedly"));
            }
        };
    });
}
