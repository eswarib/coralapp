const { ipcMain } = require('electron');

ipcMain.handle('getAppVersion', () => app.getVersion())
const { app, Tray, Menu, BrowserWindow, dialog, screen } = require('electron');
const path = require('path');
const os = require('os');
const fs = require('fs');
const { spawn } = require('child_process');
const readline = require('readline');

let originalIconPath;
const ANIMATION_FRAME_COUNT = 8;
let animationFrames = [];
function resolveIconPaths() {
  if (process.env.APPIMAGE) {
    for (let i = 0; i < ANIMATION_FRAME_COUNT; i++) {
      animationFrames.push(path.join(process.resourcesPath, 'icons', 'wave_icons', `wave_${i}.png`));
    }
    originalIconPath = path.join(process.resourcesPath, 'coral.png');
  } else {
    const baseDir = __dirname;
    for (let i = 0; i < ANIMATION_FRAME_COUNT; i++) {
      animationFrames.push(path.join(baseDir, 'icons', 'wave_icons', `wave_${i}.png`));
    }
    // Try coral.png in coral-electron, then logo/coral.png
    const candidates = [
      path.join(baseDir, 'coral.png'),
      path.join(baseDir, '..', 'logo', 'coral.png'),
      path.join(process.resourcesPath || baseDir, 'coral.png'),
    ];
    originalIconPath = candidates.find(p => fs.existsSync(p)) || candidates[0];
  }
}
resolveIconPaths();

let tray = null;
let configWindow = null;
let devWindow = null;
let backendProcess = null;
let animationInterval = null;
let currentFrame = 0;

function createConfigWindow() {
  if (configWindow) {
    configWindow.focus();
    return;
  }
  configWindow = new BrowserWindow({
    width: 520,
    height: 360,
    useContentSize: true,
    resizable: true,
    autoHideMenuBar: true,
    title: 'Settings', // Changed from 'Edit Config' to 'Settings'
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
    },
  });
  // Ensure the menu bar is not visible for this window
  configWindow.setMenuBarVisibility(false);
  configWindow.loadFile(path.join(__dirname, 'renderer', 'index.html'));
  configWindow.on('closed', () => {
    configWindow = null;
  });
}

function createDevWindow() {
  if (devWindow) {
    devWindow.focus();
    return;
  }
  devWindow = new BrowserWindow({
    width: 520,
    height: 380,
    useContentSize: true,
    resizable: true,
    autoHideMenuBar: true,
    title: 'Developer Settings',
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
    },
  });
  devWindow.setMenuBarVisibility(false);
  devWindow.loadFile(path.join(__dirname, 'renderer', 'dev.html'));
  devWindow.on('closed', () => {
    devWindow = null;
  });
}

function getAppImageMountPath()
{
    if (process.env.APPIMAGE)
    {
        // process.execPath: /tmp/.mount_Coral-XXXXXX/usr/bin/electron
        // We want: /tmp/.mount_Coral-XXXXXX
        return path.join(path.dirname(process.execPath), "..", "..", "..");
    }
    return null;
}

function isChildAlive(child) {
  if (!child || !child.pid) return false;
  try {
    process.kill(child.pid, 0);
    return true;
  } catch {
    return false;
  }
}

function startTrayAnimation() {
  if (animationInterval) return;
  currentFrame = 0;
  animationInterval = setInterval(() => {
    tray.setImage(animationFrames[currentFrame]);
    currentFrame = (currentFrame + 1) % ANIMATION_FRAME_COUNT;
  }, 100); // 100ms per frame
}
function stopTrayAnimation() {
  if (animationInterval) {
    clearInterval(animationInterval);
    animationInterval = null;
    tray.setImage(originalIconPath);
  }
}

