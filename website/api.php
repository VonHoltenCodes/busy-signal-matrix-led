<?php
/**
 * Busy Signal - server-side proxy to the LED board.
 *
 * The board only speaks plain HTTP on the LAN with zero auth, so this
 * endpoint is the security boundary: it requires the PIN-gate session
 * (set by auth.php, which is deploy-only and not in this repo) and only
 * forwards a whitelisted set of commands.
 */

ini_set('session.cookie_httponly', 1);
ini_set('session.use_only_cookies', 1);
ini_set('session.cookie_samesite', 'Strict');
session_name('BUSYSIGNAL_SESSION');
session_start();

header('Content-Type: application/json');

if (!isset($_SESSION['busy_signal_authenticated']) || $_SESSION['busy_signal_authenticated'] !== true) {
    http_response_code(401);
    echo json_encode(['ok' => false, 'error' => 'Not authenticated']);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['ok' => false, 'error' => 'POST only']);
    exit;
}

define('BOARD_BASE', 'http://192.168.68.74');
define('BOARD_TIMEOUT', 4);

// Must match the route table in firmware/BusySignal/BusySignal.ino
$STATUSES = ['meeting', 'call', 'racing', 'recording', 'working', 'comein'];

function board_get($path) {
    $ctx = stream_context_create(['http' => ['timeout' => BOARD_TIMEOUT]]);
    $body = @file_get_contents(BOARD_BASE . $path, false, $ctx);
    return $body !== false;
}

$action = $_POST['action'] ?? '';
$path = null;

switch ($action) {
    case 'status':
        $key = $_POST['key'] ?? '';
        if (!in_array($key, $STATUSES, true)) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'error' => 'Unknown status']);
            exit;
        }
        $path = '/' . $key;
        break;

    case 'message':
        $text = trim($_POST['text'] ?? '');
        if ($text === '' || mb_strlen($text) > 100) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'error' => 'Message must be 1-100 characters']);
            exit;
        }
        $path = '/message?text=' . rawurlencode($text);
        break;

    case 'message_clear':
        $path = '/message/clear';
        break;

    case 'timer':
        $minutes = (int)($_POST['minutes'] ?? 0);
        if ($minutes < 1 || $minutes > 120) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'error' => 'Minutes must be 1-120']);
            exit;
        }
        $attached = ($_POST['attached'] ?? '1') === '1' ? '1' : '0';
        $path = '/timer?minutes=' . $minutes . '&attached=' . $attached;
        break;

    case 'timer_cancel':
        $path = '/timer/cancel';
        break;

    default:
        http_response_code(400);
        echo json_encode(['ok' => false, 'error' => 'Unknown action']);
        exit;
}

if (board_get($path)) {
    echo json_encode(['ok' => true]);
} else {
    http_response_code(502);
    echo json_encode(['ok' => false, 'error' => 'Sign unreachable - is it powered on?']);
}
