const { ipcMain } = require('electron');

ipcMain.handle('getAppVersion', () => app.getVersion())
const { app, Tray, Menu, BrowserWindow, dialog} = require('electron');
const path = require('path');
const os = require('os');
const fs = require('fs');
const { spawn } = require('child_process');
const readline = require('readline');

let originalIconPath;
const ANIMATION_FRAME_COUNT = 8;
let animationFrames = [];
if (process.env.APPIMAGE) {
  for (let i = 0; i < ANIMATION_FRAME_COUNT; i++) {
    animationFrames.push(path.join(process.resourcesPath, 'icons', 'wave_icons', `wave_${i}.png`));
  }
  originalIconPath = path.join(process.resourcesPath, 'coral.png');
} else {
  for (let i = 0; i < ANIMATION_FRAME_COUNT; i++) {
    animationFrames.push(path.join(__dirname, 'icons', 'wave_icons', `wave_${i}.png`));
  }
  originalIconPath = path.join(__dirname, 'coral.png');
}

let tray = null;
let configWindow = null;
let backendProcess = null;
let animationInterval = null;
let currentFrame = 0;

function createConfigWindow() {
  if (configWindow) {
    configWindow.focus();
    return;
  }
  configWindow = new BrowserWindow({
    width: 500,
    height: 600,
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

function startBackend() {
    if (isChildAlive(backendProcess)) {
        console.log('Backend already running in this Electron process; skipping spawn.');
        return;
    }
    let coralBinary, configPath, backendCwd, backendEnv;
    if (process.env.APPIMAGE)
    {
        // AppImage: binary is in usr/bin/coral
        const appImageMountPath = getAppImageMountPath();
        coralBinary = path.join(appImageMountPath, 'usr', 'bin', 'coral');
        const defaultConfigPath = path.join(appImageMountPath, 'usr', 'share', 'coral', 'conf', 'config.json');
        const userConfigDir = path.join(os.homedir(), '.coral');
        const userConfigPath = path.join(userConfigDir, 'config.json');
        try {
            console.log('Checking if user config directory exists:', userConfigDir);
            if (!fs.existsSync(userConfigDir)) {
                fs.mkdirSync(userConfigDir, { recursive: true });
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
    else
    {
        // Development: adjust paths as needed
        coralBinary = path.join(__dirname, '..', 'coral', 'bin', 'coral');
        configPath = path.join(__dirname, '..', 'coral', 'conf', 'config.json');
        backendCwd = path.dirname(coralBinary);
        backendEnv = process.env;
    }
    // Ensure DISPLAY is set for X11 injection
    backendEnv.DISPLAY = process.env.DISPLAY || ':0';
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
            //console.log('Trigger key pressed (TRIGGER_DOWN)');
            startTrayAnimation();
        } else if (line === 'TRIGGER_UP') {
            //console.log('Trigger key released (TRIGGER_UP)');
            stopTrayAnimation();
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

function showCustomAboutWindow() {
  const aboutWin = new BrowserWindow({
    width: 400,
    height: 240,
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
  aboutWin.once('ready-to-show', () => {
    aboutWin.show();
  });
  const aboutHtml = `
    <html>
      <head>
        <title>About Coral</title>
        <style>
          body { font-family: sans-serif; background: #fff; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%; }
          .about-box { margin-top: 30px; padding: 24px 24px 16px 24px; border-radius: 12px; box-shadow: 0 2px 16px #0002; background: #f8faff; text-align: center; }
          .title { font-size: 1.3em; font-weight: bold; margin-bottom: 8px; }
          .version { color: #555; margin-bottom: 8px; }
          .desc { margin-bottom: 8px; }
          .tip { color: #0d47a1; font-size: 1em; margin-top: 10px; }
          .ok-btn { margin-top: 18px; padding: 8px 24px; font-size: 1em; border-radius: 6px; border: 1px solid #0d47a1; background: #e3eaff; color: #0d47a1; cursor: pointer; }
          .ok-btn:hover { background: #c5d8fa; }
        </style>
      </head>
      <body>
        <div class="about-box">
          <div class="title">Coral App</div>
          <div class="version">Version: ${app.getVersion()}</div>
          <div class="desc">© 2025 Coral Contributors</div>
          <div class="tip">To transcribe your speech, hold down the trigger key (Alt+z) and start talking.</div>
          <button class="ok-btn" onclick="window.close()">OK</button>
        </div>
      </body>
    </html>
  `;
  aboutWin.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(aboutHtml));
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
  // Tray icon path (use coral.png in usr/share/coral for AppImage)
  tray = new Tray(originalIconPath);
  const contextMenu = Menu.buildFromTemplate([
    { label: 'Settings', click: createConfigWindow },
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
  startBackend();
  showCustomAboutWindow();
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