// Prompt user to set up input permissions for hotkey detection and text injection.
// Two things are needed:
//   1. User in 'input' group  → access to /dev/input/event* (key detection via evdev)
//   2. udev rule for uinput   → access to /dev/uinput (text injection via virtual keyboard)
// Similar to how Wireshark asks for packet-capture permissions on first run.
let inputGroupPromptShown = false;
function promptInputGroupPermission() {
    if (inputGroupPromptShown) return;  // only ask once per session
    inputGroupPromptShown = true;

    const { execSync } = require('child_process');
    const currentUser = os.userInfo().username;

    // Check if both permissions are already in place
    let hasInputGroup = false;
    let hasUinputRule = false;
    try {
        const groups = execSync('groups', { encoding: 'utf-8' });
        hasInputGroup = groups.includes('input');
    } catch (_) {}
    try {
        hasUinputRule = fs.existsSync('/etc/udev/rules.d/99-coral-uinput.rules');
    } catch (_) {}

    if (hasInputGroup && hasUinputRule) return;  // all set

    dialog.showMessageBox({
        type: 'warning',
        title: 'Keyboard Access Required',
        message: 'Coral needs access to keyboard input devices for hotkey detection and text injection.\n\n' +
                 'This is a one-time setup that requires your password:\n' +
                 '  • Adds your user to the "input" group\n' +
                 '  • Enables the virtual keyboard device (/dev/uinput)\n\n' +
                 'You will need to log out and back in for the change to take effect.',
        buttons: ['Grant Access', 'Skip'],
        defaultId: 0,
        cancelId: 1,
    }).then((result) => {
        if (result.response === 0) {
            // Single pkexec call with a shell script that does both steps
            const { exec } = require('child_process');
            const setupCmd = `pkexec sh -c '` +
                `usermod -aG input ${currentUser} && ` +
                `echo "KERNEL==\\"uinput\\", GROUP=\\"input\\", MODE=\\"0660\\"" > /etc/udev/rules.d/99-coral-uinput.rules && ` +
                `udevadm control --reload-rules && ` +
                `udevadm trigger /dev/uinput` +
                `'`;
            exec(setupCmd, (error) => {
                if (error) {
                    dialog.showErrorBox('Permission Error',
                        'Failed to set up input permissions:\n' + error.message +
                        '\n\nYou can do it manually:\n' +
                        'sudo usermod -aG input ' + currentUser + '\n' +
                        'echo \'KERNEL=="uinput", GROUP="input", MODE="0660"\' | sudo tee /etc/udev/rules.d/99-coral-uinput.rules\n' +
                        'sudo udevadm control --reload-rules && sudo udevadm trigger /dev/uinput');
                    stopBackend();
                    app.quit();
                } else {
                    dialog.showMessageBox({
                        type: 'info',
                        title: 'Setup Complete',
                        message: 'Input permissions have been configured.\n\n' +
                                 'Please reboot your computer for the changes to take effect,\n' +
                                 'then start Coral again.',
                        buttons: ['Reboot Now', 'Later'],
                        defaultId: 0,
                        cancelId: 1,
                    }).then((res) => {
                        stopBackend();
                        if (res.response === 0) {
                            const { exec } = require('child_process');
                            exec('systemctl reboot');
                        }
                        app.quit();
                    });
                }
            });
        } else {
            // User clicked Skip — quit anyway since the app won't work without permissions
            stopBackend();
            app.quit();
        }
    });
}

