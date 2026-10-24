// Server security report: `aze serve` stack (`Aze` vs `qt-plus`)
// Scope: request handling and transport below the portal boundary. Authentication and authorization are out of scope: they are enforced upstream by the portal. `aze serve` only receives already authorized traffic.
// Method: source review of `Aze/sources/server/*`, `AzeLib/sources/CRemoteRepository.cpp`, `qt-plus/source/cpp/Web/CHTTP*`, `CWebContext`, live TLS path in `scripts/linux/serve.sh`

//-------------------------------------------------------------------------------------------------
// 1. Architecture and responsibility split
//-------------------------------------------------------------------------------------------------

// Repository layout enforces `Aze -> AzeLib -> qt-plus`. The server adds no direct storage access outside commands.

// Owned by `Aze` (`Aze/sources/server/`):
// - `CAzeServer.cpp:25-60`, `CAzeServer.h:23-68`: `QCoreApplication` bootstrap, HTTP port wiring, HTTPS opt-in validation (`--https-port` requires `--ssl-cert` + `--ssl-key`), event loop entry.
// - `CAzeServerProcessor.cpp:91-141`, `CAzeServerProcessor.h:23-96`: HTTP body gate (`pathValid`, non-empty POST, `Content-Type: application/json`), repository path confinement via `resolveRepositoryPath`, JSON parse, `CRepository` open check, dispatch to `CRemoteRepository`, post-serve hook.
// - `CAzeServerProcessor.cpp:194-275`: action dispatch map (`pull`, `push`, `push-announce`, `clone`, `delete-branch`), unknown action rejection, per-action delegation to `Aze::CRemoteRepository`.
// - `CAzeHousekeeper.cpp:57-137`, `CAzeHousekeeper.h:21-73`: post-serve prune hook (push and delete-branch only), periodic sweep under base folder, threshold-gated single repository prune, silent repository open, error swallowing.
// - `CAzeServerArguments.cpp:21-41`: CLI surface (`--port`, `--https-port`, `--ssl-cert`, `--ssl-key`). No identity options by design, the portal owns identity.
// - `AzeLib/sources/CRemoteRepository.cpp:562-782` (used by the server, owned by `AzeLib`): pull/push/clone/delete-branch semantics, fast-forward refusal in `CPushCommand.cpp:155-157`, branch existence checks before serving.

// Owned by `qt-plus` (`qt-plus/source/cpp/Web/`):
// - `CHTTPServer.cpp:50-103,204-296,361-422`: TCP listen on `QHostAddress::Any`, optional second `CHttpsListener`, TLS configuration (`QSslConfiguration`, `VerifyNone`, `SecureProtocols`), static and dynamic IP blacklists, per-IP `CRequestMonitor` flood accounting, session expiry in `onMaintenance`.
// - `CHTTPRequestProcessor.cpp:52-127`: per-connection `QThread`, TLS handshake (`startServerEncryption`, 10 s `waitForEncrypted`), fixed 20 KiB socket read buffer, header plus `Content-Length` framing, multipart handling, file fallback then dynamic content.
// - `CHTTPRequestProcessor.cpp:793-892`: static file fallback gated by `authorizedFolders()`; forbidden response when file exists outside authorized folders.
// - `CWebContext.h:55,58-59`, `CWebContext.cpp:71-74`: path split, peer address cleaning, `isSecure()` detection via `qobject_cast<QSslSocket*>`.
// - `CDynamicHTTPServer.cpp:115-185`: base dynamic content path (`getPage`, viewstate events). `CAzeServerProcessor` overrides it and does not use the page path.

// Boundary with the portal:
// - The portal terminates external TLS, authenticates users, applies repository and action authorization, then forwards allowed `pull`, `push`, `push-announce`, `clone`, `delete-branch` JSON requests to `aze serve`.
// - `Aze` treats the downstream channel as already authorized. It still owns integrity checks (path confinement, fast-forward refusal, force-flag handling) and resource limits. It does not own user identity.
// - `qt-plus` owns point-to-point protection between portal and `aze serve` (bind address, TLS, framing, flood accounting).

//-------------------------------------------------------------------------------------------------
// 2. Trust model
//-------------------------------------------------------------------------------------------------

