const fs = require('fs');
const path = require('path');
const os = require('os');
const { ipcRenderer } = require('electron');

function getAppImageMountPath() {
  if (process.env.APPIMAGE) {
    return path.join(path.dirname(process.execPath), '..', '..', '..');
  }
  return null;
}

let configPath;
if (process.env.APPIMAGE) {
  const appImageMountPath = getAppImageMountPath();
  const defaultConfigPath = path.join(appImageMountPath, 'usr', 'share', 'coral', 'conf', 'config.json');
  const userConfigDir = path.join(os.homedir(), '.coral');
  const userConfigPath = path.join(userConfigDir, 'config.json');
  try {
    if (!fs.existsSync(userConfigDir)) {
      fs.mkdirSync(userConfigDir, { recursive: true });
    }
    if (!fs.existsSync(userConfigPath) && fs.existsSync(defaultConfigPath)) {
      fs.copyFileSync(defaultConfigPath, userConfigPath);
    }
  } catch (e) {
    console.error('Failed to prepare user config:', e.message);
  }
  configPath = userConfigPath;
} else {
  configPath = path.join(__dirname, '../..', 'coral', 'conf', 'config.json');
}

const devForm = document.getElementById('devForm');
const devSaveBtn = document.getElementById('devSaveBtn');
const devStatus = document.getElementById('devStatus');

let config = {};

function renderDevForm(cfg) {
  devForm.innerHTML = '';
  const primaryKeys = new Set(['triggerKey', 'cmdTriggerKey', 'whisperModelPath']);
  const hiddenKeys = new Set(['silenceTimeoutSeconds', 'audioSampleRate', 'audioChannels']);
  for (const [key, value] of Object.entries(cfg)) {
    if (primaryKeys.has(key) || hiddenKeys.has(key)) continue;
    const label = document.createElement('label');
    label.textContent = key;
    let input;
    if (typeof value === 'boolean') {
      input = document.createElement('select');
      ['true', 'false'].forEach(opt => {
        const option = document.createElement('option');
        option.value = opt;
        option.text = opt;
        if (String(value) === opt) option.selected = true;
        input.appendChild(option);
      });
      input.name = key;
      label.appendChild(input);
    } else if (typeof value === 'number') {
      input = document.createElement('input');
      input.type = 'number';
      input.value = value;
      input.name = key;
      label.appendChild(input);
    } else {
      input = document.createElement('input');
      input.type = 'text';
      input.value = value;
      input.name = key;
      label.appendChild(input);
    }
    devForm.appendChild(label);
  }
  requestResize();
}

function requestResize() {
  try {
    const formBottom = devForm.getBoundingClientRect().bottom;
    const desired = Math.ceil(window.scrollY + formBottom + 24);
    ipcRenderer.send('resize-window', desired);
    setTimeout(() => {
      const formBottom2 = devForm.getBoundingClientRect().bottom;
      const desired2 = Math.ceil(window.scrollY + formBottom2 + 24);
      ipcRenderer.send('resize-window', desired2);
    }, 50);
  } catch (_) {}
}

window.addEventListener('load', () => {
  requestResize();
});

function loadConfig() {
  fs.readFile(configPath, 'utf8', (err, data) => {
    if (err) {
      devStatus.textContent = 'Failed to load config: ' + err.message;
      devStatus.style.color = 'red';
      return;
    }
    try {
      config = JSON.parse(data);
      renderDevForm(config);
      devStatus.textContent = '';
      devStatus.style.color = '';
    } catch (e) {
      devStatus.textContent = 'Invalid config.json: ' + e.message;
      devStatus.style.color = 'red';
    }
  });
}

devSaveBtn.onclick = (e) => {
  e.preventDefault();
  const newConfig = { ...config };
  for (const el of devForm.elements) {
    if (!el.name) continue;
    if (el.tagName === 'SELECT') {
      newConfig[el.name] = el.value === 'true';
    } else if (el.type === 'number') {
      newConfig[el.name] = Number(el.value);
    } else {
      newConfig[el.name] = el.value;
    }
  }
  fs.writeFile(configPath, JSON.stringify(newConfig, null, 2), (err) => {
    if (err) {
      devStatus.textContent = 'Failed to save: ' + err.message;
    } else {
      devStatus.textContent = 'Saved!';
      try {
        window?.require && ipcRenderer.send('config-updated');
      } catch (_) {}
      setTimeout(() => { try { window.close(); } catch (_) {} }, 150);
    }
  });
};

loadConfig();