function startBackend() {
    if (isChildAlive(backendProcess)) {
        console.log('Backend already running in this Electron process; skipping spawn.');
        return;
    }
    let coralBinary, configPath, backendCwd, backendEnv;
    const isWin = process.platform === 'win32';
    const userConfigDir = path.join(os.homedir(), '.coral');
    const userConfigPath = path.join(userConfigDir, 'conf', 'config.json');

    if (process.env.APPIMAGE)
    {
        // AppImage: binary is in usr/bin/coral
        const appImageMountPath = getAppImageMountPath();
        coralBinary = path.join(appImageMountPath, 'usr', 'bin', 'coral');
        const defaultConfigPath = path.join(appImageMountPath, 'usr', 'share', 'coral', 'conf', 'config.json');
        try {
            console.log('Checking if user config directory exists:', userConfigDir);
            if (!fs.existsSync(path.dirname(userConfigPath))) {
                fs.mkdirSync(path.dirname(userConfigPath), { recursive: true });
                console.log('Created user config directory:', userConfigDir);
            }
            console.log('Checking if user config exists:', userConfigPath);
            console.log('Checking if default config exists:', defaultConfigPath);
            if (!fs.existsSync(userConfigPath) && fs.existsSync(defaultConfigPath)) {
                fs.copyFileSync(defaultConfigPath, userConfigPath);
                console.log('Copied default config to user config:', defaultConfigPath, '->', userConfigPath);
            } else {
                if (fs.existsSync(userConfigPath)) {
                    console.log('User config already exists:', userConfigPath);
                }
                if (!fs.existsSync(defaultConfigPath)) {
                    console.log('Default config does not exist:', defaultConfigPath);
                }
            }
        } catch (e) {
            console.error('Failed to prepare user config:', e.message);
        }
        configPath = userConfigPath;
        backendCwd = path.join(appImageMountPath, 'usr', 'bin');
        backendEnv = {
            ...process.env,
            LD_LIBRARY_PATH: path.join(appImageMountPath, 'usr', 'lib') + (process.env.LD_LIBRARY_PATH ? (':' + process.env.LD_LIBRARY_PATH) : '')
        };
    }
    else if (isWin)
    {
        // Windows: packaged (MSI) vs development
        if (app.isPackaged) {
            // Installed app: coral.exe and resources are in process.resourcesPath
            coralBinary = path.join(process.resourcesPath, 'coral.exe');
            if (!fs.existsSync(coralBinary)) {
                coralBinary = path.join(path.dirname(process.execPath), 'resources', 'coral.exe');
            }
        } else {
            // Development: coral.exe in build-win/Release
            const repoRoot = path.join(__dirname, '..');
            coralBinary = path.join(repoRoot, 'build-win', 'Release', 'coral.exe');
            if (!fs.existsSync(coralBinary)) {
                coralBinary = path.join(repoRoot, 'build-win', 'coral', 'Release', 'coral.exe');
            }
            if (!fs.existsSync(coralBinary)) {
                const { execSync } = require('child_process');
                try {
                    const found = execSync(`dir /s /b "${path.join(repoRoot, 'build-win')}\\coral.exe" 2>nul`, { encoding: 'utf-8' }).trim().split('\n')[0];
                    if (found && fs.existsSync(found)) coralBinary = found.trim();
                } catch (_) {}
            }
        }
        const defaultConfigSrc = app.isPackaged
            ? path.join(process.resourcesPath, 'conf', 'config.json')
            : path.join(__dirname, '..', 'coral', 'conf', process.platform === 'win32' ? 'config.json' : 'config-linux.json');
        try {
            if (!fs.existsSync(path.dirname(userConfigPath))) {
                fs.mkdirSync(path.dirname(userConfigPath), { recursive: true });
                console.log('Created user config directory:', userConfigDir);
            }
            if (!fs.existsSync(userConfigPath) && fs.existsSync(defaultConfigSrc)) {
                fs.copyFileSync(defaultConfigSrc, userConfigPath);
                console.log('Copied default config to user config (first run):', defaultConfigSrc, '->', userConfigPath);
            }
        } catch (e) {
            console.error('Failed to prepare user config:', e.message);
        }
        configPath = userConfigPath;
        backendCwd = path.dirname(coralBinary);
        backendEnv = { ...process.env };
    }
    else
    {
        // Linux development: use ~/.coral/config.json, copy from config-linux.json on first run
        coralBinary = path.join(__dirname, '..', 'coral', 'bin', 'coral');
        const defaultConfigSrc = path.join(__dirname, '..', 'coral', 'conf', 'config-linux.json');
        try {
            if (!fs.existsSync(path.dirname(userConfigPath))) fs.mkdirSync(path.dirname(userConfigPath), { recursive: true });
            if (!fs.existsSync(userConfigPath) && fs.existsSync(defaultConfigSrc)) {
                fs.copyFileSync(defaultConfigSrc, userConfigPath);
            }
        } catch (e) { console.error('Failed to prepare user config:', e.message); }
        configPath = userConfigPath;
        backendCwd = path.dirname(coralBinary);
        backendEnv = process.env;
    }
    // Ensure DISPLAY is set for X11 injection (Linux only)
    if (!isWin) {
        backendEnv.DISPLAY = process.env.DISPLAY || ':0';
    }
    backendProcess = spawn(coralBinary, [configPath], {
        cwd: backendCwd,
        detached: false,
        stdio: ['ignore', 'pipe', 'ignore'], // Enable stdout pipe
        env: backendEnv
    });
    // Listen for trigger events from backend
    const rl = readline.createInterface({ input: backendProcess.stdout });
    rl.on('line', (line) => {
        if (line === 'TRIGGER_DOWN') {
            startTrayAnimation();
        } else if (line === 'TRIGGER_UP') {
            stopTrayAnimation();
        } else if (line === 'NEED_INPUT_GROUP' || line === 'NEED_UINPUT_RULE') {
            // Close welcome first, then show permission dialog
            closeWelcomeWindow();
            promptInputGroupPermission();
        } else if (line === 'BACKEND_READY') {
            updateWelcomeReady();
        }
    });
    backendProcess.on('error', (err) => {
        dialog.showErrorBox('Backend Error', 'Failed to start backend: ' + err.message);
    });
    backendProcess.on('exit', (code, signal) => {
        console.log('Backend process exited with code:', code, 'signal:', signal);
    });
    // Log backend process PID for confirmation
    if (backendProcess.pid)
    {
        console.log('Backend process started with PID:', backendProcess.pid);
    }
    else
    {
        console.log('Backend process failed to start.');
    }
}