// - Allowed source: the portal only. Direct external access to `aze serve` is a deployment fault, not a supported configuration.
// - The portal may send all five actions. Clone and pull disclosure, push history growth, and delete-branch with `force=true` are legitimate portal-mediated operations. `Aze` must not second-guess the portal authorization decision.
// - `CWebContext::m_sPeer` is captured but unused (`CAzeServerProcessor.cpp:202` leaves the IP lookup commented out). This is consistent with portal ownership of identity: no per-user audit is expected at this layer. Portal IP allowlisting, if used, belongs to deployment configuration, not per-request user checks.
// - Server filesystem identity remains the local trust boundary: the process can read and write every repository under the base folder. The default base path is `"."` (`CAzeServer.cpp:27`), so the working folder determines exposure behind the portal.
// - TLS between portal and `aze serve` protects the already authorized payload on the inner hop. End-to-end confidentiality toward browsers or CLI clients is a portal responsibility.

//-------------------------------------------------------------------------------------------------
// 3. Findings owned by `Aze`
//-------------------------------------------------------------------------------------------------

// --- A1 (Medium): service must not be directly reachable, no in-scope compensating check ---
// Location: `Aze/sources/server/CAzeServerProcessor.cpp:60-64,214-275`, `Aze/sources/server/CAzeServer.cpp:27`
// - The dispatch map accepts `push` and `delete-branch` from any TCP peer able to reach the port. This is by design under portal delegation, so it is not filed as a missing-authentication defect.
// - Residual risk is deployment-only: if the port is exposed past the portal, the portal policy is bypassed and any peer inherits full read, write, and delete capability (bounded only by fast-forward and force-flag rules).
// - Required posture, owner `Aze` for options and documentation, operator for enforcement: bind to loopback or portal-only interface, firewall the port to the portal IP range, document direct exposure as unsupported. A `--bind-address` option is the missing code piece; see section 6.

// --- A2 (Medium): repository confinement is correct but narrow ---
// Location: `Aze/sources/server/CAzeServerProcessor.cpp:154-188`, tests in `Aze/sources/tests/CTestServe.cpp:702-728`
// - Positive: rejects empty segments, `.`, `..`, embedded `/`, `\`, `:`, enforces `cleanPath` prefix containment and rechecks `canonicalFilePath` when both base and candidate exist. This blocks the direct `..` and separator traversal cases.
// - Residual risks:
//   1. Default base path `"."` is broad. Behind the portal this still matters: one compromised or mis-scoped portal route can address every sibling repository folder under the working folder.
//   2. Time-of-check to time-of-use on symlinks: canonical check happens before `CRepository` open. A swapped symlink between check and open can redirect. Low likelihood locally, higher on shared hosting.
//   3. `QDirIterator` in `CAzeHousekeeper.cpp:91-108` uses `NoSymLinks`, but the request path resolves a single path; symlink escape after the canonical check is still the weak window.
// - Fix owner: `Aze`. Resolve base once at startup to canonical form, refuse to serve when base path is `.` unless explicitly allowed, reopen canonical path and reverify containment after open, document that base folder must hold only served repositories.

// --- A3 (Low): error responses distinguish states, useful behind the portal ---
// Location: `Aze/sources/server/CAzeServerProcessor.cpp:103-139,194-210`
// - Distinct nodes for empty request, invalid response content type, not-a-repository, unknown action, plus detailed `Aze::CError` nodes from push and delete paths.
// - Behind an authorized portal this verbosity is acceptable and helps portal operators diagnose routes. If portal error passthrough reaches untrusted users verbatim, it becomes a repository and branch enumeration oracle (`eBranchDoesNotExist` vs success).
// - Fix owner: `Aze` for a quiet mode if needed, portal for error sanitization at the edge. Keep detailed errors by default, let the portal decide what to forward.

// --- A4 (Low): input validation is partial at the `Aze` layer ---
// Location: `Aze/sources/server/CAzeServerProcessor.cpp:91-141`
// - `Aze` enforces JSON content type and non-empty body, and `CRemoteRepository` validates `request` and `branch` attributes per action.
// - `Aze` sets no body size cap, no JSON depth cap, no commit or object count cap. Oversized portal-forwarded payloads reach `CXMLNode::parseJSON` and `CPushCommand` before any `Aze`-owned limit. A faulty or abused portal can therefore forward resource exhaustion.
// - Fix owner: `Aze` for semantic limits (max body bytes, max commits per push, max object bytes). `qt-plus` owns transport framing limits; see Q3.