function stopBackend() {
  if (backendProcess) {
    backendProcess.kill();
    backendProcess = null;
  }
}

async function restartBackend() {
  try {
    stopBackend();
  } catch (_) {}
  // slight delay to allow OS to release resources
  setTimeout(() => {
    try { startBackend(); } catch (e) { console.error('Failed to restart backend:', e.message); }
  }, 200);
}

let welcomeWindow = null;

function showWelcomeWindow() {
  if (welcomeWindow) return;
  welcomeWindow = new BrowserWindow({
    width: 420,
    height: 260,
    resizable: false,
    minimizable: false,
    maximizable: false,
    alwaysOnTop: true,
    frame: false,
    show: false,
    transparent: false,
    skipTaskbar: true,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
    },
  });
  welcomeWindow.once('ready-to-show', () => {
    welcomeWindow.show();
  });
  welcomeWindow.on('closed', () => { welcomeWindow = null; });

  const welcomeHtml = `
    <html>
      <head>
        <title>Coral</title>
        <style>
          body { font-family: sans-serif; background: #fff; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%; }
          .about-box { margin-top: 30px; padding: 24px 24px 16px 24px; border-radius: 12px; box-shadow: 0 2px 16px #0002; background: #f8faff; text-align: center; }
          .title { font-size: 1.3em; font-weight: bold; margin-bottom: 8px; }
          .version { color: #555; margin-bottom: 8px; }
          .desc { margin-bottom: 8px; }
          .status { color: #888; font-size: 0.95em; margin-top: 12px; }
          .status.ready { color: #2e7d32; }
          .tip { color: #0d47a1; font-size: 1em; margin-top: 10px; display: none; }
          .ok-btn { margin-top: 18px; padding: 8px 24px; font-size: 1em; border-radius: 6px; border: 1px solid #0d47a1; background: #e3eaff; color: #0d47a1; cursor: pointer; display: none; }
          .ok-btn:hover { background: #c5d8fa; }
          @keyframes dots { 0% { content: ''; } 33% { content: '.'; } 66% { content: '..'; } 100% { content: '...'; } }
          .loading::after { content: ''; animation: dots 1.2s steps(4, end) infinite; }
        </style>
      </head>
      <body>
        <div class="about-box">
          <div class="title">Coral App</div>
          <div class="version">Version: ${app.getVersion()}</div>
          <div class="desc">&copy; 2025 Coral Contributors</div>
          <div class="status loading" id="status">Starting up, checking permissions</div>
          <div class="tip" id="tip">Press the trigger key (Alt+Z) to start/stop recording.</div>
          <button class="ok-btn" id="okBtn" onclick="window.close()">OK</button>
        </div>
      </body>
    </html>
  `;
  welcomeWindow.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(welcomeHtml));
}

function updateWelcomeReady() {
  if (!welcomeWindow) return;
  try {
    welcomeWindow.webContents.executeJavaScript(`
      document.getElementById('status').className = 'status ready';
      document.getElementById('status').textContent = 'Ready!';
      document.getElementById('tip').style.display = 'block';
      document.getElementById('okBtn').style.display = 'inline-block';
    `);
    // Auto-close after 5 seconds if the user doesn't click OK
    setTimeout(() => {
      try { if (welcomeWindow) welcomeWindow.close(); } catch (_) {}
    }, 5000);
  } catch (_) {}
}

function closeWelcomeWindow() {
  try { if (welcomeWindow) welcomeWindow.close(); } catch (_) {}
}

const gotLock = app.requestSingleInstanceLock();

if (!gotLock) {
  app.quit();
} else {
  app.on('second-instance', () => {
    if (configWindow) {
      if (configWindow.isMinimized()) configWindow.restore();
      configWindow.focus();
    }
  });
}

app.whenReady().then(() => {
  // Remove the default application menu (File/Edit/View/Window/Help)
  Menu.setApplicationMenu(null);
  // Tray icon - ensure path exists (Windows may fail silently with bad path)
  if (!fs.existsSync(originalIconPath)) {
    console.warn('Tray icon not found at:', originalIconPath, '- tray may not display correctly');
  }
  try {
    tray = new Tray(originalIconPath);
  } catch (e) {
    console.error('Failed to create tray:', e.message);
  }
  if (!tray) {
    dialog.showErrorBox('Coral', 'Could not create system tray icon. Ensure coral.png exists in coral-electron or logo folder.');
    app.quit();
    return;
  }
  const contextMenu = Menu.buildFromTemplate([
    { label: 'Settings', click: createConfigWindow },
    { label: 'Developer Settings', click: createDevWindow },
    { label: 'About', click: () => {
        dialog.showMessageBox({
          type: 'info',
          title: 'About Coral',
          message: 'Coral App\nVersion: ' + app.getVersion() + '\n\u00A9 2025 Coral Contributors\n\nTo transcribe your speech, hold down the trigger key (Alt+z) and start talking.',
          buttons: ['OK']
        });
      }
    },
    { type: 'separator' },
    { label: 'Quit', click: () => { stopBackend(); app.quit(); } },
  ]);
  tray.setToolTip('Coral App');
  tray.setContextMenu(contextMenu);
  showWelcomeWindow();
  startBackend();
});

app.on('window-all-closed', (e) => {
  // Prevent app from quitting when config window is closed
  e.preventDefault();
});

app.on('before-quit', () => {
  stopBackend();
});

// Add IPC handler for Spectron and other integration tests
ipcMain.on('show-config', () => {
  createConfigWindow();
});

// Restart backend when renderer reports config changes
ipcMain.on('config-updated', () => {
  restartBackend();
});

// Allow renderers to request window height resize to fit content
ipcMain.on('resize-window', (event, contentHeight) => {
  try {
    const win = BrowserWindow.fromWebContents(event.sender);
    if (!win) return;
    const [currentWidth] = win.getContentSize();
    const minH = 200;
    const display = screen.getDisplayMatching(win.getBounds()) || screen.getPrimaryDisplay();
    const workH = (display && display.workAreaSize && display.workAreaSize.height) ? display.workAreaSize.height : 900;
    const maxH = Math.max(320, workH - 80);
    const padded = Math.max(minH, Math.min(maxH, Math.ceil(Number(contentHeight) || 0) + 20));
    win.setContentSize(currentWidth, padded);
  } catch (_) {}
});

// IPC handler for folder selection
ipcMain.handle('select-folder', async () => {
  const result = await dialog.showOpenDialog({
    properties: ['openDirectory']
  });
  if (result.canceled || !result.filePaths.length) return null;
  return result.filePaths[0];
});

// IPC handler for selecting a model file instead of a folder
ipcMain.handle('select-model-file', async () => {
  const result = await dialog.showOpenDialog({
    properties: ['openFile'],
    filters: [
      { name: 'Whisper models', extensions: ['bin', 'gguf'] },
      { name: 'All Files', extensions: ['*'] }
    ]
  });
  if (result.canceled || !result.filePaths.length) return null;
  return result.filePaths[0];
});