// --- A5 (Informative): integrity controls stay correct under delegation ---
// Location: `AzeLib/sources/CRemoteRepository.cpp:562-782`, `AzeLib/sources/commands/CPushCommand.cpp:155-157`
// - Push still refuses history loss without fast-forward. Delete-branch still honors the `force` flag and the branch versus tag type. Pull still returns attributes-only on converged tips and bounded deltas on known tips.
// - These controls protect repository integrity against buggy portal callers. They are not access controls and are not evaluated as such in this report.
// - Housekeeper posture is unchanged: only `push` and `delete-branch` trigger gated prune (`CAzeHousekeeper.cpp:57-64`), threshold 500 stored files, retention 86400 s, sweep interval 3600000 ms, silent open, errors ignored. No direct client trigger exists.

//-------------------------------------------------------------------------------------------------
// 4. Findings owned by `qt-plus`
//-------------------------------------------------------------------------------------------------

// --- Q1 (Medium): inner-hop TLS posture is enforced by configuration ---
// Location: `qt-plus/source/cpp/Web/CHTTPServer.cpp:389-435`, `CHTTPRequestProcessor.cpp:55-100`, `Aze/sources/server/CAzeServer.cpp:63-125`
// - Server certificate and RSA/ECDSA key load from PEM is validated. Failure returns false and `CAzeServer` refuses to run with a clear error. Positive, unchanged.
// - Protocol floor is `QSsl::TlsV1_2OrLater` in `enableHttps`, no longer Qt-version dependent. HSTS is opt-in via `setHstsEnabled`/`setHstsMaxAge`.
// - `setSslPeerVerifyMode` plus `setSslCaCertificates` expose portal client certificate verification. Wired to `aze serve` as `--ssl-verify-peer` (requires `--https-port` with certificate and key) and `--ssl-ca` (portal CA bundle, system store when absent). Misconfigurations fail closed with `eInvalidServerConfiguration`.
// - Qt verifies a presented client certificate but tolerates a missing one, so `CHTTPRequestProcessor::run` drops peers without a certificate whenever the mode is above `QueryPeer`. `QueryPeer` keeps ask-without-refusing semantics.
// - Cipher suite selection stays on Qt defaults under the TLS 1.2 floor, no operator knob by design.
// - Fix owner: operator selects the HTTPS endpoint for split-host deployments and passes `--ssl-verify-peer --ssl-ca`; single-host loopback deployments keep plain HTTP loopback (A1).

// --- Q2 (Medium): flood protection exists but is weak and partially bypassed ---
// Location: `qt-plus/source/cpp/Web/CHTTPServer.h:41-104`, `CHTTPServer.cpp:361-422`, `CHTTPRequestProcessor.cpp:163-171,223-241`
// - Positive: per-IP monitor (15 requests per 4 s sample, more than 15 simultaneous triggers `m_bFlooding`), static blacklist entries, dynamic blacklist insertion, `requestStart` and `requestEnd` accounting.
// - Weaknesses:
//   1. `incomingConnection` (`CHTTPServer.cpp:603-614`) forwards every descriptor to a new `QThread` before any blacklist check. The commented blacklist block shows the intended early reject is disabled. Each connection therefore costs a thread even from blocked IPs. Behind a portal the source IP is the portal, so per-IP accounting collapses to one heavy peer.
//   2. `onSocketReadyRead` checks `shouldBeBlocked` before parsing, but `processRequest` still parses headers, buffers POST bodies, and dispatches to `Aze` before post-request accounting detects sustained flood. Slow POST and large body abuse from a misbehaving forwarder remain.
//   3. Maintenance interval is 60 s (`CHTTPServer.cpp:56`), session lifetime 15 min; dynamic blacklist growth and session map growth are bounded only by those timers.
// - Fix owner: `qt-plus`. Restore early reject in `incomingConnection`, enforce max connections per IP and global cap, add max header bytes and max body bytes with early close, make thresholds configurable. Operator: keep portal-to-server concurrency bounded independently.

// --- Q3 (Medium): no transport size cap before `Aze` JSON handling ---
// Location: `qt-plus/source/cpp/Web/CHTTPRequestProcessor.cpp:90-93,192-241,321-343,648-676`
// - Read buffer is set to 20 KiB (`setReadBufferSize(1024 * 20)`), but `m_baBuffer.append(baData)` accumulates the full body and `Content-Length` is trusted via `toInt()` with no upper bound check visible in `getExpectedBytes`.
// - Consequence: large portal-forwarded `push` payloads (many commits and base64 objects) allocate in the worker thread before `Aze` validates semantics. Combined with one thread per connection, this is the primary memory and CPU exhaustion vector on the inner hop.
// - Fix owner: `qt-plus` for `Content-Length` ceiling and total buffered body ceiling with `413` close. `Aze` for semantic ceiling (finding A4). Both limits are needed.

// --- Q4 (Low): static file fallback is safe by default in this deployment ---
// Location: `qt-plus/source/cpp/Web/CHTTPRequestProcessor.cpp:793-892`, `CHTTPServer.cpp:304-310`
// - Files are served only when they exist under `applicationDirPath` plus request path and only when the first path segment matches an entry in `authorizedFolders()`.
// - `CAzeServerProcessor` never calls `addAuthorizedFolder`, so the authorized list stays empty in the `aze serve` deployment. Requests therefore fall through to dynamic content. No path traversal to static files was found in this configuration.
// - Residual: the fallback constructs the disk path by string concatenation without canonicalization. Any future `addAuthorizedFolder` call in `Aze` would inherit that weakness. Keep the list empty or fix `getResponseFile` to canonicalize before serving.
// - Fix owner: `qt-plus` for canonicalization hardening; `Aze` for keeping the current empty-list configuration.

// --- Q5 (Low): cleartext HTTP on all interfaces is a portal-bypass enabler ---
// Location: `qt-plus/source/cpp/Web/CHTTPServer.cpp:91-98`, `Aze/sources/server/CAzeServerArguments.cpp:23-26`, `scripts/linux/serve.sh:28-37,174-178`
// - HTTP binds `QHostAddress::Any` on the configured port (default 80 at `qt-plus` level, 8080 at script level). HTTPS is opt-in at the code level (`uiHttpsPort == 0` disables) and opt-out only at the script level.
// - On the inner hop the risk is not passive Internet capture but portal bypass: a cleartext all-interface listener is easier to reach and reuse when firewall rules drift.
// - Fix owner: shared. `Aze` documents portal-only binding and adds bind-address selection; `qt-plus` supports bind-address restriction instead of hardcoded `Any`; operator binds to loopback or portal network and prefers the HTTPS endpoint.

//-------------------------------------------------------------------------------------------------
// 5. Joint risk summary (portal enforces auth)
//-------------------------------------------------------------------------------------------------

// | ID | Owner | Risk | Condition | Recommended action |
// | A1 | Aze + operator | Medium | Port reachable past the portal | Bind to portal-only interface, firewall aperture, add `--bind-address` |
// | Q1 | qt-plus + Aze | Medium | Inner hop without peer verification | Done: --ssl-verify-peer/--ssl-ca, presence enforced, TLS 1.2 floor |
// | Q2 | qt-plus | Medium | High concurrency from forwarder | Early blacklist reject, connection caps, tunable flood thresholds |
// | Q3 | qt-plus + Aze | Medium | Large forwarded push body | Transport body cap in `qt-plus`, semantic push cap in `Aze` |
// | A2 | Aze | Medium | Overbroad base path, symlink TOCTOU | Canonical base at startup, refuse `.` default, reverify after open |
// | A3 | Aze + portal | Low | Verbose errors forwarded to users | Keep detail for operators, sanitize at portal edge if needed |
// | Q5 | shared | Low | Cleartext all-interface listener | Bind-address option, portal-network binding, production TLS guidance |
// | Q4 | qt-plus | Low (current deployment safe) | None today; latent if authorized folders are added | Canonicalize static file path |
// | A5 | Aze | Informative | Integrity controls intact | No change, retain fast-forward and force-flag enforcement |
// | A4 | Aze | Low | Unbounded portal-forwarded JSON | Add semantic push limits complementary to Q3 |

//-------------------------------------------------------------------------------------------------
// 6. Recommended remediation order
//-------------------------------------------------------------------------------------------------

// 1. `Aze` + operator: make portal-only deployment the enforced default. Add `--bind-address`, document loopback or private-network binding, firewall the port to portal IPs, treat direct exposure as a fault.
// 2. `qt-plus` + `Aze`: done. Early blacklist rejection, connection and body caps, TLS 1.2 floor, portal peer verification wired (`--ssl-verify-peer`, `--ssl-ca`, presence enforced above `QueryPeer`). Operator selects the HTTPS endpoint for split-host deployments.
// 3. `Aze`: harden storage handling (canonical base resolution, no `.` default in production guidance, reverify after open, semantic push limits, optional quiet error mode for portal passthrough).
// 4. Tests: done. `CTestServe` covers portal-bypass binding checks, oversized body rejection, concurrent connection cap, symlink swap coverage, mutual TLS handshake (chained, missing and rogue client certificates) and `--ssl-verify-peer`/`--ssl-ca` validation with live enforcement.
